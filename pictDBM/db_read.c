/**
 * @file db_read.c
 * @brief implementation of function to read an image from a database file
 *
 * @author Basile Thullen - Jeremy Hottinger
 * @date 11 May 2016
 */

#include "pictDB.h"
#include "image_content.h"

/**
 * @brief reads an image from the database
 *
 * @param img_id name of the image to read in the database
 * @param res_code code of the resolution
 * @param img_array array to set with the content of the image
 * @param size pointer to the size of the image
 * @param db_file database from which to read
 */
int do_read(const char* img_id, int res_code, char** img_array, uint32_t* size, struct pictdb_file* db_file)
{

    if(img_id == NULL || res_code >= NB_RES || res_code < 0 || db_file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    int i = 0;
    // get the index of the image with img_id
    while((strcmp(db_file->metadata[i].pict_id, img_id) != 0 || db_file->metadata[i].is_valid == EMPTY) && i <= db_file->header.max_files) {
        i++;
    }

    if(i == db_file->header.max_files) {
        // image doesn't exists in database
        return ERR_FILE_NOT_FOUND;
    }

    // check if image at corresponding resolution exists
    if(db_file->metadata[i].offset[res_code] == 0 || db_file->metadata[i].size[res_code] == 0) {

        if(res_code == RES_ORIG) {
            return ERR_FILE_NOT_FOUND;
        }

        // resize original to the resolution
        int res = lazily_resize(res_code, db_file, i);
        if(res != 0) {
            return res;
        }
    }

    // at this point we know that the image at correspondign resolution exists

    if(fseek(db_file->fpdb, db_file->metadata[i].offset[res_code], SEEK_SET) != 0) {
        return ERR_IO;
    }

    // allocate memory to receive the image from the file
    char* img = calloc(db_file->metadata[i].size[res_code], sizeof(char));
    if(img == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    if(fread(img, db_file->metadata[i].size[res_code], 1, db_file->fpdb) != 1) {
        free(img);
        return ERR_IO;
    }

    // set return arguments
    *img_array = img;
    *size = db_file->metadata[i].size[res_code];
    return 0;

}
