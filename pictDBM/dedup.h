/**
 * @file dedup.h
 * @brief prototypes and includes for dedup.c
 *
 * @author Basile Thullen, Jeremy Hottinger
 * @date 12 May 2016
 */

#ifndef DEDUP_H
#define DEDUP_H

#include "pictDB.h"

/**
 * @brief check for duplicate of an image in the database file
 *        If there is a duplicate, make the fields of the image point to the
 *        fields of the image already in database
 *
 * @param database file in which to search
 * @param index of the image to check for duplicate
 */
int do_name_and_content_dedup(struct pictdb_file* db_file, uint32_t index);

#endif
