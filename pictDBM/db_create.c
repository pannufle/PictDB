/**
 * @file db_create.c
 * @brief pictDB library: do_create implementation.
 *
 * @author Basile Thullen, Jeremy Hottinger
 * @date 13 Apr 2016
 */

#include "pictDB.h"

#include <string.h> // for strncpy

/**
 * @brief Creates the database called filename. Writes the header and the
 *  preallocated empty metadata array to database file.
 *
 * @param filename Filename of the file to write the file to
 *
 * @param db_file Database representation
 */
int do_create(const char* filename, struct pictdb_file* db_file)
{
    // Check for good parameters
    if((filename == NULL) || (filename[0] == '\0') || (db_file == NULL)) {
        return ERR_INVALID_ARGUMENT;
    }

    // Sets the DB header name
    size_t name_len = strlen(CAT_TXT);
    strncpy(db_file->header.db_name, CAT_TXT, name_len);
    db_file->header.db_name[name_len] = '\0';
    db_file->header.db_version = 0;
    db_file->header.num_files = 0;

    // Initialize metadata
    db_file->metadata = calloc(db_file->header.max_files, sizeof(struct pict_metadata));
    if(db_file->metadata == NULL) {
        return ERR_OUT_OF_MEMORY;
    }

    db_file->fpdb = fopen(filename, "wb+");
    if(db_file == NULL) {
        free(db_file->metadata);
        return ERR_IO;
    }

    int items = fwrite(&(db_file->header), sizeof(struct pictdb_header), 1, db_file->fpdb);
    // write the db header
    if(items != 1) {
        fclose(db_file->fpdb);
        free(db_file->metadata);
        return ERR_IO;
    }

    // write the metadata
    items += fwrite(db_file->metadata, sizeof(struct pict_metadata), db_file->header.max_files, db_file->fpdb);

    //fclose(db_file->fpdb);

    if(items != db_file->header.max_files+1) {
        free(db_file->metadata);
        return ERR_IO;
    }

    printf("%d items written\n", items);

    return 0;
}
