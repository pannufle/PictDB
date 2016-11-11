/**
 * @file db_insert.c
 * @brief implementation of a function that insert an image in the database
 *
 * @author Basile Thullen - Jeremy Hottinger
 * @date 10 May 2016
 */

#include "pictDB.h"
#include <openssl/sha.h>
#include "image_content.h"
#include "dedup.h"

int update_file(struct pictdb_file* db_file, size_t index);

int do_insert(const char* img_array, size_t img_size, const char* img_id, struct pictdb_file* db_file)
{

    if(db_file == NULL || img_id == NULL || img_id[0] == '\0' || img_array == NULL || img_size <= 0) {
        return ERR_INVALID_ARGUMENT;
    }

    // database is full
    if(db_file->header.num_files >= db_file->header.max_files) {
        return ERR_FULL_DATABASE;
    }

    int i = 0;
    while(i <= db_file->header.max_files && db_file->metadata[i].is_valid == NON_EMPTY) {
        i++;
    }

    if(i > db_file->header.max_files) {
        return ERR_FULL_DATABASE;
    }

    // we found a place at index i
    // compute sha
    (void)SHA256((unsigned char *)img_array, img_size, db_file->metadata[i].SHA);
    strncpy(db_file->metadata[i].pict_id, img_id, strlen(img_id));
    db_file->metadata[i].pict_id[strlen(img_id)] = '\0';
    db_file->metadata[i].size[RES_ORIG] = img_size;

    // check for dedup
    int dedup_res = do_name_and_content_dedup(db_file, i);
    if(dedup_res != 0) {
        return dedup_res;
    }

    // an image with the same SHA was found by dedup
    if(db_file->metadata[i].offset[RES_ORIG] != 0) {
        db_file->metadata[i].is_valid = NON_EMPTY;
        int res = update_file(db_file, i);
        return res;
    }

    if(fseek(db_file->fpdb, 0, SEEK_END) != 0) {
        return ERR_IO;
    }

    long offset = ftell(db_file->fpdb);

    if(fwrite(img_array, img_size, 1, db_file->fpdb) != 1) {
        return ERR_IO;
    }

    int a = 0;
    for(a = 0; a<NB_RES; a++) {
        if(a != RES_ORIG) {
            db_file->metadata[i].offset[a] = 0;
            db_file->metadata[i].size[a] = 0;
        }
    }

    db_file->metadata[i].offset[RES_ORIG] = (uint32_t) offset;
    db_file->metadata[i].is_valid = NON_EMPTY;

    int res = get_resolution(&(db_file->metadata[i].res_orig[1]), &(db_file->metadata[i].res_orig[0]), img_array, img_size);
    if(res != 0) {
        db_file->metadata[i].is_valid = EMPTY;
        return res;
    }

    res = update_file(db_file, i);

    return res;
}

int update_file(struct pictdb_file* db_file, size_t index)
{

    db_file->header.num_files ++;
    db_file->header.db_version ++;

    if(fseek(db_file->fpdb, 0, SEEK_SET) != 0) {
        db_file->metadata[index].is_valid = EMPTY;
        return ERR_IO;
    }
    if(fwrite(&(db_file->header), sizeof(struct pictdb_header), 1, db_file->fpdb) != 1) {
        db_file->metadata[index].is_valid = EMPTY;
        return ERR_IO;
    }

    if(fseek(db_file->fpdb, sizeof(struct pictdb_header) + index*sizeof(struct pict_metadata), SEEK_SET)) {
        db_file->metadata[index].is_valid = EMPTY;
        return ERR_IO;
    }
    if(fwrite(&(db_file->metadata[index]), sizeof(struct pict_metadata), 1, db_file->fpdb) != 1) {
        db_file->metadata[index].is_valid = EMPTY;
        return ERR_IO;
    }

    return 0;
}
