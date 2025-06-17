/**
 * @file trace.h
 *
 * @brief
 *
 * @author Chuck Tilbury (chucktilbury@gmail.com)
 * @version 0.1
 * @date 2025-03-24
 * @copyright Copyright (c) 2025
 */
#ifndef _TRACE_H_
#define _TRACE_H_

#ifdef USE_TRACE
#include <stdio.h>
#include <stdlib.h>

// defined in trace.c
extern int trace_depth;
static int local_verbosity = 0;

#define PRINT(...) \
    do { \
        if(local_verbosity < peek_trace_verbosity()) { \
            print_indent(__VA_ARGS__); \
            fputc('\n', get_trace_handle()); \
        } \
    } while(0)

#define TRACE(...) \
    do { \
        if(local_verbosity < peek_trace_verbosity()) { \
            print_trace(__VA_ARGS__); \
        } \
    } while(0)

#define ENTER \
    do { \
        if(local_verbosity < peek_trace_verbosity()) { \
            print_enter(__FILE__, __LINE__, __func__);\
        } \
    } while(0)

#define RETURN(...) \
    do { \
        if(local_verbosity < peek_trace_verbosity()) { \
            print_return(__FILE__, __LINE__, __func__, #__VA_ARGS__); \
        } \
        return __VA_ARGS__; \
    } while(0)

#define SEPARATOR \
    do { \
        if(local_verbosity < peek_trace_verbosity()) { \
            for(int i = 0; i < 80; i++) \
                fputc('-', get_trace_handle()); \
            fputc('\n', get_trace_handle()); \
        } \
    } while(0)

#define HEADER \
    do { \
        reset_trace_depth(0); \
        SEPARATOR; \
        PRINT("\t%s", __func__); \
        SEPARATOR; \
    } while(0)

#define INIT_TRACE(fh) init_trace(fh)
#define PUSH_TRACE_VERBOSITY(n) push_trace_verbosity(n)
#define POP_TRACE_VERBOSITY() pop_trace_verbosity()
#define PEEK_TRACE_VERBOSITY() peek_trace_verbosity()
#define LOCAL_VERBOSITY(n) local_verbosity = (n)

void init_trace(FILE* fp);
void reset_trace_depth(int val);
void push_trace_verbosity(int num);
void pop_trace_verbosity(void);
int peek_trace_verbosity(void);
FILE* get_trace_handle(void);
void print_indent(const char* fmt, ...);
void print_trace(const char* fmt, ...);
void print_enter(const char* file, int line, const char* func);
void print_return(const char* file, int line, const char* func, const char* str);

#else

#define TRACE(...)
#define ENTER
#define RETURN(...)         \
    do {                    \
        return __VA_ARGS__; \
    } while(0)
#define SEPARATOR
#define INIT_TRACE(fh)
#define PUSH_TRACE_VERBOSITY(n)
#define POP_TRACE_VERBOSITY()
#define PEEK_TRACE_VERBOSITY()
#define PRINT_INDENT(...)
#define PRINT_TRACE(...)
#define LOCAL_VERBOSITY(n)
#define HEADER

#endif


#endif /* _TRACE_H_ */
