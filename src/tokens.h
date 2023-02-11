#ifndef _TOKENS_H
#define _TOKENS_H

typedef enum {
    // Parser control tokens
    TOK_ERROR = 0xFF,   // Inform the parser that the token is no good
    TOK_NON_TERMINAL,   // This token is a non-terminal.
    TOK_END_INPUT,
    TOK_END_FILE,

    // Keywords
    TOK_CONST,      // "const"
    TOK_IMPORT,     // "import"

    TOK_BREAK,      // "break"
    TOK_CONTINUE,   // "continue"

    TOK_DO,         // "do"
    TOK_ELSE,       // "else"
    TOK_IF,         // "if"
    TOK_WHILE,      // "while"
    TOK_RETURN,     // "return"

    TOK_TRUE,       // "true"
    TOK_FALSE,      // "false"

    TOK_YIELD,      // "yield"
    TOK_EXIT,       // "exit"
    TOK_TRACE,      // "trace"
    TOK_PRINT,      // "print"
    TOK_TYPE,       // "type"

    TOK_STRUCT,     // "struct"
    TOK_NAMESPACE,  // "namespace"
    TOK_CLASS,      // "class"
    TOK_PUBLIC,     // "public"
    TOK_PRIVATE,    // "private"
    TOK_PROTECTED,  // "protected"
    TOK_TRY,        // "try"
    TOK_CATCH,      // "catch"
    TOK_RAISE,      // "raise"

    TOK_INLINE,     // "inline"

    // Multi-keyword tokens
    TOK_INT,        // "int"|"integer"
    TOK_UINT,       // "uint"|"unsigned"
    TOK_NOTHING,    // "noth"|"nothing"
    TOK_STRING,     // "strg"|"string"
    TOK_BOOLEAN,    // "bool"|"boolean"
    TOK_FLOAT,      // "float"

    TOK_LIST,       // "list"
    TOK_HASH,       // "hash"

    // Multi-character operators
    TOK_LORE,       // "<="
    TOK_GORE,       // ">="
    TOK_EQU,        // "=="|"equ"
    TOK_NEQU,       // "!="|"neq"
    TOK_OR,         // "or"
    TOK_AND,        // "and"
    TOK_ADD_ASSIGN, // "+="
    TOK_SUB_ASSIGN, // "-="
    TOK_MUL_ASSIGN, // "*="
    TOK_DIV_ASSIGN, // "/="
    TOK_MOD_ASSIGN, // "%="
    TOK_NOT,        // not

    // Single character operators
    TOK_ADD,        // "+"
    TOK_SUB,        // "-"
    TOK_MUL,        // "*"
    TOK_DIV,        // "/"
    TOK_MOD,        // "%"
    TOK_EQUAL,      // "="
    TOK_OPAREN,     // "("
    TOK_CPAREN,     // ")"
    TOK_OPBRACE,    // "<"
    TOK_CPBRACE,    // ">"
    TOK_OSBRACE,    // "["
    TOK_CSBRACE,    // "]"
    TOK_OCBRACE,    // "{"
    TOK_CCBRACE,    // "}"

    TOK_COMMA,      // ","
    TOK_DOT,        // "."

    // Generated tokens
    TOK_SYMBOL,     //  [a-zA-Z_][a-zA-Z_0-9]*
    TOK_INUM,       // [0-9]+
    TOK_FNUM,       // ([0-9]*\.)?[0-9]+([Ee][-+]?[0-9]+)?
    TOK_UNUM,       // 0[Xx][0-9a-fA-F]+
    TOK_QSTRG,      // quoted string
} TokenType;

#include "strptr.h"

typedef struct {
    TokenType type;
    String str;
} Token;

Token* expect_token(TokenType type);
Token* expect_token_list(int num, TokenType* list);
const char* tokToStr(TokenType tt);
Token* get_token();

#define CHECK_EOF() \
    do { \
        Token* _local_token_ = crnt_token(); \
        if(_local_token_->type == TOK_END_FILE \
                || _local_token_->type == TOK_END_INPUT) { \
            syntax("unexpected end of file"); \
            return NULL; \
        } \
    } while(0)

#define GET_TOKEN(t) \
    do { \
        t = get_token(); \
        if(t == NULL) { \
            syntax("unexpected end of file"); \
            return NULL; \
        } \
    } while(0)

#ifdef ENABLE_TOKENS_TRACE
#include "filebuf.h"
# define TTRACE(t) fprintf(stderr, "token\t%s()\t%s:%d:%d \"%s\" \"%s\"(0x%02X)\n", \
            __func__, get_file_name(), get_line_no(), get_col_no(),\
            get_string_ptr(t->str), tokToStr(t->type), t->type)
#else
# define TTRACE(t)
#endif

#endif