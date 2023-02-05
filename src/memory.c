
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

void* mem_malloc(size_t size) {

    void* ptr = malloc(size);
    if(ptr == NULL) {
        fprintf(stderr, "Cannot allocate %lu bytes of memory!\n", size);
        exit(1);
    }
    memset(ptr, 0, size);

    return ptr;
}

void* mem_realloc(void* ptr, size_t size) {

    void* nptr = realloc(ptr, size);
    if(nptr == NULL) {
        fprintf(stderr, "Cannot reallocate %lu bytes of memory!\n", size);
        exit(1);
    }

    return nptr;
}

void* mem_copy(void* ptr, size_t size) {

    void* nptr = mem_malloc(size);
    memcpy(nptr, ptr, size);

    return nptr;
}

void mem_free(void* ptr) {

    if(ptr != NULL)
        free(ptr);
}
