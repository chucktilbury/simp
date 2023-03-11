%debug
%defines
%locations

%{

#include <stdio.h>
#include <stdint.h>

#include "scanner.h"

#define TRACE(fmt, ...) do { \
        printf((fmt), ## __VA_ARGS__); \
        fputc('\n', stdout); \
        } while(0)

int errors = 0;
// TODO:
// 1. Structs with LH and RH syntax for references and assignment.
// 2. Init a struct with static data.
// 3. Structs can be nested.
// 4. Syntax for list and hash definition and access with weak types.
// 5. Syntax for string formatting? Revisit?
// 6. Namespaces? Scopes?
//
// Might happen:
// 1. Parsing imports for symbols instead of simple include.
// 2. Use the linker.
//
// Sky Pie:
// 1. Functions defined for structs.
// 2. Non-local GOTO.
//
// To NOT do:
// 1. Exceptions
// 2. Classes
// 3. Function pointers
//

%}

%union {
    char* str;
    char* symbol;
    double fnum;
    unsigned unum;
    int inum;
    int type;

};

%token BREAK CONTINUE CONST NAMESPACE LIST DICT
%token DO ELSE IF RETURN IMPORT CLASS TRY EXCEPT RAISE
%token TRUE FALSE YIELD EXIT STRUCT WHILE
%token EQU NEQU LORE GORE OR AND NOT
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token UNSIGNED INTEGER FLOAT NOTHING STRING BOOLEAN
%token PRINT TRACE TYPE

%token<symbol> SYMBOL
%token<fnum> FNUM
%token<inum> INUM
%token<unum> UNUM
%token<str> STRG

%define parse.error verbose
%locations

%%

module
    : module_list {TRACE("module:module_list");}
    ;

module_list
    : module_item {TRACE("module_list:module_item");}
    | module_list module_item {TRACE("module_list:add");}
    ;

module_item
    : symbol_decl {TRACE("module_item:symbol_decl");}
    | func_definition {TRACE("module_item:func_definition");}
    | struct_definition {TRACE("module_item:struct_definition");}
    | namespace_definition {TRACE("module_item:namespace_definition");}
    | class_definition {}
    | IMPORT STRG {TRACE("module_item:IMPORT");}
    ;

namespace_definition
    : NAMESPACE SYMBOL '{' module_list '}' {TRACE("namespace_definition:NAMESPACE");}
    ;

type_spec_list
    : type_spec
    | type_spec_list ',' type_spec
    ;

func_declaration
    : type_spec SYMBOL '(' ')' {}
    | type_spec SYMBOL '(' type_spec_list ')' {}

except_clause
    : EXCEPT '(' SYMBOL ')' func_body
    ;

except_clause_final
    : EXCEPT '(' ')' func_body
    | EXCEPT func_body
    ;

except_list
    : except_clause
    | except_list except_clause
    ;

try_clause
    : TRY func_body except_list
    | TRY func_body except_list except_clause_final
    ;

class_body_element
    : symbol_decl {}
    | func_definition {}
    | func_declaration {}
    | struct_definition {}
    ;

class_body_list
    : class_body_element {}
    | class_body_list class_body_element {}
    ;

class_body
    : '{' '}' {}
    | '{' class_body_list '}' {}
    ;

class_definition
    : CLASS SYMBOL class_body {}
    | CLASS SYMBOL '(' compound_symbol ')' class_body {}
    ;

array_reference
    : SYMBOL '[' expression ']' {TRACE("array_reference:");}
    ;

function_reference
    : SYMBOL '(' expression_list ')' {TRACE("function_reference:");}
    | SYMBOL '(' ')' {TRACE("function_reference:");}
    ;

compound_symbol
    : SYMBOL
    | compound_symbol '.' SYMBOL {}
    ;

compound_refrence_element
    : SYMBOL {TRACE("compound_element:SYMBOL");}
    | array_reference {TRACE("compound_element:array_reference");}
    | function_reference {}
    ;

compound_reference
    : compound_refrence_element {TRACE("compound_name:compound_element");}
    | compound_reference '.' compound_refrence_element {TRACE("compound_name:add");}
    ;

type_definition
    : UNSIGNED {TRACE("type_definition:UNSIGNED");}
    | INTEGER {TRACE("type_definition:INTEGER");}
    | FLOAT {TRACE("type_definition:FLOAT");}
    | STRING {TRACE("type_definition:STRING");}
    | BOOLEAN {TRACE("type_definition:BOOLEAN");}
    | compound_symbol {}
    ;

dict_init_element
    : SYMBOL ':' expression {TRACE("dict_init_element:");}
    ;

dict_list
    : dict_init_element {TRACE("dict_list:dict_init_element");}
    | dict_list ',' dict_init_element {TRACE("dict_list:add");}
    ;

type_spec
    : type_definition {TRACE("type_spec:type_definition");}
    | CONST type_definition {TRACE("type_spec:CONST");}
    | NOTHING {TRACE("type_spec:NOTHING");}
    ;

cast_spec
    : '(' type_spec ')' {TRACE("cast_spec:");}
    ;

constant_expression
    : UNUM {TRACE("constant_expression:UNUM");}
    | FNUM {TRACE("constant_expression:FNUM");}
    | INUM {TRACE("constant_expression:INUM");}
    | STRG {TRACE("constant_expression:STRG");}
    | TRUE {TRACE("constant_expression:TRUE");}
    | FALSE {TRACE("constant_expression:FALSE");}
    ;

symbol_decl
    : type_spec SYMBOL {TRACE("symbol_decl:type_spec SYMBOL");}
    | type_spec SYMBOL '=' expression {TRACE("symbol_decl:type_spec SYMBOL =");}
    | LIST SYMBOL {TRACE("symbol_decl:LIST SYMBOL");}
    | DICT SYMBOL {TRACE("symbol_decl:DICT SYMBOL");}
    | LIST SYMBOL '=' '[' expression_list ']' {TRACE("symbol_decl:LIST SYMBOL =");}
    | DICT SYMBOL '=' '[' dict_list ']' {TRACE("symbol_decl:DICT SYMBOL =");}
    ;

symbol_decl_list
    : symbol_decl {TRACE("symbol_decl_list:");}
    | symbol_decl_list ',' symbol_decl {TRACE("symbol_decl_list:");}
    ;

struct_element
    : symbol_decl_list {TRACE("struct_element:");}
    | struct_definition {TRACE("struct_element:");}
    ;

struct_list
    : struct_element {TRACE("struct_list:");}
    | struct_list struct_element {TRACE("struct_list:");}
    ;

struct_definition
    :   STRUCT SYMBOL '{' '}' {TRACE("struct_definition:");}
    |   STRUCT SYMBOL '{' struct_list '}' {TRACE("struct_definition:");}
    ;

    /*
     * Expressions are arranged from lowest precidence to the highest. Each
     * precidence level has its own rule that references the next higher
     * level.
     */
expression
    : expr_or {TRACE("expression:");}
    ;

expr_or
    : expr_and {TRACE("expr_or:and");}
    | expr_or OR expr_and {TRACE("expr_or:or");}
    ;

expr_and
    : expr_equality {TRACE("expr_and:=");}
    | expr_and AND expr_equality {TRACE("expr_and:and");}
    ;

expr_equality
    : expr_compare {TRACE("expr_equality:compare");}
    | expr_equality EQU expr_compare {TRACE("expr_equality:==");}
    | expr_equality NEQU expr_compare {TRACE("expr_equality:!=");}
    ;

expr_compare
    : expr_term {TRACE("expr_compare:term");}
    | expr_compare LORE expr_term {TRACE("expr_compare:<=");}
    | expr_compare GORE expr_term {TRACE("expr_compare:>=");}
    | expr_compare '<' expr_term {TRACE("expr_compare:<");}
    | expr_compare '>' expr_term {TRACE("expr_compare:>");}
    ;

expr_term
    : expr_factor {TRACE("expr_term:fact");}
    | expr_term '+' expr_factor {TRACE("expr_term:+");}
    | expr_term '-' expr_factor {TRACE("expr_term:-");}
    ;

expr_factor
    : expr_unary {TRACE("expr_factor:unary");}
    | expr_factor '*' expr_unary {TRACE("expr_factor:*");}
    | expr_factor '/' expr_unary {TRACE("expr_factor:/");}
    | expr_factor '%' expr_unary {TRACE("expr_factor:%%");}
    ;

expr_unary
    : expr_primary {TRACE("expr_unary:primary");}
    | '-' expr_unary {TRACE("expr_unary:-");}
    | '+' expr_unary {TRACE("expr_unary:+");}
    | NOT expr_unary {TRACE("expr_unary:not");}
    | cast_spec expr_unary {TRACE("expr_unary:cast");}
    ;

expr_primary
    : constant_expression {TRACE("expr_primary:const");}
    | compound_reference {TRACE("expr_primary:compound");}
    | '(' expression ')' {TRACE("expr_primary:()");}
    ;

expression_list
    : expression {TRACE("expression_list:");}
    | expression_list ',' expression {TRACE("expression_list:add");}
    ;

func_definition
    : type_spec SYMBOL '(' symbol_decl_list ')' func_body {TRACE("func_definition:");}
    | type_spec SYMBOL '(' ')' func_body {TRACE("func_definition:");}
    | type_spec SYMBOL func_body {TRACE("func_definition:");}
    ;

func_body_statement_list
    : func_body_statement {TRACE("func_body_statement_list:");}
    | func_body_statement_list func_body_statement {TRACE("func_body_statement_list:");}
    ;

func_body
    : '{' func_body_statement_list '}' {TRACE("func_body:");}
    | '{' '}' {TRACE("func_body:");}
    ;

    /*
     * Note that the final else clause would be syntattically determined instead
     * of symantically if this were a bison parser design.
     */
else_clause
    : ELSE '(' expression ')' func_body {TRACE("else_clause:");}
    | ELSE '(' ')' func_body {TRACE("else_clause:");}
    | ELSE func_body {TRACE("else_clause:");}
    ;

else_clause_list
    : else_clause {TRACE("else_clause_list:");}
    | else_clause_list else_clause {TRACE("else_clause_list:");}
    ;

    /* expression stacks? */
if_clause
    : IF '(' expression ')' func_body {TRACE("if_clause:");}
    ;

if_statement
    : if_clause {TRACE(":");}
    | if_clause else_clause_list {TRACE("if_statement:");}
    ;

while_statement
    : WHILE '(' expression ')' func_body {TRACE("while_statement:");}
    | WHILE '(' ')' func_body {TRACE("while_statement:");}
    | WHILE func_body {TRACE("while_statement:");}
    ;

do_statement
    : DO func_body WHILE '(' expression ')' {TRACE("do_statement:");}
    | DO func_body WHILE '(' ')' {TRACE("do_statement:");}
    | DO func_body WHILE {TRACE("do_statement:");}
    ;

break_statement
    : BREAK {TRACE("break_statement:");}
    ;

continue_statement
    : CONTINUE {TRACE("continue_statement:");}
    ;

yield_statement
    : YIELD {TRACE("yield_statement:");}
    | YIELD '(' expression ')' {TRACE("yield_statement:");}
    ;

trace_statement
    : TRACE {TRACE("trace_statement:");}
    | TRACE '(' ')' {TRACE("trace_statement:");}
    | TRACE '(' STRG ')' {TRACE("trace_statement:");}
    ;

return_statement
    : RETURN {TRACE("return_statement:");}
    | RETURN '(' ')' {TRACE("return_statement:");}
    | RETURN '(' expression ')' {TRACE("return_statement:");}
    ;

print_statement
    : PRINT {TRACE("print_statement:");}
    | PRINT '(' ')' {TRACE("print_statement:");}
    | PRINT '(' expression ')' {TRACE("print_statement:");}
    ;

exit_statement
    : EXIT {TRACE("exit_statement:");}
    | EXIT '(' expression ')' {TRACE("exit_statement:");}
    ;

type_statement
    : TYPE '(' expression ')' {TRACE("type_statement:");}
    ;

raise_statement
    : RAISE '(' SYMBOL ')' {}
    | RAISE '(' ')' {}
    | RAISE {}
    ;

assignment
    : compound_symbol '=' expression {TRACE("assignment:=");}
    | compound_symbol ADD_ASSIGN expression {TRACE("assignment:+=");}
    | compound_symbol SUB_ASSIGN expression {TRACE("assignment:-=");}
    | compound_symbol MUL_ASSIGN expression {TRACE("assignment:*=");}
    | compound_symbol DIV_ASSIGN expression {TRACE("assignment:/=");}
    | compound_symbol MOD_ASSIGN expression {TRACE("assignment:%%=");}
    ;

func_body_statement
    : symbol_decl {TRACE(":");}
    | assignment {TRACE(":");}
    | if_statement {TRACE(":");}
    | while_statement {TRACE(":");}
    | do_statement {TRACE(":");}
    | function_reference {TRACE(":");}
    | trace_statement {TRACE(":");}
    | return_statement {TRACE(":");}
    | print_statement {TRACE(":");}
    | exit_statement {TRACE(":");}
    | type_statement {TRACE(":");}
    | break_statement {TRACE(":");}
    | continue_statement {TRACE(":");}
    | yield_statement {TRACE(":");}
    | struct_definition {TRACE(":");}
    | func_body {TRACE(":");}
    | try_clause {}
    | raise_statement {}
    ;

%%

extern int errors;
void yyerror(const char* s) {

    fprintf(stderr, "%s:%d:%d %s\n",
            get_file_name(), get_line_no(), get_col_no(), s);
    errors++;
}

const char* tokenToStr(int tok) {

    return yysymbol_name(YYTRANSLATE(tok));
}
