/**
 * @file db_gbcollect.c
 * @brief pictDB library: garbage collecting
 *
 * @author Basile Thullen, Jeremy Hottinger
 * @date 30 May 2016
 */

#include <stdio.h>
#include "pictDB.h"
#include "image_content.h"

int copy_and_delete(char* old, char* new);
int check_hole(struct pictdb_file* file);

int do_gbcollect(struct pictdb_file* db_file, char* orig_filename, char* new_filename)
{

    // create new temporary pictdb_file
    struct pictdb_file new_db_file;

    new_db_file.header.max_files = db_file->header.max_files;
    new_db_file.header.res_resized[0] = db_file->header.res_resized[0];
    new_db_file.header.res_resized[1] = db_file->header.res_resized[1];
    new_db_file.header.res_resized[2] = db_file->header.res_resized[2];
    new_db_file.header.res_resized[3] = db_file->header.res_resized[3];

    do_create(new_filename, &new_db_file);

    uint32_t max_files = db_file->header.max_files;

    int res = 0;
    int new_db_index = 0;
    int i;
    for(i = 0; i < max_files; i ++) {
        if(db_file->metadata[i].is_valid == 1) { // there is a valid image here
            int offset_orig = db_file->metadata[i].offset[RES_ORIG];
            char* img_array;
            uint32_t size;

            res = do_read(db_file->metadata[i].pict_id, RES_ORIG, &img_array, &size, db_file);
            if(res != 0) {
                remove(new_filename);
                return res;
            }

            res = do_insert(img_array, size, db_file->metadata[i].pict_id, &new_db_file);
            if(res != 0) {
                remove(new_filename);
                return res;
            }

            if(db_file->metadata[i].size[RES_SMALL] != 0 || db_file->metadata[i].offset[RES_SMALL] != 0) {
                res = lazily_resize(RES_SMALL, &new_db_file, new_db_index);
                if(res != 0) {
                    remove(new_filename);
                    return res;
                }
            }

            if(db_file->metadata[i].size[RES_THUMB] != 0 || db_file->metadata[i].offset[RES_THUMB] != 0) {
                res = lazily_resize(RES_THUMB, &new_db_file, new_db_index);
                if(res != 0) {
                    remove(new_filename);
                    return res;
                }
            }
            new_db_index ++;
        }
    }
    new_db_file.header.db_version = db_file->header.db_version;

    res = copy_and_delete(orig_filename, new_filename);
    if(res != 0) {
        remove(new_filename);
        return res;
    }
    return 0;
}

int copy_and_delete(char* old, char* new)
{
    if(remove(old) != 0) {
        return ERR_IO;
    }
    if(rename(new, old) != 0) {
        return ERR_IO;
    }
    return 0;
}
