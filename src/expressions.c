/**
 * @brief Expressions. There are enough functions to justify a separate group.
 *
 *
 * Expressions are arranged from lowest precedence to the highest. Each
 * precedence level has its own rule that references the next higher
 * level.
 *
 * expression
 *     : expr_or
 *     ;
 *
 * expr_or
 *     : expr_and
 *     | expr_or OR expr_and
 *     ;
 *
 * expr_and
 *     : expr_equality
 *     | expr_and AND expr_equality
 *     ;
 *
 * expr_equality
 *     : expr_compare
 *     | expr_equality "==" expr_compare
 *     | expr_equality "!=" expr_compare
 *     ;
 *
 * expr_compare
 *     : expr_term
 *     | expr_compare "<=" expr_term
 *     | expr_compare ">=" expr_term
 *     | expr_compare '<' expr_term
 *     | expr_compare '>' expr_term
 *     ;
 *
 * expr_term
 *     : expr_factor
 *     | expr_term '+' expr_factor
 *     | expr_term '-' expr_factor
 *     ;
 *
 * expr_factor
 *     : expr_unary
 *     | expr_factor '*' expr_unary
 *     | expr_factor '/' expr_unary
 *     | expr_factor '%' expr_unary
 *     ;
 *
 * expr_unary
 *     : expr_primary
 *     | '-' expr_unary
 *     | '+' expr_unary
 *     | NOT expr_unary
 *     | cast_spec expr_unary
 *     ;
 *
 * expr_primary
 *     : constant_expression
 *     | compound_reference
 *     | '(' expression ')'
 *     ;
 *
 * expression_list
 *     : expression
 *     | expression_list ',' expression
 *     ;
 *
 */
#include "common.h"
#include "parser.h"

// TODO: add the guts of this.....
struct _expression_ {
    Ast* ast;
    PtrLst* list;
};

struct _expression_list_ {
    Ast ast;
    PtrLst* list;
};

/**
 * @brief Parse the primary precedence of the expression.
 *
 * expr_primary
 *     : constant_expression
 *     | compound_reference
 *     | '(' expression ')'
 *     ;
 *
 * @return void*
 */
void* expr_primary() {

    PTRACE;

    return NULL;
}

/**
 * @brief Parse the unary precedence expression.
 *
 * expr_unary
 *     : expr_primary
 *     | '-' expr_unary
 *     | '+' expr_unary
 *     | NOT expr_unary
 *     | cast_spec expr_unary
 *     ;
 *
 * @return void*
 */
void* expr_unary() {

    PTRACE;

    return NULL;
}

/**
 * @brief Parse the factor precedence expression.
 *
 * expr_factor
 *     : expr_unary
 *     | expr_factor '*' expr_unary
 *     | expr_factor '/' expr_unary
 *     | expr_factor '%' expr_unary
 *     ;
 *
 * @return void*
 */
void* expr_factor() {

    PTRACE;

    return NULL;
}

/**
 * @brief Parse the term precedence expression
 *
 * expr_term
 *     : expr_factor
 *     | expr_term '+' expr_factor
 *     | expr_term '-' expr_factor
 *     ;
 *
 * @return void*
 */
void* expr_term() {

    PTRACE;

    return NULL;
}

/**
 * @brief Parse the comparison precedence expression.
 *
 * expr_compare
 *     : expr_term
 *     | expr_compare "<=" expr_term
 *     | expr_compare ">=" expr_term
 *     | expr_compare '<' expr_term
 *     | expr_compare '>' expr_term
 *     ;
 *
 * @return void*
 */
void* expr_compare() {

    PTRACE;

    return NULL;
}

/**
 * @brief Parse the equality precedence of the expression.
 *
 * expr_equality
 *     : expr_compare
 *     | expr_equality "==" expr_compare
 *     | expr_equality "!=" expr_compare
 *     ;
 *
 * @return void*
 */
void* expr_equality() {

    PTRACE;

    return NULL;
}

/**
 * @brief Parser the AND clause of an expression.
 *
 * expr_and
 *     : expr_equality
 *     | expr_and AND expr_equality
 *     ;
 *
 * @return void*
 */
void* expr_and() {

    PTRACE;

    return NULL;
}

/**
 * @brief The OR clause is the lowest precedence expression.
 *
 * expr_or
 *     : expr_and
 *     | expr_or OR expr_and
 *     ;
 *
 * @return void*
 */
void* expr_or() {

    PTRACE;

    return NULL;
}

/**
 * @brief Read an expression object in "shunting yard" format so that it can
 * be written out later as a RPN expression.
 *
 * expression
 *     : expr_or
 *     ;
 *
 * @return void*
 */
void* expression() {

    PTRACE;

    return NULL;
}


/**
 * @brief Make a list of expressions separated by a ',' token.
 *
 * expression_list
 *     : expression
 *     | expression_list ',' expression
 *     ;
 *
 * @return void*
 */
void* expression_list() {

    PTRACE;

    return NULL;
}

