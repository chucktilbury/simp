
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include "scanner.h"
#include "filebuf.h"

static int errors = 0;
static int warnings = 0;

/************************************************
 * Errors API
 */
void syntax(const char* fmt, ...) {

    if(get_line_no() > 0)
        fprintf(stderr, "Syntax Error: %s:%d:%d: ",
                get_file_name(), get_line_no(), get_col_no());
    else
        fprintf(stderr, "Syntax Error: ");

    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

// TODO: Synchronize the parser.....

    errors++;
}


void warning(const char* fmt, ...) {

    if(get_line_no() > 0)
        fprintf(stderr, "Warning: %s:%d:%d: ",
                get_file_name(), get_line_no(), get_col_no());
    else
        fprintf(stderr, "Warning: ");

    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    warnings++;
}

void __fatal(const char* func, int line, const char* fmt, ...) {

    fprintf(stderr, "Fatal Error: %s:%d: ", func, line);

    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    errors++;
    exit(1);
}

int get_num_errors() {

    return errors;
}

int get_num_warnings() {

    return warnings;
}
