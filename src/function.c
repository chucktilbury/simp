
#include "common.h"
#include "scanner.h"
#include "function.h"

struct _function_body_ {
    Ast ast;
    PtrLst* list;
};

// This is a list of VariableDef's
struct _func_def_params_ {
    Ast ast;
    PtrLst* list;
};

struct _function_def_ {
    Ast ast;
    TypeSpec* type;
    CompoundName* name;
    PtrLst* parms;
    FunctionBody* body;
};

/**
 * @brief Read a function body and return a pointer to it. The function body
 * can be empty and is surrounded with '{' and '}'.
 *
 * function_body_item
 *   : function_body
 *   | variable_definition
 *   | assignment
 *   | function_reference
 *   | if_statement
 *   | do_statement
 *   | while_statement
 *   | try_statement
 *   | break_statement
 *   | trace_statement
 *   | continue_statement
 *   | yield_statement
 *   | print_statement
 *   ;
 *
 * function_body_list
 *   : function_body_item
 *   | function_body_list function_body_item
 *   ;
 *
 *  function_body
 *   : '{' '}'
 *   | '{' function_body_list '}'
 *   ;
 *
 * @return void*
 */
void* function_body() {

    PTRACE;
    Token* tok = expect_token(TOK_OCBRACE);
    if(tok == NULL)
        return NULL;

    FunctionBody* ptr = _alloc_ds(FunctionBody);
    ptr->ast.type = AST_FUNC_BODY;
    ptr->list = create_ptr_list();

    consume_token();
    GET_TOKEN(tok); // current token is first one of the actual body
    if(tok->type == TOK_CCBRACE) {
        consume_token();
        return ptr; // definition is empty.
    }

    // TODO: Add the function body guts here.

    return NULL;
}

/**
 * @brief Read a list of function parameters separated by ',' characters and
 * surrounded by '()'. When this is entered, the '(' is the current token.
 *
 * function_param_list
 *   : variable_def
 *   | function_param_list ',' variable_def
 *   ;
 *
 * function_params
 *   : '(' ')'
 *   | '(' function_param_list ')'
 *   ;
 *
 * @return void*
 */
void* function_params() {

    PTRACE;

    Token* tok;
    FuncDefParams* ptr = _alloc_ds(FuncDefParams);
    ptr->ast.type = AST_FUNC_PARAMS;
    ptr->list = create_ptr_list();

    GET_TOKEN(tok);
    // first token should be a '('
    if(tok->type != TOK_OPAREN) {
        syntax("expected function parameter list but got \"%s\"", tokToStr(tok->type));
        return NULL;
    }
    consume_token();

    GET_TOKEN(tok);
    // next token is either a ')' or a variable definition
    if(tok->type == TOK_CPAREN) {
        consume_token();
        return ptr; // empty parameter list
    }

    // next token is hopefully a variable definition
    while(get_num_errors() == 0) {
        VariableDef* var = variable_param_def();
        if(var == NULL) {
            syntax("invalid function parameter definition");
            return NULL;
        }
        append_ptr_list(ptr->list, (void*)var);

        // next token can be a ',' or a ')'
        GET_TOKEN(tok);
        if(tok->type == TOK_COMMA)
            consume_token();
        else if(tok->type == TOK_CPAREN) {
            consume_token();
            return ptr;
        }
        // else repeat the loop
    }

    return NULL;
}

/**
 * @brief Read a complete function definition and return a pointer to it. When
 * this is entered, the '(' is the current token and the name and the data
 * type have already been read. The name can be a simple SYMBOL or it can be
 * a compound name. The function parameters can be simple variable definitions
 * or they be initialized with a value, but the value has to be known at
 * compile time. The body is parsed by another function.
 *
 * function_definition
 *   : type_spec compound_name parameter_spec func_body
 *   ;
 *
 * @param type
 * @param name
 * @return void*
 */
void* function_definition(TypeSpec* type, CompoundName* name) {

    PTRACE;

    FunctionDef* ptr = _alloc_ds(FunctionDef);
    ptr->ast.type = AST_FUNC_DEF;
    ptr->type = type;
    ptr->name = name;

    ptr->parms = function_params();
    if(ptr->parms == NULL)
        return NULL; // error message already issued.

    ptr->body = function_body();
    if(ptr->body == NULL)
        return NULL; // error message already issued.

    return ptr;
}

