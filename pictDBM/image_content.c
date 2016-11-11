/**
 * @file image_content.c
 * @brief functions to manipulate images
 *
 * @author Basile Thullen, Jeremy Hottinger
 * @date 1 May 2016
 */

#include "image_content.h"

VipsImage* resize(VipsImage* original, int new_w, int new_h);
double resize_ratio(VipsImage* image, int resized_width, int resized_height);
void free_ressources(void* res_buff_orig, VipsImage* global, VipsObject* process, void* res_buff);

/**
 * @brief resize the image with the given id to the given resolution in a pictdb file.
 *
 * @param res_code resolution code that indicates which resolution we want: RES_THUMB or RES_SMALL
 *
 * @param file pointer to the pictdb file
 *
 * @param image_id index of the image we want to resize
 *
 * @return 0 if successful, error code if not
 */
int lazily_resize(int res_code, struct pictdb_file* file, size_t image_id)
{

    // check for good arguments
    if(res_code == RES_ORIG) {
        return 0;
    } else if(res_code != RES_THUMB && res_code != RES_SMALL) {
        return ERR_INVALID_ARGUMENT;
    }

    if((file == NULL) || (image_id > file->header.max_files)) {
        return ERR_INVALID_ARGUMENT;
    }

    // if the file exists
    if(file->metadata[image_id].is_valid == NON_EMPTY) {

        // if the image at resolution doesnt exists
        if(file->metadata[image_id].offset[res_code] == 0) {
            // shortcut for often used variables
            size_t img_size = file->metadata[image_id].size[RES_ORIG];

            // allocate memory for original and resized
            void* res_buff_orig = malloc(img_size);
            if(res_buff_orig == NULL) {
                free_ressources(res_buff_orig, NULL, NULL, NULL);
                return ERR_OUT_OF_MEMORY;
            }

            // we want 1 image
            VipsImage *global = vips_image_new();
            if(global == NULL) {
                free_ressources(res_buff_orig, global, NULL, NULL);
                return ERR_VIPS;
            }

            // seek and read from memory
            if(fseek(file->fpdb, file->metadata[image_id].offset[RES_ORIG], SEEK_SET) != 0) {
                return ERR_IO;
            }

            if(fread(res_buff_orig, img_size, 1, file->fpdb) != 1) {
                return ERR_IO;
            }

            VipsImage **original = (VipsImage**)vips_object_local_array(VIPS_OBJECT(global), 1);
            if(original == NULL) {
                return ERR_VIPS;
            }

            // create image with vips_jpegload_buffer
            if(vips_jpegload_buffer(res_buff_orig, img_size, original, NULL) != 0) {
                return ERR_VIPS;
            }

            // start resize process
            // some place to do the job
            VipsObject* process = VIPS_OBJECT( vips_image_new() );

            // we want 1 new images
            VipsImage** thumbs = (VipsImage**) vips_object_local_array( process, 1 );

            double ratio = resize_ratio(original[0], file->header.res_resized[2*res_code], file->header.res_resized[(2*res_code)+1]);

#if VIPS_MAJOR_VERSION > 7 || (VIPS_MAJOR_VERSION == 7 && MINOR_VERSION > 40)
            // was only introduced in libvips 7.42
            if(vips_resize(original[0], &thumbs[0], ratio, NULL) != 0) {
                free_ressources(res_buff_orig, global, process, NULL);
                return ERR_VIPS;
            }
#else
            printf("Requires vips version >= 7.42\n");
            free_ressources(res_buff_orig, global, process, NULL);
            return ERR_VIPS;
#endif


            if(thumbs[0] == NULL) {
                free_ressources(res_buff_orig, global, process, NULL);
                return ERR_VIPS;
            }

            // save on disk
            void* res_buff = NULL;
            size_t res_length = 0;

            if(vips_jpegsave_buffer(thumbs[0], &res_buff, &res_length, NULL) != 0) {
                free_ressources(res_buff_orig, global, process, res_buff);
                return ERR_VIPS;
            }

            // go to the end of the file
            if(fseek(file->fpdb, 0, SEEK_END) != 0) {
                free_ressources(res_buff_orig, global, process, res_buff);
                return ERR_IO;
            }

            // save offset
            int off = ftell(file->fpdb);
            if(off < 0) {
                free_ressources(res_buff_orig, global, process, res_buff);
                return ERR_IO;
            }

            // write
            if(fwrite(res_buff, res_length, 1, file->fpdb) != 1) {
                free_ressources(res_buff_orig, global, process, res_buff);
                return ERR_IO;
            }

            // update metadata
            file->metadata[image_id].offset[res_code] = off;
            file->metadata[image_id].size[res_code] = res_length;

            if(fseek(file->fpdb, sizeof(struct pictdb_header)+image_id*sizeof(struct pict_metadata), SEEK_SET) != 0) {
                free_ressources(res_buff_orig, global, process, res_buff);
                return ERR_IO;
            }

            // write metadata
            if(fwrite(&(file->metadata[image_id]), sizeof(struct pict_metadata), 1, file->fpdb) != 1) {
                free_ressources(res_buff_orig, global, process, res_buff);
                return ERR_IO;
            }

            free_ressources(res_buff_orig, global, process, res_buff);

        } else {
            return 0;
        }
    } else {
        return ERR_INVALID_PICID;
    }
    return 0;
}

/**
 * @brief free ressources passed in parameters if they are not null
 */
void free_ressources(void* res_buff_orig, VipsImage* global, VipsObject* process, void* res_buff)
{
    if(res_buff_orig != NULL) {
        free(res_buff_orig);
    }
    if(global != NULL) {
        g_object_unref(global);
    }
    if(process != NULL) {
        g_object_unref(process);
    }
    if(res_buff != NULL) {
        g_free(res_buff);
    }
}

/**
 * @brief get the ratio to reduce the image
 *
 * @param image pointer to the vips image that will be reduced
 * @param resized_width the maximum width that the reduced image can have
 * @param resized_height the maximum height that the reduced image can have
 *
 * @return the ratio
 */
double resize_ratio(VipsImage* image, int resized_width, int resized_height)
{
    const double h_shrink = (double) resized_width  / (double) image->Xsize ;
    const double v_shrink = (double) resized_height / (double) image->Ysize ;
    return h_shrink > v_shrink ? v_shrink : h_shrink ;
}

/**
 * @brief get the resolution of an image
 *
 * @param height return argument for the height of the image
 * @param width return argument for the widht of the image
 * @param image_buffer image in memory
 * @param image_size size of the image in memory
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size)
{
    VipsImage *global = vips_image_new();
    if(global == NULL) {
        return ERR_VIPS;
    }

    VipsImage **image = (VipsImage**)vips_object_local_array(VIPS_OBJECT(global), 1);
    if(image == NULL) {
        return ERR_VIPS;
    }

    if(vips_jpegload_buffer (image_buffer, image_size, image, NULL) != 0) {
        return ERR_VIPS;
    }

    *width = image[0]->Xsize;
    *height = image[0]->Ysize;

    g_object_unref(global);

    return 0;
}
