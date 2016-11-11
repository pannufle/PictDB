/**
 * @file db_delete.c
 * @brief pictDB library: do_delete implementation.
 *
 * @author Basile Thullen, Jeremy Hottinger
 * @date 14 Apr 2016
 */

#include "pictDB.h"

/**
 * @brief Delete an image from a database file
 *
 * @param name Name of the image to delete
 * @param file File from which the image must be deleted
 */
int do_delete(const char* name, struct pictdb_file* file)
{

    // Check for good parameters
    if((name == NULL) || (name[0] == '\0')) {
        return ERR_INVALID_ARGUMENT;
    }

    if(file == NULL) {
        return ERR_INVALID_ARGUMENT;
    }

    // find metadata corresponding to image
    int index = -1;
    size_t cur = 0;
    // find the index of the image to delete
    while(index < 0 && cur < file->header.max_files) {
        // Compare the name given in parameter with the image name
        if(file->metadata[cur].is_valid == NON_EMPTY && strcmp(file->metadata[cur].pict_id, name) == 0) {
            index = cur;
        }
        cur ++;
    }

    // picture id not found
    if(index < 0) {
        return ERR_FILE_NOT_FOUND;
    }

    file->metadata[index].is_valid = EMPTY;

    // save everything back on the disk
    if(file->fpdb == NULL) {
        return ERR_IO;
    }

    // sets the file position to the metadata struct
    if(fseek(file->fpdb, sizeof(struct pictdb_header) + sizeof(struct pict_metadata) * index, SEEK_SET) != 0) {
        return ERR_IO;
    }
    // write the metadata
    int items = fwrite(&(file->metadata[index]), sizeof(struct pict_metadata), 1, file->fpdb);
    if(items != 1) {
        return ERR_IO;
    }

    // update header
    if(file->header.num_files > 0) {
        --(file->header.num_files);
    }
    ++(file->header.db_version);

    // write header to disk
    if(fseek(file->fpdb, 0, SEEK_SET) != 0) {
        return ERR_IO;
    }
    items = fwrite(&(file->header), sizeof(struct pictdb_header), 1, file->fpdb);
    if(items != 1) {
        return ERR_IO;
    }

    return 0;
}
