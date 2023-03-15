/**
 * @file reference.c
 *
 * @brief This parser module implements references to objects.
 *
 */
#include "common.h"
#include "scanner.h"
#include "ast.h"
#include "reference.h"

struct _compound_name_ {
    Ast ast;
    PtrLst* symbol;
    String str;
    bool is_single;
};

struct _array_reference_ {
    Ast ast;
    String name;
    Expression* expr;
};

struct _function_reference_ {
    Ast ast;
    String name;
    ExpressionList* params;
};

struct _compound_reference_element_ {
    Ast ast;
    void* element; // type is given by the element's AST.
};

struct _compound_reference_ {
    Ast ast;
    PtrLst* list;
};

/**
 * @brief A compound name can be a simple symbol or a list of symbols
 * separated by '.'. When this is entered, the current token is a SYMBOL.
 *
 * compound_name
 *   : SYMBOL
 *   | compound_name '.' SYMBOL
 *   ;
 *
 * @return void*
 */
void* compound_name() {

    PTRACE;
    CompoundName* ptr = _alloc_ds(CompoundName);
    ptr->ast.type = AST_COMPOUND_NAME;
    ptr->symbol = create_ptr_list();
    ptr->str = create_string(NULL);
    ptr->is_single = true;

    Token* tok;
    while(get_num_errors() == 0) {
        GET_TOKEN(tok);
        if(tok->type == TOK_SYMBOL) {
            append_ptr_list(ptr->symbol, tok->str);
            append_string_str(ptr->str, get_string_ptr(tok->str));
        }
        else {
            // syntax error
            syntax("expected a symbol but got a \"%s\"", tokToStr(tok->type));
            return NULL;
        }
        consume_token();

        tok = crnt_token(); // EOF is okay here
        if(tok->type == TOK_DOT) {
            append_string_str(ptr->str, ".");
            ptr->is_single = false;
        }
        else {
            // finished...
            DBG_MSG("compound_name: %s (%s)",
                    get_string_ptr(ptr->str), ptr->is_single? "true": "false");
            return ptr;
        }
        consume_token();
    }

    return NULL;
}

/**
 * @brief Parse an array reference.
 *
 * array_reference
 *    : SYMBOL '[' expression ']'
 *    ;
 *
 * @return void*
 */
void* array_reference() {


    return NULL;
}


/**
 * @brief Parse a reference to a function.
 *
 * function_reference
 *   : SYMBOL '(' expression_list ')'
 *   | SYMBOL '(' ')'
 *   ;
 *
 * @return void*
 */
void* function_reference() {

    return NULL;
}

/**
 * @brief Recognize a compound reference element.
 *
 * compound_reference_element
 *   : SYMBOL
 *   | SYMBOL '[' expression ']'
 *   | SYMBOL '(' expression_list ')'
 *   | SYMBOL '(' ')'
 *   ;
 *
 * @return void*
 */
void* compound_reference_element() {

    return NULL;
}

/**
 * @brief A compound reference is a reference that is joined by '.'.
 *
 * compound_reference
 *   : compound_reference_element
 *   | compound_reference '.' compound_reference_element
 *   ;
 *
 * @return void*
 */
void* compound_reference() {

    return NULL;
}

