/**
 * @file array.c
 *
 * Implement an array of data structures in contiguous memory.
 *
 */
#include <string.h>

#include "alloc.h"
#include "array.h"

typedef struct {
    unsigned char* buffer; // array of bytes
    size_t capacity; // capacity of array in items
    size_t length; // length of the array in items
    size_t size; // size of each item
} _array_t_;


array_t create_array(size_t size) {

    _array_t_* ptr = _ALLOC_TYPE(_array_t_);
    ptr->size = size;
    ptr->capacity = 1 << 3;
    ptr->length = 0;
    ptr->buffer = _ALLOC(ptr->capacity*size);

    return ptr;
}

void destroy_array(array_t h) {

    _array_t_* ptr = (_array_t_*)h;

    if(ptr != NULL) {
        _FREE(ptr->buffer);
        _FREE(ptr);
    }
}

void append_array(array_t h, void* data) {

    _array_t_* ptr = (_array_t_*)h;

    if(ptr->length > ptr->capacity) {
        ptr->capacity <<= 1;
        ptr->buffer = _REALLOC(ptr->buffer, ptr->capacity * ptr->size);
    }

    memcpy(&ptr->buffer[ptr->length*ptr->size], data, ptr->size);
    ptr->length++;
}

void* index_array(array_t h, int index) {

    _array_t_* ptr = (_array_t_*)h;

    if(index >= 0 && ((size_t)(index) < ptr->length))
        return (void*)&ptr->buffer[ptr->length*ptr->size];
    else
        return NULL;

}

void* iterate_array(array_t h, int* post) {

    _array_t_* ptr = (_array_t_*)h;

    if((size_t)(*post) < ptr->length) {
        void* retv = (void*)&ptr->buffer[ptr->length*ptr->size];
        *post = *post + 1;
        return retv;
    }
    else
        return NULL;
}

// zero the length but don't change the capacity
void clear_array(array_t h) {

    ((_array_t_*)h)->length = 0;
}


void* raw_array(array_t h) {

    return (void*)((_array_t_*)h)->buffer;
}

size_t len_array(array_t h) {

    return ((_array_t_*)h)->length;
}

