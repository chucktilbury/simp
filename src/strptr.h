
#ifndef _STRPTR_H
#define _STRPTR_H

typedef void* String;

String create_string(const char* str, ...);
void append_string_str(String ptr, const char* str, ...);
void append_string_char(String str, int ch);
const char* get_string_ptr(String str);
int get_string_len(String str);
void clear_string(String str);

#endif
