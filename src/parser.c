#include "common.h"
#include "scanner.h"
#include "ast.h"
#include "parser.h"


/**
 * @brief This is the root of the parse tree. It returns the whole parse tree.
 *
 * @param fname
 * @return void*
 */
void* parser(const char* fname) {

    PTRACE;
    init_scanner(fname);

    PtrLst* ptr = create_ptr_list();

    Token* tok;
    while(get_num_errors() == 0) {
        tok = crnt_token();
        switch(tok->type) {
            case TOK_NAMESPACE:
                append_ptr_list(ptr, namespace_definition());
                break;
            case TOK_IMPORT:
                append_ptr_list(ptr, import_definition());
                break;
            case TOK_ERROR:
                return NULL;    // scanner has reported an error
            case TOK_END_FILE:
                consume_token();
                break;          // do nothing
            case TOK_END_INPUT:
                return ptr;     // finished return the tree
            default:
                // syntax error
                syntax("a \"%s\" is not allowed here. Expected an \"import\" or a \"namespace\"", tokToStr(tok->type));
                return NULL;
        }
    }

    // return error
    return NULL;
}


