/**
 * @file image_content.h
 * @brief prototypes for image_content.c
 *
 * @author Basile Thullen, Jeremy Hottinger
 * @date 12 May 2016
 */

#ifndef IMAGE_CONTENT_H
#define IMAGE_CONTENT_H

#include "pictDB.h"

/**
 * @brief resize an image given by its id to a specified resolution
 *
 * @param res_code resolution code at which to resize the image
 * @param file database file in which we find the image
 * @param image_id image id to resize
 */
int lazily_resize(int res_code, struct pictdb_file* file, size_t image_id);

/**
 * @brief get the resolution of an image in memory
 *
 * @param height pointer to an unsigned int to return the image height
 * @param width pointer to an unsigned int to return the image width
 * @param image_buffer memory location in which the image is stored
 * @param image_size the size of the image in memory
 */
int get_resolution(uint32_t* height, uint32_t* width, const char* image_buffer, size_t image_size);

#endif
