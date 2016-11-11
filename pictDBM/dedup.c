/**
 * @file dedup.c
 * @brief method to check for duplicates in a database file
 *
 * @author Basile Thullen, Jeremy Hottinger
 * @date 1 May 2016
 */

#include "dedup.h"

int sha_equal(unsigned char* sha1, unsigned char* sha2);

/**
 * @brief check for duplicate of a certain image referenced by an index
 *
 * @param db_file database file to search in
 * @param index index of the image to search for duplicates
 */
int do_name_and_content_dedup(struct pictdb_file* db_file, uint32_t index)
{

    // check if arguments are correct
    if(db_file == NULL || index > db_file->header.max_files) {
        return ERR_INVALID_ARGUMENT;
    }

    char* img_name = db_file->metadata[index].pict_id;
    int i = 0;

    // check for duplicate pic_id
    for(i = 0; i<db_file->header.max_files; i++) {
        if(db_file->metadata[i].is_valid == NON_EMPTY) {
            if(strcmp(db_file->metadata[i].pict_id, img_name) == 0) {
                return ERR_DUPLICATE_ID;
            }
        }
    }

    for(i = 0; i<db_file->header.max_files; i++) {
        if(db_file->metadata[i].is_valid == NON_EMPTY) {
            if(i != index) {
                // same sha -> update metadata
                if(sha_equal(db_file->metadata[i].SHA, db_file->metadata[index].SHA) == 0) {

                    int j = 0;
                    for(j = 0; j<NB_RES; j++) {
                        // update offset and sizes
                        db_file->metadata[index].size[j] = db_file->metadata[i].size[j];
                        db_file->metadata[index].offset[j] = db_file->metadata[i].offset[j];
                    }
                    db_file->metadata[index].res_orig[0] = db_file->metadata[i].res_orig[0];
                    db_file->metadata[index].res_orig[1] = db_file->metadata[i].res_orig[1];

                    return 0;
                }
            }
        }
    }

    // file has no duplicate
    db_file->metadata[index].offset[RES_ORIG] = 0;
    return 0;

}

/**
 * @brief compare two SHA digest
 *
 * @param sha1 first sha to compare
 * @param sha2 second sha to compare
 *
 * @return  0 if sha1 = sha2
            something else if sha1 != sha2
 */
int sha_equal(unsigned char* sha1, unsigned char* sha2)
{
    if(sha1 != NULL && sha2 != NULL) {
        int i = 0;
        for(i = 0; i<SHA256_DIGEST_LENGTH; i++) {
            if(sha1[i] != sha2[i]) {
                return 1;
            }
        }
        return 0;
    }
    return 1;
}
