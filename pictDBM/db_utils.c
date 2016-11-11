/**
 * @file db_utils.c
 * @brief implementation of several tool functions for pictDB
 *
 * @author Basile Thullen - Jeremy Hottinger
 * @date 13 Apr 2016
 */

#include "pictDB.h"

#include <stdint.h>         // for uint8_t
#include <stdio.h>          // for sprintf
#include <openssl/sha.h>    // for SHA256_DIGEST_LENGTH
#include <inttypes.h>

/**
 * @brief Human-readable SHA
 */
static void
sha_to_string (const unsigned char* SHA,
               char* sha_string)
{
    if (SHA == NULL) {
        return;
    }
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        sprintf(&sha_string[i*2], "%02x", SHA[i]);
    }

    sha_string[2*SHA256_DIGEST_LENGTH] = '\0';
}

/**
 * @brief pictDB header display.
 *
 * @param header Header to display
 */
void print_header(const struct pictdb_header* header)
{
    if(header != NULL) {
        printf("*****************************************\n");
        printf("**********DATABASE HEADER START**********\n");
        printf("DB NAME:%31s\n", header->db_name);
        printf("VERSION: %" PRIu32 "\n", header->db_version);
        printf("IMAGE COUNT: %" PRIu32 "\tMAX IMAGES: %" PRIu32 "\n", header->num_files, header->max_files);
        printf("THUMBNAIL: %" PRIu16 " x %" PRIu16 "\t\tSMALL: %" PRIu16 " x %" PRIu16"\n", header->res_resized[0], header->res_resized[1], header->res_resized[2], header->res_resized[3]);
        printf("***********DATABASE HEADER END***********\n");
        printf("*****************************************\n");
    }
}

/**
 * @brief Metadata display.
 *
 * @param metadata Metadata to display
 */
void
print_metadata (const struct pict_metadata* metadata)
{
    if(metadata != NULL) {
        char sha_printable[2*SHA256_DIGEST_LENGTH+1];
        sha_to_string(metadata->SHA, sha_printable);

        printf("PICTURE ID: %s\n", metadata->pict_id);
        printf("SHA: %s\n", sha_printable);
        printf("VALID: %" PRIu16 "\n", metadata->is_valid);
        printf("UNUSED: %" PRIu16 "\n", metadata->unused_16);
        printf("OFFSET ORIG. : %" PRIu64 "\t\tSIZE ORIG. : %" PRIu32 "\n", metadata->offset[RES_ORIG], metadata->size[RES_ORIG]);
        printf("OFFSET THUMB.: %" PRIu64 "\t\tSIZE THUMB.: %" PRIu32 "\n", metadata->offset[RES_THUMB], metadata->size[RES_THUMB]);
        printf("OFFSET SMALL : %" PRIu64 "\t\tSIZE SMALL : %" PRIu32 "\n", metadata->offset[RES_SMALL], metadata->size[RES_SMALL]);
        printf("ORIGINAL: %" PRIu32 " x %" PRIu32 "\n", metadata->res_orig[0], metadata->res_orig[1]);
        printf("*****************************************\n");
    }
}

/**
 * @brief Open a pictdb_file in a certain mode
 *
 * @param filename Filename of the database
 * @param mode Opening mode of the file
 * @param db_file Pictdb_file in which the opened image will be stored
 */
int do_open(const char* filename, const char* mode, struct pictdb_file* db_file)
{
    // Check for good parameters
    if((filename == NULL) || (filename[0] == '\0') || (mode == NULL) || (mode[0] == '\0') || (db_file == NULL)) {
        printf("Invalid argument\n");
        return ERR_INVALID_ARGUMENT;
    }

    // Open and check if the opening worked
    db_file->fpdb = fopen(filename, mode);
    if(db_file->fpdb == NULL) {
        return ERR_IO;
    }

    // Read the header
    fseek(db_file->fpdb, 0L, SEEK_SET);
    int header_count = fread(&(db_file->header), sizeof(struct pictdb_header), 1, db_file->fpdb);
    if(header_count != 1) {
        return ERR_IO;
    }

    // Read the metadata
    fseek(db_file->fpdb, sizeof(struct pictdb_header), SEEK_SET);
    db_file->metadata = calloc(db_file->header.max_files, sizeof(struct pict_metadata));
    if(db_file->metadata == NULL) {
        return ERR_OUT_OF_MEMORY;
    }
    int metadata_count = fread(db_file->metadata, sizeof(struct pict_metadata), db_file->header.max_files, db_file->fpdb);
    if(metadata_count != db_file->header.max_files) {
        free(db_file->metadata);
        return ERR_IO;
    }

    return 0;
}

/**
 * @brief Close a pictdb_file
 *  (reverse function of do_open provided above)
 *
 * @param db_file Pictbd_file to close
 */
void do_close(const struct pictdb_file* db_file)
{
    // Check if the pointer is defined
    if(db_file != NULL) {
        free(db_file->metadata);
        fclose(db_file->fpdb);
    }

}

/**
 * @brief convert a string resolution to a code
 *
 * @param res_name resolution name
 */
int resolution_atoi(const char* res_name)
{
    if(res_name == NULL) {
        return -1;
    }
    size_t res_length = strlen(res_name);
    if(strncmp(res_name, "thumb", res_length) == 0 || strncmp(res_name, "thumbnail", res_length) == 0) {
        return RES_THUMB;
    }

    if(strncmp(res_name, "small", res_length) == 0) {
        return RES_SMALL;
    }

    if(strncmp(res_name, "orig", res_length) == 0 || strncmp(res_name, "original", res_length) == 0) {
        return RES_ORIG;
    }
    return -1;
}
