#include "common.h"
#include "scanner.h"
#include "ast.h"
#include "module.h"

struct _namespace_ {
    Ast ast;
    String name;
    PtrLst* list;
};

struct _type_spec_ {
    Ast ast;
    CompoundName* symbol; // NULL if it's a native type.
    int type;
    bool is_const;
};

struct _scope_operator_ {
    Ast ast;
    int type;
};

struct _variable_def_ {
    Ast ast;
    CompoundName* name;
    TypeSpec* type;
    Expression* expr;
};

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
    consume_token(); // consume the import token
    // current token is the quoted string.
    consume_token(); // consume QSTRG
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
        ptr->expr = expression();
    }

    return ptr;
}

/**
 * @brief Read a variable def where that is the only thing that is acceptable
 * in this location.
 *
 * @return void*
 */
void* variable_param_def() {

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

    // return what ever variable definition returns, including the expression.
    return variable_definition(type, name);
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
