/**
 * @file reference.c
 *
 * @brief This parser module implements references to objects.
 *
 */
#include "common.h"
#include "parser.h"

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
};

struct _compound_reference_ {
    Ast ast;
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
 *    : SYMBOL '[' expression ']' {TRACE("array_reference:");}
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
 *   : SYMBOL '(' expression_list ')' {TRACE("function_reference:");}
 *   | SYMBOL '(' ')' {TRACE("function_reference:");}
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
 *   : SYMBOL {TRACE("compound_element:SYMBOL");}
 *   | array_reference {TRACE("compound_element:array_reference");}
 *   | function_reference {}
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
 *   : compound_refrence_element {TRACE("compound_name:compound_element");}
 *   | compound_reference '.' compound_refrence_element {TRACE("compound_name:add");}
 *   ;
 *
 * @return void*
 */
void* compound_reference() {

    return NULL;
}