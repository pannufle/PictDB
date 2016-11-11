/**
 * @file pictDBM.c
 * @brief pictDB Manager: command line interpretor for pictDB core commands.
 *
 * Picture Database Management Tool
 *
 * @author Basile Thullen - Jeremy Hottinger
 * @date 4 Apr 2016
 */

#include "pictDB.h"
#include "image_content.h"

#include <stdlib.h>
#include <string.h>

// function prototypes
int do_list_cmd(int args, char *argv[]);
int do_create_cmd(int args, char *argv[]);
int do_delete_cmd(int args, char *argv[]);
int help(int args, char *argv[]);

int read_disk_image(char** img_array, size_t* size, const char* filename);

typedef int (*command)(int, char**);
typedef struct command_mapping {
    char* name;
    command function;
} command_mapping;

/**
 * @brief Opens pictDB file and calls do_list command.
 */
int
do_list_cmd(int args, char *argv[])
{
    // Check for good parameter
    if(args < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    char* filename = argv[1];

    if((filename == NULL) || (filename[0] == '\0') || (strlen(filename) > MAX_DB_NAME)) {
        return ERR_INVALID_ARGUMENT;
    }

    struct pictdb_file file;

    // Check for opening problems
    if(do_open(filename, "rb+", &file) != 0) {
        return ERR_IO;
    }

    do_list(&file, STDOUT);

    do_close(&file);
    return 0;
}

/**
 * @brief create a new name from a pict_id plus a resolution code
 */
char* create_name(char* pict_id, int res_code)
{
    // create array of suffixes for each resolution
    char* suffix = NULL;
    const char* extension = ".jpg";

    switch(res_code) {
    case 0 :
        suffix = calloc(6, sizeof(char));
        strncpy(suffix, "_thumb", 6);
        break;
    case 1:
        suffix = calloc(6, sizeof(char));
        strncpy(suffix, "_small", 6);
        break;
    case 2:
        suffix = calloc(5, sizeof(char));
        strncpy(suffix, "_orig", 5);
        break;
    default:
        return NULL;
    }

    // compute lengths of id and suffixes
    size_t id_length = strlen(pict_id);
    size_t suffix_length = strlen(suffix);
    size_t ext_length = strlen(extension);

    // allocate a new memory location for the new name
    char* name = calloc(id_length + suffix_length + ext_length, sizeof(char));
    if(name == NULL) {
        free(suffix);
        return NULL;
    }

    // first copy the id
    strncpy(name, pict_id, id_length);
    // then the suffix
    strncat(name, suffix, suffix_length);
    // then the extension
    strncat(name, extension, ext_length);

    free(suffix);

    return name;
}

/**
 * @brief command to insert an image in the database
 */
int do_insert_cmd(int args, char *argv[])
{

    if(args < 4) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    // arguments parsing
    char* db_filename = argv[1];
    if(db_filename == NULL || db_filename[0] == '\0' || strlen(db_filename) > MAX_DB_NAME) {
        return ERR_INVALID_ARGUMENT;
    }
    char* pict_id = argv[2];
    if(pict_id == NULL || pict_id[0] == '\0' || strlen(pict_id) > MAX_PIC_ID) {
        return ERR_INVALID_ARGUMENT;
    }
    char* filename = argv[3];
    if(filename == NULL || filename[0] == '\0') {
        return ERR_INVALID_ARGUMENT;
    }

    struct pictdb_file file;

    // Check for opening problems
    if(do_open(db_filename, "rb+", &file) != 0) {
        return ERR_IO;
    }

    char* img_array = NULL;
    size_t size = 0;
    int res = read_disk_image(&img_array, &size, filename);
    if(res != 0) {
        do_close(&file);
        return res;
    }

    res = do_insert(img_array, size, pict_id, &file);
    if(res != 0) {
        free(img_array);
        do_close(&file);
        return res;
    }
    free(img_array);
    return 0;
}

/**
 * @brief reads an image on the disk given the filename and affect
 *        parameters img_array and size with the content of the file and
 *        the size respectively
 */
int read_disk_image(char** img_array, size_t* size, const char* filename)
{
    FILE* fp_img = fopen(filename, "r");
    if(fp_img == NULL) {
        return ERR_IO;
    }

    // compute the size of the image on disk
    fseek(fp_img, 0, SEEK_END);
    long end_pos = ftell(fp_img);
    if(end_pos <= 0) {
        fclose(fp_img);
        return ERR_IO;
    }
    size_t diff = end_pos;

    fseek(fp_img, 0, SEEK_SET);
    // allocate necessary memory to read the image
    char* buffer = calloc(diff, sizeof(char));
    if(buffer == NULL) {
        fclose(fp_img);
        return ERR_OUT_OF_MEMORY;
    }
    if(fread(buffer, diff, 1, fp_img) != 1) {
        free(buffer);
        fclose(fp_img);
        return ERR_IO;
    }

    *img_array = buffer;
    *size = diff;

    fclose(fp_img);
    return 0;
}
/**
 * @brief writes the given image to the disk with the given filename. The image is passed as a pointer to an array of bytes.
 */
int write_disk_image(const char* to_write, size_t size, char* filename)
{

    FILE* file = fopen(filename, "w+");
    if(file == NULL) {
        return ERR_IO;
    }

    if(fwrite(to_write, size, 1, file) != 1) {
        fclose(file);
        return ERR_IO;
    }

    fclose(file);
    return 0;
}

/**
 * @brief reads and return an image from the database
 */
int do_read_cmd(int args, char *argv[])
{
    if(args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    char* db_filename = argv[1];
    char* pict_id = argv[2];
    int res_code = 2;

    if(args >= 4) {
        char* res_name = argv[3];
        if((res_code = resolution_atoi(res_name)) == -1) {
            return ERR_INVALID_ARGUMENT;
        }
    }

    struct pictdb_file file;

    // Check for opening problems
    if(do_open(db_filename, "rb+", &file) != 0) {
        return ERR_IO;
    }

    char* img_array = NULL;
    uint32_t size = 0;
    int res = do_read(pict_id, res_code, &img_array, &size, &file);
    if(res != 0) {
        do_close(&file);
        return res;
    }

    char* new_name = create_name(pict_id, res_code);
    if(new_name == NULL) {
        free(img_array);
        do_close(&file);
        return ERR_IO;
    }
    res = write_disk_image(img_array, size, new_name);
    if(res != 0) {
        free(img_array);
        free(new_name);
        do_close(&file);
        return res;
    }

    free(img_array);
    free(new_name);
    return 0;
}

/**
 * @brief Prepares and calls do_create command.
 */
int
do_create_cmd (int args, char *argv[])
{
    // Check for good parameter
    if(args < 2) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    char* filename = argv[1];

    if((filename == NULL) || (filename[0] == '\0')) {
        return ERR_INVALID_ARGUMENT;
    }

    // Default values
    uint32_t max_files = DEFAULT_MAX_FILES;
    uint16_t thumb_res_x = DEFAULT_THUMB_RES;
    uint16_t thumb_res_y =  DEFAULT_THUMB_RES;
    uint16_t small_res_x = DEFAULT_SMALL_RES;
    uint16_t small_res_y = DEFAULT_SMALL_RES;

    if(args > 2) {
        int i = 2;
        while(i < args) {
            if(!strcmp(argv[i], "-max_files")) {
                if(args - 1 - i < 1) {
                    return ERR_NOT_ENOUGH_ARGUMENTS;
                }
                uint32_t res = atouint32(argv[i+1]);
                if(res == 0 || res > MAX_MAX_FILES) {
                    return ERR_MAX_FILES;
                }
                max_files = res;
                i += 2;
            } else if(!strcmp(argv[i], "-thumb_res")) {
                if(args - 1 - i < 2) {
                    return ERR_NOT_ENOUGH_ARGUMENTS;
                }
                uint16_t resx = atouint16(argv[i+1]);
                uint16_t resy = atouint16(argv[i+2]);
                if(resx == 0 || resy == 0 || resx > MAX_THUMB_RES || resy > MAX_THUMB_RES) {
                    return ERR_RESOLUTIONS;
                }
                thumb_res_x = resx;
                thumb_res_y = resy;
                i += 3;
            } else if(!strcmp(argv[i], "-small_res")) {
                if(args - 1 - i < 2) {
                    return ERR_NOT_ENOUGH_ARGUMENTS;
                }
                uint16_t resx = atouint16(argv[i+1]);
                uint16_t resy = atouint16(argv[i+2]);
                if(resx == 0 || resy == 0 || resx > MAX_SMALL_RES || resy > MAX_SMALL_RES) {
                    return ERR_RESOLUTIONS;
                }
                small_res_x = resx;
                small_res_y = resy;
                i += 3;
            } else {
                return ERR_INVALID_ARGUMENT;
            }
        }
    }

    puts("Create");
    // set known fields
    struct pictdb_file db_file;
    db_file.header.max_files = max_files;
    db_file.header.res_resized[0] = thumb_res_x;
    db_file.header.res_resized[1] = thumb_res_y;
    db_file.header.res_resized[2] = small_res_x;
    db_file.header.res_resized[3] = small_res_y;

    // create the file
    int res = do_create(filename, &db_file);
    fclose(db_file.fpdb);

    if(res == 0) {
        print_header(&(db_file.header));
    }

    return res;
}

int do_gc_cmd(int args, char* argv[])
{

    if(argv[1] == NULL || argv[2] == NULL) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }
    if(strlen(argv[1]) > MAX_DB_NAME || strlen(argv[2]) > MAX_DB_NAME) {
        return ERR_INVALID_ARGUMENT;
    }

    char* old_db_file_name = argv[1];
    char* tmp_file_name = argv[2];

    struct pictdb_file old_db_file;
    int res = 0;
    if((res = do_open(old_db_file_name, "rb+", &old_db_file)) != 0) {
        return res;
    }
    if((res = do_gbcollect(&old_db_file, old_db_file_name, tmp_file_name)) != 0) {
        return res;
    }

    return 0;
}

/**
 * @brief Displays some explanations.
 */
int
help (int args, char *argv[])
{
    printf("pictDBM [COMMAND] [ARGUMENTS]\n");
    printf("  help: displays this help.\n");
    printf("  list <dbfilename>: list pictDB content.\n");
    printf("  create <dbfilename>: create a new pictDB.\n");
    printf("      options are:\n");
    printf("          -max_files <MAX_FILES>: maximum number of files.\n");
    printf("                                  default value is 10\n");
    printf("                                  maximum value is 100000\n");
    printf("          -thumb_res <X_RES> <Y_RES>: resolution for thumbnail images.\n");
    printf("                                  default value is 64x64\n");
    printf("                                  maximum value is 128x128\n");
    printf("          -small_res <X_RES> <Y_RES>: resolution for small images.\n");
    printf("                                  default value is 256x256\n");
    printf("                                  maximum value is 512x512\n");
    printf("  read   <dbfilename> <pictID> [original|orig|thumbnail|thumb|small]:\n");
    printf("      read an image from the pictDB and save it to a file.\n");
    printf("      default resolution is \"original\".\n");
    printf("  insert <dbfilename> <pictID> <filename>: insert a new image in the pictDB.\n");
    printf("  delete <dbfilename> <pictID>: delete picture pictID from pictDB.\n");
    printf("  gc <dbfilename> <tmp dbfilename>: performs garbage collecting on pictDB. Requires a temporary filename for copying the pictDB.\n");
    return 0;
}

/**
 * @brief Deletes a picture from the database.
 */
int
do_delete_cmd(int args, char *argv[])
{
    // Check for good parameter
    if(args < 3) {
        return ERR_NOT_ENOUGH_ARGUMENTS;
    }

    char* filename = argv[1];
    char* pictID   = argv[2];

    if((filename == NULL) || (filename[0] == '\0')) {
        return ERR_INVALID_ARGUMENT;
    }
    if(strlen(pictID) > MAX_PIC_ID) {
        return ERR_INVALID_PICID;
    }

    struct pictdb_file file;

    // Open the file
    if(do_open(filename, "rb+", &file) != 0) {
        return ERR_IO;
    }

    // delete the image
    int res = do_delete(pictID, &file);
    if(res != 0) {
        do_close(&file);
        return res;
    }

    // close the file
    do_close(&file);

    return 0;
}

/**
 * @brief Entry point
 */
int main (int argc, char* argv[])
{
    int ret = 0;

    if (VIPS_INIT(argv[0])) {
        vips_error_exit("unable to start VIPS");
    }

    if (argc < 2) {
        ret = ERR_NOT_ENOUGH_ARGUMENTS;
    } else {

        command_mapping commands[NB_CMD] = {{"list", do_list_cmd},
            {"create", do_create_cmd},
            {"delete", do_delete_cmd},
            {"help", help},
            {"insert", do_insert_cmd},
            {"read", do_read_cmd},
            {"gc", do_gc_cmd}
        };

        argc--;
        argv++; // skips command call name

        int found = 0;
        size_t i = 0;

        while(!found && i < NB_CMD) {
            if(!strcmp(commands[i].name, argv[0])) {
                found = 1;
                ret = commands[i].function(argc, argv);
            }
            i ++;
        }

        if(!found) {
            ret = ERR_INVALID_COMMAND;
        }
    }

    if (ret) {
        fprintf(stderr, "ERROR: %s\n", ERROR_MESSAGES[ret]);
        help(argc, argv);
    }
    vips_shutdown();
    return ret;
}
