/**
 * @file pictDB.h
 * @brief Main header file for pictDB core library.
 *
 * Defines the format of the data structures that will be stored on the disk
 * and provides interface functions.
 *
 * The picture database starts with exactly one header structure
 * followed by exactly pictdb_header.max_files metadata
 * structures. The actual content is not defined by these structures
 * because it should be stored as raw bytes appended at the end of the
 * database file and addressed by offsets in the metadata structure.
 *
 * @author Basile Thullen - Jeremy Hottinger
 * @date 13 Apr 2016
 */

#ifndef PICTDBPRJ_PICTDB_H
#define PICTDBPRJ_PICTDB_H

#include "error.h" /* not needed here, but provides it as required by
                    * all functions of this lib.
                    */
#include <stdio.h> // for FILE
#include <stdint.h> // for uint32_t, uint64_t
#include <openssl/sha.h> // for SHA256_DIGEST_LENGTH
#include <string.h> // for strcmp
#include <stdlib.h> // for malloc
#include <vips/vips.h>
#include "pictDBM_tools.h"

#define CAT_TXT "EPFL PictDB binary"

/* constraints */
#define MAX_DB_NAME 31  // max. size of a PictDB name
#define MAX_PIC_ID 127  // max. size of a picture id
#define MAX_MAX_FILES 100000  // will be increased later in the project
#define MAX_THUMB_RES 128
#define MAX_SMALL_RES 512

/* default values for databse creation */
#define DEFAULT_MAX_FILES 10
#define DEFAULT_THUMB_RES 64
#define DEFAULT_SMALL_RES 256

/* For is_valid in pictdb_metadata */
#define EMPTY 0
#define NON_EMPTY 1

// pictDB library internal codes for different picture resolutions.
#define RES_THUMB 0
#define RES_SMALL 1
#define RES_ORIG  2
#define NB_RES    3

//number of available commands
#define NB_CMD 7

#ifdef __cplusplus
extern "C" {
#endif

/* Define the header of a database file */
struct pictdb_header {
    char db_name[MAX_DB_NAME + 1];
    uint32_t db_version;
    uint32_t num_files;
    uint32_t max_files;
    uint16_t res_resized[2 * (NB_RES - 1)];
    uint32_t unused_32;
    uint64_t unused_64;
};

/* Define the metadata of an image contained in the database file */
struct pict_metadata {
    char pict_id[MAX_PIC_ID + 1];
    unsigned char SHA[SHA256_DIGEST_LENGTH];
    uint32_t res_orig[2];
    uint32_t size[NB_RES];
    uint64_t offset[NB_RES];
    uint16_t is_valid;
    uint16_t unused_16;
};

/* Represent a database file */
struct pictdb_file {
    FILE* fpdb;
    struct pictdb_header header;
    struct pict_metadata* metadata;
};

/* Define modes for do list */
typedef enum {
    STDOUT, JSON
} do_list_mode;

/**
 * @brief Prints database header informations.
 *
 * @param header The header to be displayed.
 */
void print_header(const struct pictdb_header* header);

/**
 @brief Prints picture metadata informations.
 *
 * @param metadata The metadata of one picture.
 */
void print_metadata (const struct pict_metadata* metadata);

/**
 * @brief Displays (on stdout) pictDB metadata.
 *
 * @param db_file In memory structure with header and metadata.
 */

char* do_list(const struct pictdb_file* file, do_list_mode mode);

/**
 * @brief Creates the database called db_filename. Writes the header and the
 *        preallocated empty metadata array to database file.
 *
 * @param db_file In memory structure with header and metadata.
 */
int do_create(const char* filename, struct pictdb_file* db_file);

/**
 * @brief Delete an image from a database file
 *
 * @param name Name of the image to delete
 * @param file File from which the image must be deleted
 */
int do_delete(const char* name, struct pictdb_file* file);

/**
 * @brief Open a database file from the disk
 *
 * @param filename Name of the database file on disk
 * @param mode mode in which the file will be open
 * @param file File to store the opened database
 */
int do_open(const char* filename, const char* mode, struct pictdb_file* file);

/**
 * @brief Close a database file
 *
 * @param file File to close
 */
void do_close(const struct pictdb_file* file);

/**
 * @brief convert a string into a resolution code
 *
 * @param resolution_name string to convert
 */
int resolution_atoi(const char* resolution_name);

/**
 * @brief read an image from a database file
 *
 * @param img_id id of the image to read
 * @param res_code resolution code in which to read the image
 * @param img_array the array into which the image will be stored
 * @param size the size of the image
 * @param db_file the database file in which to read the image
 */
int do_read(const char* img_id, int res_code, char** img_array, uint32_t* size, struct pictdb_file* db_file);

/**
 * @brief insert an image in the database
 *
 * @param img_array the array containing the image to insert
 * @param img_size the size of the image
 * @param img_id new id for image
 * @param db_file file in which to insert the image
 */
int do_insert(const char* img_array, size_t img_size, const char* img_id, struct pictdb_file* db_file);

int do_gbcollect(struct pictdb_file* db_file, char* orig_filename, char* new_filename);

#ifdef __cplusplus
}
#endif
#endif
