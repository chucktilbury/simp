
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "ptrlst.h"
#include "strptr.h"
#include "memory.h"
#include "scanner.h"

#include "filebuf.h"
#include "parser.h"

#include "errors.h"

#include "scanner.h"


void print_token(Token* tok) {

    if(tok != NULL) {
        printf("token name: \"%s\"\n", get_string_ptr(tok->str));
        printf("      type: %s (%d)\n", tokToStr(tok->type), tok->type);
        printf("      %s:%d:%d\n", get_file_name(), get_line_no(), get_col_no());
    }
    else
        printf("NULL token\n");
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

    void* data = parser(argv[1]);

    // init_scanner(argv[1]);
    // //open_file(argv[1]);
    //
    // //Module* mod = parse_all(argv[1]);
    // Token* tok;
    // while(1) {
    //     tok = crnt_token();
    //     print_token(tok);
    //     consume_token();
    //     if(tok->type == TOK_END_INPUT) {
    //         print_token(tok);
    //         break;
    //     }
    // }
    //
    //if(mod == NULL || get_num_errors() != 0) {
    //    printf("parse fail: %d error(s)\n", get_num_errors());
    //    return 1;
    //}

    return 0;
}
