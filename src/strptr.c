
/* a little bit stubby */
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdarg.h>

#include "memory.h"
#include "strptr.h"

typedef struct {
    char* buf;
    int len;
    int cap;
} _string;

static void append(_string* ptr, void* buf, size_t size) {

    if(ptr->len+size > ptr->cap) {
        while(ptr->len+size > ptr->cap)
            ptr->cap <<= 1;
        ptr->buf = _realloc_ds_array(ptr->buf, char, ptr->cap);
    }

    memcpy(&ptr->buf[ptr->len], buf, size);
    ptr->len += size;
}

String create_string(const char* str, ...) {

    va_list ap;

    _string* ptr = _alloc_ds(_string);
    ptr->len = 0;
    ptr->cap = 1 << 3;
    ptr->buf = _alloc_ds_array(char, ptr->cap);

    va_start(ap, str);
    int len = vsnprintf(NULL, 0, str, ap)+1;
    va_end(ap);

    char* buf = _alloc(len);

    if(str != NULL) {
        va_start(ap, str);
        vsnprintf(buf, len, str, ap);
        va_end(ap);

        append(ptr, (void*)buf, strlen(buf));
    }

    return (String)ptr;
}

void append_string_str(String ptr, const char* str, ...) {

    va_list ap;

    va_start(ap, str);
    int len = vsnprintf(NULL, 0, str, ap)+1;
    va_end(ap);

    char* buf = _alloc(len);

    va_start(ap, str);
    vsnprintf(buf, len, str, ap);
    va_end(ap);

    append(ptr, (void*)buf, strlen(buf));
}

void append_string_char(String ptr, int ch) {

    append(ptr, &ch, 1);
}

const char* get_string_ptr(String str) {

    _string* ptr = (_string*)str;

    ptr->buf[ptr->len] = 0;
    return ptr->buf;
}

int get_string_len(String ptr) {

    return ((_string*)ptr)->len;
}

void clear_string(String ptr) {

    ((_string*)ptr)->len = 0;
}

