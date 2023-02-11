#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#include "errors.h"
#include "ptrlst.h"
#include "memory.h"
#include "scanner.h"
#include "strptr.h"
#include "parser.h"
#include "ast.h"

/**
 * @brief Store a scope operator.
 *
 * scope_operator
 *   : TOK_PUBLIC
 *   | TOK_PRIVATE
 *   | TOK_PROTECTED
 *   ;
 *
 * @return void*
 */
void* scope_operator() {

    PTRACE;

    ScopeOperator* ptr = _alloc_ds(ScopeOperator);
    ptr->ast.type = AST_SCOPE_OPERATOR;
    Token* tok = crnt_token();
    ptr->type = tok->type;
    consume_token();

    return ptr;
}

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
 * @brief Parse a type spec.
 *
 * type_name
 *   : INTEGER
 *   | UNSIGNED
 *   | FLOAT
 *   | STRING
 *   | BOOLEAN
 *   | LIST
 *   | HASH
 *   | compound_name
 *   ;
 *
 * type_spec
 *   : type_name
 *   | CONST type_name
 *   ;
 *
 * @return void*
 */
void* type_spec() {

    PTRACE;
    CHECK_EOF();

    Token* tok;
    TypeSpec* ptr = _alloc_ds(TypeSpec);
    ptr->ast.type = AST_TYPE_SPEC;
    ptr->symbol = NULL;
    ptr->is_const = false;

    while(get_num_errors() == 0) {
        GET_TOKEN(tok);
        switch(tok->type) {
            case TOK_INT:
            case TOK_UINT:
            case TOK_NOTHING:
            case TOK_STRING:
            case TOK_BOOLEAN:
            case TOK_FLOAT:
            case TOK_LIST:
            case TOK_HASH:
                ptr->type = tok->type;
                consume_token();
                return ptr;
            case TOK_SYMBOL:
                ptr->type = TOK_NON_TERMINAL;
                ptr->symbol = compound_name();
                return ptr;
            case TOK_CONST:
                ptr->is_const = true;
                consume_token();
                break;
            default:
                syntax("expected a type specification but got a \"%s\"", tokToStr(tok->type));
                consume_token();
                return NULL;
        }
    }

    // unreachable. keeps the compiler happy.
    return NULL;
}

/**
 * @brief Read a class declaration body. When this is entered, the current
 * token should be a '{'. All of the elements are optional and can appear
 * in any order. Class definitions can be nested to any level.
 *
 * class_body_elem
 *   : variable_declaration
 *   | function_declaration
 *   | scope_operator
 *   | class_definition
 *   ;
 *
 * class_body_list
 *   : class_body_elem
 *   | class_body_list class_body_elem
 *   ;
 *
 * class_body
 *   : '{' '}'
 *   | '{' class_body_list '}'
 *   ;
 *
 * @return void*
 */
void* class_body() {

    PTRACE;
    Token* tok = expect_token(TOK_OCBRACE);
    if(tok == NULL)
        return NULL;

    ClassBody* ptr = _alloc_ds(ClassBody);
    ptr->ast.type = AST_CLASS_BODY;
    ptr->list = create_ptr_list();

    consume_token();
    GET_TOKEN(tok); // current token is first one of the actual body
    if(tok->type == TOK_CCBRACE) {
        consume_token();
        return ptr; // definition is empty.
    }

    // TODO: Add the class body guts here.

    return NULL;
}

/**
 * @brief Define a class. When this is entered, the CLASS token is the current
 * token. Classes must have a simple SYMBOL as the name. The parent name is a
 * compound name and it's optional. The class body is read by another function.
 *
 * class_definition
 *   : CLASS SYMBOL '(' compound_name ')' class_body
 *   | CLASS SYMBOL '(' ')' class_body
 *   | CLASS SYMBOL class_body
 *   ;
 *
 * @return void*
 */
void* class_definition() {

    PTRACE;

    consume_token(); // consume the CLASS token.
    Token* tok;
    GET_TOKEN(tok);
    if(tok->type != TOK_SYMBOL) {
        syntax("expected a simple name for the class but got %s",
                    tokToStr(tok->type));
        return NULL;
    }

    ClassDef* ptr = _alloc_ds(ClassDef);
    ptr->ast.type = AST_CLASS_DEF;
    ptr->name = create_string(get_string_ptr(tok->str));
    ptr->parent_name = NULL;
    ptr->body = NULL;
    consume_token();

    GET_TOKEN(tok);
    if(tok->type != TOK_OPAREN) {
        ptr->body = class_body();
        return ptr;
    }
    else {
        consume_token(); // consume the '('
        GET_TOKEN(tok);
        if(tok->type == TOK_CPAREN) {
            // CLASS SYMBOL class_body
            ptr->body = class_body();
            return ptr;
        }
        else if(tok->type == TOK_SYMBOL) {
            // CLASS SYMBOL '(' compound_name
            ptr->parent_name = compound_name();
            GET_TOKEN(tok);
            if(tok->type != TOK_CPAREN) {
                syntax("expected a \")\" or a compound name but got a %s", tokToStr(tok->type));
                return NULL;
            }
            else {
                // CLASS SYMBOL '(' ')'
                consume_token(); // consume the ')'
                // Current token is the first token in the class body.
                // Should be a '{'.
                ptr->body = class_body();
                return ptr;
            }
        }
        else {
            // CLASS SYMBOL ???
            syntax("expected a compound name then a class body but got a %s", tokToStr(tok->type));
            return NULL;
        }
    }

    return NULL;
}

/**
 * @brief Import a file into the parse tree.
 *
 * import_definition
 *   : IMPORT QSTRG
 *   ;
 *
 * @return void*
 */
void* import_definition() {

    PTRACE;
    return NULL;
}

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
    ptr->parms = NULL;
    ptr->body = NULL;
    return ptr;
}

/**
 * @brief Read an expression object in "shunting yard" format so that it can
 * be written out later as a RPN expression.
 *
 * @return void*
 */
void* get_expression() {

    return NULL;
}

/**
 * @brief Read a variable definition and return a pointer to it. The name in
 * this data structure is a CompoundName, but it can only have one element in
 * it. This function takes care to verify that the name is properly formatted
 * because that is a syntax issue not a semantic issue.
 *
 * variable_definition
 *   : type_spec SYMBOL
 *   | type_spec SYMBOL '=' expression
 *   ;
 *
 * @param type
 * @param name
 * @return void*
 */
void* variable_definition(TypeSpec* type, CompoundName* name) {

    PTRACE;

    VariableDef* ptr = _alloc_ds(VariableDef);
    ptr->ast.type = AST_VAR_DEF;
    ptr->type = type;
    ptr->name = name;
    ptr->expr = NULL;

    Token* tok;
    GET_TOKEN(tok);
    if(tok->type == TOK_EQUAL) {
        consume_token();
        ptr->expr = get_expression();
    }

    return ptr;
}

/**
 * @brief When this is entered, it's expecting a type spec, but there is no
 * way to tell if it's a variable or a function definition.
 *
 * func_or_var_def
 *   : type_spec compound_name '(' ')' func_body
 *   | type_spec SYMBOL
 *   | type_spec SYMBOL '=' expression
 *   ;
 *
 * A function definition may have a SYMBOL or a compound name, but a variable
 * definition can only have a SYMBOL. A function definition always has a '('
 * following the name, but a variable definition can have any token that it
 * valid to have in the namespace. If the name is a SYMBOL and it is followed
 * by a '=' then an expression is expected to follow. Otherwise the non-terminal
 * is finished.
 *
 * @return void*
 */
void* func_or_var_def() {

    PTRACE;
    TypeSpec* type = type_spec();
    if(type == NULL) {
        syntax("expected type specification");
        return NULL;
    }

    CompoundName* name = compound_name();
    if(name == NULL) {
        syntax("expected a name specification");
        return NULL;
    }

    // If this is a function definition, the the current token is a '('.
    Token* tok;
    GET_TOKEN(tok);
    if(tok->type == TOK_OPAREN)
        return function_definition(type, name);
    else
        return variable_definition(type, name);

    return NULL;
}

/**
 * @brief Parse the namespace. When this is called, the namespace token has
 * been seen but not consumed. Returns the pointer to the namespace.
 *
 * namespace
 *   : namespace SYMBOL { namespace_body }
 *   ;
 *
 * namespace_body
 *   : class_definition
 *   | function_definition
 *   | variable_definition
 *   | namespace_definition
 *   | scope_operator
 *   ;
 *
 * @return void*
 */
void* namespace_definition() {

    PTRACE;
    CHECK_EOF();

    Token* tok;
    Namespace* ptr = _alloc_ds(Namespace);
    ptr->ast.type = AST_NAMESPACE;
    ptr->list = create_ptr_list();

    consume_token(); // consume the namespace token

    tok = expect_token(TOK_SYMBOL);
    if(tok != NULL)
        ptr->name = create_string(get_string_ptr(tok->str));
    else
        return NULL;
    consume_token(); // consume the SYMBOL

    tok = expect_token(TOK_OCBRACE);
    if(tok == NULL)
        return NULL;
    consume_token(); // consume the '{'

    // parse the contents of a namespace.
    while(get_num_errors() == 0) {
        GET_TOKEN(tok);
        switch(tok->type) {
            case TOK_CLASS:
                append_ptr_list(ptr->list, class_definition());
                break;
            case TOK_NAMESPACE:
                append_ptr_list(ptr->list, namespace_definition());
                break;

            case TOK_PUBLIC:
            case TOK_PRIVATE:
                append_ptr_list(ptr->list, scope_operator());
                break;

            case TOK_CCBRACE:
                // finished...
                consume_token();
                return ptr;

            case TOK_PROTECTED:
                syntax("\"protected\" keyword is only allowed in a class" );
                break;
            default:
                append_ptr_list(ptr->list, func_or_var_def());
                break;
        }
    }

    // an error happened
    return NULL;
}

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


