/**
 * @file db_list.c
 * @brief pictDB library: do_list implementation.
 *
 * @author Basile Thullen, Jeremy Hottinger
 * @date 13 Apr 2016
 */

#include "pictDB.h"
#include <json-c/json.h>

/**
 * @brief List the images contained in a pictdb_file
 *
 * @param db_file file to list the images from
 */
char* do_list(const struct pictdb_file* db_file, do_list_mode mode)
{
    // Check for good parameter
    if(db_file != NULL) {

        if(mode == STDOUT) {
            print_header(&(db_file->header));

            // check for the special case where there is no images
            if(db_file->header.num_files == 0) {
                printf("<< empty database >>\n");
            } else {
                int size = db_file->header.max_files;
                int i;
                // list the files contained in the database by printing their metadata
                for(i = 0; i < size; i ++) {
                    if(db_file->metadata[i].is_valid == NON_EMPTY) {
                        print_metadata(&(db_file->metadata[i]));
                    }
                }
            }
            return NULL;
        } else if(mode == JSON) {
            // create container json object
            struct json_object* main_obj = json_object_new_object();
            if(main_obj == NULL) {
                return NULL;
            }

            // create container array for strings
            struct json_object* main_array = json_object_new_array();
            if(main_array == NULL) {
                return NULL;
            }

            struct json_object* pict_id_string = NULL;
            int i = 0;
            // pass through all metadatas
            for(i = 0; i<db_file->header.max_files; i++) {
                if(db_file->metadata[i].is_valid == NON_EMPTY) {
                    // create a json string with the pict_id
                    pict_id_string = json_object_new_string(db_file->metadata[i].pict_id);
                    if(pict_id_string != NULL) {
                        // add the string to the array
                        json_object_array_add(main_array, pict_id_string);
                    }
                }
            }

            // add the array to the object
            json_object_object_add(main_obj, "Pictures", main_array);
            const char* res = json_object_to_json_string(main_obj);
            char* res_cpy = calloc(strlen(res)+1, sizeof(char));
            if(res_cpy == NULL) {
                json_object_put(main_obj);
                return NULL;
            }
            strncpy(res_cpy, res, strlen(res));
            json_object_put(main_obj);
            return res_cpy;
        } else {
            return "unimplemented do_list mode";
        }

        return NULL;
    }
    return NULL;
}
