#include "common.h"
#include "scanner.h"
#include "class.h"

struct _class_body_ {
    Ast ast;
    PtrLst* list;
};

struct _class_def_ {
    Ast ast;
    String name;
    CompoundName* parent_name;
    ClassBody* body;
};

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

