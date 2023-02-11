/**
 * @file tokens.c
 *
 * @brief This is mostly a wrapper for the scanner. It provides an API that
 * is centered around tokens, rather than chunks of input text.
 *
 */
#include <stdio.h>
#include <stdbool.h>

#include "scanner.h"
#include "errors.h"
#include "tokens.h"

/**
 * @brief Check the current token for the desired type and return the token
 * if it's a match. Otherwise publish a syntax error and return NULL.
 *
 * @param type
 * @return Token*
 */
Token* expect_token(TokenType type) {

    Token* tok = crnt_token();
    if(tok->type == type) {
        TTRACE(tok);
        return tok;
    }

    syntax("expected a \"%s\" but got a \"%s\"", tokToStr(type), tokToStr(tok->type));
    return NULL;
}

/**
 * @brief Compare the current token with a list of tokens and it the current
 * one is in the list then return it, otherwise publish a syntax error and
 * return NULL.
 *
 * @param num
 * @param list
 * @return Token*
 */
Token* expect_token_list(int num, TokenType* list) {

    Token* tok = crnt_token();
    for(int i = 0; i < num; i++)
        if(tok->type == list[i]) {
            TTRACE(tok);
            return tok;
        }

    String s = create_string("expected a ");
    for(int i = 0; i < num; i++) {
        if(i < num-1)
            append_string_str(s, "\"%s\", ", tokToStr(list[i]));
        else
            append_string_str(s, "or \"%s\" ", tokToStr(list[i]));
    }
    append_string_str(s, "but got a \"%s\"", tokToStr(tok->type));

    syntax(get_string_ptr(s));
    return NULL;
}

/**
 * @brief Return the current token after checking for the end of the file.
 * This is the way to get tokens most of the time.
 *
 * @return Token*
 */
Token* get_token() {

    Token* tok = crnt_token();
    if(tok->type == TOK_END_FILE || tok->type == TOK_END_INPUT) {
        syntax("unexpected end of file.");
        return NULL;
    }

    TTRACE(tok);
    return tok;
}

/**
 * @brief Return a string that represents the token ID. For error handling.
 *
 * @param tt
 * @return const char*
 */
const char* tokToStr(TokenType t) {

    return (t == TOK_ERROR)?    "ERROR":
        (t == TOK_NON_TERMINAL)? "NON TERMINAL": // used in rare cases.
        (t == TOK_END_FILE)?    "END OF FILE":
        (t == TOK_END_INPUT)?   "END OF INPUT":
        (t == TOK_CONST)?       "const":
        (t == TOK_IMPORT)?      "import":
        (t == TOK_BREAK)?       "break":
        (t == TOK_CONTINUE)?    "continue":
        (t == TOK_DO)?          "do":
        (t == TOK_ELSE)?        "else":
        (t == TOK_IF)?          "if":
        (t == TOK_WHILE)?       "while":
        (t == TOK_RETURN)?      "return":
        (t == TOK_TRUE)?        "true":
        (t == TOK_FALSE)?       "false":
        (t == TOK_YIELD)?       "yield":
        (t == TOK_EXIT)?        "exit":
        (t == TOK_TRACE)?       "trace":
        (t == TOK_PRINT)?       "print":
        (t == TOK_TYPE)?        "type":
        (t == TOK_STRUCT)?      "struct":
        (t == TOK_NAMESPACE)?   "namespace":
        (t == TOK_CLASS)?       "class":
        (t == TOK_PUBLIC)?      "public":
        (t == TOK_PRIVATE)?     "private":
        (t == TOK_PROTECTED)?   "protected":
        (t == TOK_TRY)?         "try":
        (t == TOK_CATCH)?       "catch":
        (t == TOK_RAISE)?       "raise":
        (t == TOK_INLINE)?      "inline":
        (t == TOK_INT)?         "integer":
        (t == TOK_UINT)?        "unsigned":
        (t == TOK_NOTHING)?     "nothing":
        (t == TOK_STRING)?      "string":
        (t == TOK_BOOLEAN)?     "boolean":
        (t == TOK_FLOAT)?       "float":
        (t == TOK_LIST)?        "list":
        (t == TOK_HASH)?        "hash":
        (t == TOK_LORE)?        "<=":
        (t == TOK_GORE)?        ">=":
        (t == TOK_EQU)?         "equ":
        (t == TOK_NEQU)?        "neq":
        (t == TOK_OR)?          "or":
        (t == TOK_AND)?         "and":
        (t == TOK_ADD_ASSIGN)?  "+=":
        (t == TOK_SUB_ASSIGN)?  "-=":
        (t == TOK_MUL_ASSIGN)?  "*=":
        (t == TOK_DIV_ASSIGN)?  "/=":
        (t == TOK_MOD_ASSIGN)?  "%=":
        (t == TOK_NOT)?         "not":
        (t == TOK_ADD)?         "+":
        (t == TOK_SUB)?         "-":
        (t == TOK_MUL)?         "*":
        (t == TOK_DIV)?         "/":
        (t == TOK_MOD)?         "%":
        (t == TOK_EQUAL)?       "=":
        (t == TOK_OPAREN)?      "(":
        (t == TOK_CPAREN)?      ")":
        (t == TOK_OPBRACE)?     "<":
        (t == TOK_CPBRACE)?     ">":
        (t == TOK_OSBRACE)?     "[":
        (t == TOK_CSBRACE)?     "]":
        (t == TOK_OCBRACE)?     "{":
        (t == TOK_CCBRACE)?     "}":
        (t == TOK_COMMA)?       ",":
        (t == TOK_DOT)?         ".":
        (t == TOK_SYMBOL)?      "SYMBOL":
        (t == TOK_INUM)?        "INTEGER":
        (t == TOK_FNUM)?        "FLOAT":
        (t == TOK_UNUM)?        "UNSIGNED":
        (t == TOK_QSTRG)?       "QSTRG": "UNKNOWN";
}

