#ifndef _MEMORY_H_
#define _MEMORY_H_

#include <string.h>

#define _init_memory()
#define _uninit_memory()
#define _alloc(s) mem_malloc(s)
#define _alloc_ds(t) (t*)mem_malloc(sizeof(t))
#define _alloc_ds_array(t, n) (t*)mem_malloc(sizeof(t) * (n))
#define _realloc(p, s) mem_realloc((p), (s))
#define _realloc_ds_array(p, t, n) (t*)mem_realloc((p), sizeof(t) * (n))
#define _copy_str(s) mem_copy((void*)(s), strlen(s) + 1)
#define _copy_data(p, s) mem_copy((void*)(p), (s))
#define _free(p) mem_free((void*)(p))

#include <stddef.h>

void* mem_malloc(size_t size);
void* mem_realloc(void* ptr, size_t size);
void* mem_copy(void* ptr, size_t size);
void mem_free(void* ptr);

#endif
