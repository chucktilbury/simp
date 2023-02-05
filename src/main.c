
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "scanner.h"
#include "parser.h"

int errors = 0;

void syntax_error(const char* fmt, ...) {

    if(get_line_no() > 0)
        fprintf(stderr, "%s:%d:%d syntax error, ",
                get_file_name(), get_line_no(), get_col_no());
    else
        fprintf(stderr, "syntax error, ");

    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    errors++;
}

void __ferror(const char* func, int line, const char* fmt, ...) {

    fprintf(stderr, "%s:%d: fatal error, ", func, line);

    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    errors++;
    exit(1);
}

/**
 * @file main.c
 *
 * @brief Main entry point to the program.
 *
 */
int main(int argc, char** argv) {

    if(argc < 2) {
        fprintf(stderr, "require file name\n");
        return 1;
    }

    open_file(argv[1]);

    if(simp_parse()) {
        printf("parse fail: %d error(s)\n", errors);
        return 1;
    }

    return 0;
}
