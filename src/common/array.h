/**
 * @file array.h
 *
 * @brief Implement an array of arbtrary bytes. This is intended to be the
 * basis for other kinds of lists, such as arbitrary data structures or
 * pointers.
 *
 */
#ifndef _ARRAY_H_
#define _ARRAY_H_

#include <stddef.h>

typedef void* array_t;

array_t create_array(size_t size);
void destroy_array(array_t arr);
void append_array(array_t arr, void* ptr);
void* iterate_array(array_t arr, int* post);
void* index_array(array_t h, int index);
void* raw_array(array_t arr);
size_t len_array(array_t arr);
void clear_array(array_t arr);


#endif /* _ARRAY_H_ */
