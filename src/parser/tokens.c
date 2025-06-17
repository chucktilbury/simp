/**
 * @file tokens.c
 *
 * @brief
 *
 * @author Chuck Tilbury (chucktilbury@gmail.com)
 * @version 0.1
 * @date 2025-04-01
 * @copyright Copyright (c) 2025
 */

#include "alloc.h"
#include "tokens.h"
#include "parser.h"
#include "errors.h"
extern int yylineno;

token_t* create_token(const char* str, int type) {

    token_t* tok = _ALLOC_TYPE(token_t);
    tok->str = create_string(str);
    tok->line_no = yylineno;
    tok->type = type;

    return tok;
}

void destroy_token(token_t* tok) {

    if(tok != NULL) {
        destroy_string(tok->str);
        _FREE(tok);
    }
}

const char* tok_to_str(int type) {

    return NULL;
}
