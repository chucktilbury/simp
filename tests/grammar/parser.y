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

%right '='
%left OR
%left AND
%left EQU NEQU
%left LORE GORE '<' '>'
%left '+' '-'
%left '*' '/' '%'
%right '^'
%left NEGATE
%left '.'
%left CAST

%glr-parser

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
    : NAMESPACE SYMBOL '{' module_list '}' {TRACE("namespace_definition:NAMESPACE %s", $2);}
    ;

compound_name
    : SYMBOL { TRACE("compound_name: create %s", $1); }
    | compound_name '.' SYMBOL { TRACE("compound_name: add %s", $3); }

type_spec_list
    : type_spec
    | type_spec_list ',' type_spec
    ;

func_type_spec
    : type_definition { TRACE("func_type_spec:type_spec"); }
    | NOTHING { TRACE("func_type_spec:NOTHING"); }
    ;

func_declaration
    : func_type_spec SYMBOL '(' ')' { TRACE("func_declaration:%s()", $2); }
    | func_type_spec SYMBOL '(' type_spec_list ')' { TRACE("func_declaration:%s(type_spec_list)", $2); }

except_clause
    : EXCEPT '(' compound_name ')' func_body
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
    | CLASS SYMBOL '(' compound_name ')' class_body {}
    ;

array_reference
    : SYMBOL '[' expression ']' {TRACE("array_reference:%s", $1);}
    ;

function_reference
    : SYMBOL '(' expression_list ')' {TRACE("function_reference:%s", $1);}
    | SYMBOL '(' ')' {TRACE("function_reference:%s", $1);}
    ;

type_definition
    : UNSIGNED {TRACE("type_definition:UNSIGNED");}
    | INTEGER {TRACE("type_definition:INTEGER");}
    | FLOAT {TRACE("type_definition:FLOAT");}
    | STRING {TRACE("type_definition:STRING");}
    | BOOLEAN {TRACE("type_definition:BOOLEAN");}
    | compound_name { TRACE("type_definition:compound_name");}
    ;

dict_init_element
    : STRG ':' expression {TRACE("dict_init_element:");}
    ;

dict_list
    : dict_init_element {TRACE("dict_list:dict_init_element");}
    | dict_list ',' dict_init_element {TRACE("dict_list:add");}
    ;

type_spec
    : type_definition {TRACE("type_spec:type_definition");}
    | CONST type_definition {TRACE("type_spec:CONST");}
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
    : type_spec SYMBOL {TRACE("symbol_decl:type_spec SYMBOL %s", $2);}
    | type_spec SYMBOL '=' expression {TRACE("symbol_decl:type_spec SYMBOL %s =", $2);}
    | LIST SYMBOL {TRACE("symbol_decl:LIST SYMBOL %s", $2);}
    | DICT SYMBOL {TRACE("symbol_decl:DICT SYMBOL %s", $2);}
    | LIST SYMBOL '=' '[' expression_list ']' {TRACE("symbol_decl:LIST SYMBOL %s =", $2);}
    | DICT SYMBOL '=' '[' dict_list ']' {TRACE("symbol_decl:DICT SYMBOL %s=", $2);}
    ;

symbol_decl_list
    : symbol_decl {TRACE("symbol_decl_list: create");}
    | symbol_decl_list ',' symbol_decl {TRACE("symbol_decl_list: add");}
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
    : STRUCT SYMBOL '{' '}' {TRACE("struct_definition:");}
    | STRUCT SYMBOL '{' struct_list '}' {TRACE("struct_definition:");}
    ;

expression
    : expr_primary
    | expression OR expression { TRACE("expression:or"); }
    | expression AND expression { TRACE("expression:and"); }
    | expression EQU expression { TRACE("expression:=="); }
    | expression NEQU expression { TRACE("expression:!="); }
    | expression LORE expression { TRACE("expression:<="); }
    | expression GORE expression { TRACE("expression:>="); }
    | expression '<' expression { TRACE("expression:<"); }
    | expression '>' expression { TRACE("expression:>"); }
    | expression '+' expression { TRACE("expression:+"); }
    | expression '-' expression { TRACE("expression:-"); }
    | expression '*' expression { TRACE("expression:*"); }
    | expression '/' expression { TRACE("expression:/"); }
    | expression '%' expression { TRACE("expression:%%"); }
    | expression '^' expression { TRACE("expression:^"); }
    | '-' expression %prec NEGATE {TRACE("expression unary:-");}
    | '+' expression %prec NEGATE {TRACE("expression unary:+");}
    | NOT expression %prec NEGATE {TRACE("expression unary:not");}
    | expression '.' expression { TRACE("expression:."); }
    | '(' compound_name ')' expression %prec CAST { TRACE("expression:cast"); }
    | '(' expression ')' {TRACE("expression:()");}
    ;

expr_primary
    : constant_expression {TRACE("expr_primary:constant_expression");}
    | function_reference {TRACE("expr_primary:function_reference");}
    | array_reference {TRACE("expr_primary:array_reference");}
    | SYMBOL {TRACE("expr_primary:SYMBOL %s", $1);}
    ;

expression_list
    : expression {TRACE("expression_list:");}
    | expression_list ',' expression {TRACE("expression_list:add");}
    ;

func_definition
    : func_type_spec SYMBOL '(' symbol_decl_list ')' func_body {TRACE("func_definition:");}
    | func_type_spec SYMBOL '(' ')' func_body {TRACE("func_definition:");}
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
    : compound_name '=' expression {TRACE("assignment:=");}
    | compound_name ADD_ASSIGN expression {TRACE("assignment:+=");}
    | compound_name SUB_ASSIGN expression {TRACE("assignment:-=");}
    | compound_name MUL_ASSIGN expression {TRACE("assignment:*=");}
    | compound_name DIV_ASSIGN expression {TRACE("assignment:/=");}
    | compound_name MOD_ASSIGN expression {TRACE("assignment:%%=");}
    ;

func_body_statement
    : symbol_decl {TRACE("func_body_statement:symbol_decl");}
    | assignment {TRACE("func_body_statement:assignment");}
    | if_statement {TRACE("func_body_statement:if_statement");}
    | while_statement {TRACE("func_body_statement:while_statement");}
    | do_statement {TRACE("func_body_statement:do_statement");}
    | function_reference {TRACE("func_body_statement:function_reference");}
    | trace_statement {TRACE("func_body_statement:trace_statement");}
    | return_statement {TRACE("func_body_statement:return_statement");}
    | print_statement {TRACE("func_body_statement:print_statement");}
    | exit_statement {TRACE("func_body_statement:exit_statement");}
    | type_statement {TRACE("func_body_statement:type_statement");}
    | break_statement {TRACE("func_body_statement:break_statement");}
    | continue_statement {TRACE("func_body_statement:continue_statement");}
    | yield_statement {TRACE("func_body_statement:yield_statement");}
    | struct_definition {TRACE("func_body_statement:struct_definition");}
    | func_body {TRACE("func_body_statement:func_body");}
    | try_clause {TRACE("func_body_statement:try_clause");}
    | raise_statement {TRACE("func_body_statement:raise_statement");}
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
