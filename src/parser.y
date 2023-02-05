%debug
%defines
%locations

%{

#include <stdio.h>
#include <stdint.h>

#include "scanner.h"

#ifdef ENABLE_PARSER_TRACE
#define PTRACE(fmt, ...) \
    do { \
        printf("%s:%d:%d: ", get_file_name(), get_line_no(), get_col_no()); \
        printf(fmt, ## __VA_ARGS__); fputc('\n', stdout); \
    } while(0)
#else
#define PTRACE(fmt, ...)
#endif

%}

%union {
    char* str;
    char* symbol;
    double fnum;
    uint64_t unum;
    int64_t inum;
    int type;

    /* Union objects for the AST. */
    void* ptr;
};

%token BREAK CONTINUE CONST
%token DO ELSE IF RETURN IMPORT
%token TRUE FALSE YIELD EXIT WHILE
%token EQU NEQU LORE GORE OR AND NOT
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token UINT INT FLOAT NOTHING STRING BOOLEAN
%token PRINT TRACE TYPE

%token<symbol> SYMBOL
%token<fnum> FNUM
%token<inum> INUM
%token<unum> UNUM
%token<str> STRG

%define parse.error verbose
%locations

%left CAST
%left OR
%left AND
%left EQU NEQU
%left LORE GORE '<' '>'
%left '+' '-'
%left '*' '/' '%'
%left NEGATE

%%

module
    : module_list { PTRACE("module:module_list"); }
    ;

module_list
    : module_item { PTRACE("module_list:module_item"); }
    | module_list module_item { PTRACE("module_list:module_list"); }
    ;

module_item
    : symbol_decl { PTRACE("module_item:symbol_decl"); }
    | func_definition { PTRACE("module_item:func_definition"); }
    | IMPORT STRG { PTRACE("module_item:IMPORT STRG"); }
    | error { PTRACE("module_item:error"); }
    ;

type_definition
    : UINT { PTRACE("type_definition:UINT"); }
    | INT { PTRACE("type_definition:INT"); }
    | FLOAT { PTRACE("type_definition:FLOAT"); }
    | STRING { PTRACE("type_definition:STRING"); }
    | BOOLEAN { PTRACE("type_definition:BOOLEAN"); }
    ;

type_spec
    : type_definition { PTRACE("type_spec:type_definition"); }
    | CONST type_definition { PTRACE("type_spec:CONST type_definition"); }
    | NOTHING { PTRACE("type_spec:NOTHING"); }
    ;

cast_spec
    : '(' type_spec ')' { PTRACE("cast_spec:(type_spec)"); }
    ;

constant_expression
    : UNUM { PTRACE("constant_expression:UNUM"); }
    | FNUM { PTRACE("constant_expression:FNUM"); }
    | INUM { PTRACE("constant_expression:INUM"); }
    | STRG { PTRACE("constant_expression:STRG"); }
    | TRUE { PTRACE("constant_expression:TRUE"); }
    | FALSE { PTRACE("constant_expression:FALSE"); }
    ;

symbol_intro
    : type_spec SYMBOL { PTRACE("symbol_intro:type_spec SYMBOL"); }
    ;

symbol_decl
    : symbol_intro { PTRACE("symbol_decl:symbol_intro"); }
    | symbol_intro '=' expression { PTRACE("symbol_decl:symbol_intro=expression"); }
    ;

symbol_intro_list
    : symbol_intro { PTRACE("symbol_intro_list:symbol_intro"); }
    | symbol_intro_list ',' symbol_intro { PTRACE("symbol_intro_list:symbol_intro_list,symbol_intro"); }
    ;

expression_factor
    : constant_expression { PTRACE("expression_factor:constant_expression"); }
    | SYMBOL { PTRACE("expression_factor:SYMBOL"); }
    | function_reference { PTRACE("expression_factor:function_reference"); }
    ;

expression
    : expression_factor { PTRACE("expression:expression_factor"); }
    | expression '+' expression { PTRACE("expression:expression + expression"); }
    | expression '-' expression { PTRACE("expression:expression - expression"); }
    | expression '*' expression { PTRACE("expression:expression * expression"); }
    | expression '/' expression { PTRACE("expression:expression / expression"); }
    | expression '%' expression { PTRACE("expression:expression %% expression"); }
    | expression EQU expression { PTRACE("expression:expression EQU expression"); }
    | expression NEQU expression { PTRACE("expression:expression NEQU expression"); }
    | expression LORE expression { PTRACE("expression:expression LORE expression"); }
    | expression GORE expression { PTRACE("expression:expression GORE expression"); }
    | expression OR expression { PTRACE("expression:expression OR expression"); }
    | expression AND expression { PTRACE("expression:expression AND expression"); }
    | expression '<' expression { PTRACE("expression:expression < expression"); }
    | expression '>' expression { PTRACE("expression:expression > expression"); }
    | '-' expression %prec NEGATE { PTRACE("expression:- expression %%prec NEGATE"); }
    | NOT expression %prec NEGATE { PTRACE("expression:NOT expression %%prec NEGATE"); }
    | cast_spec expression %prec CAST { PTRACE("expression:cast_spec expression %%prec CAST"); }
    | '(' expression ')' { PTRACE("expression:(expression)"); }
    | error { PTRACE("expression:error"); }
    ;

expression_list
    : expression { PTRACE("expression_list:expression"); }
    | expression_list ',' expression { PTRACE("expression_list:expression_list,expression"); }
    ;

function_reference
    : SYMBOL '(' expression_list ')' { PTRACE("function_reference:SYMBOL(expression_list)"); }
    | SYMBOL '(' ')' { PTRACE("function_reference:SYMBOL()"); }
    ;

func_decl_parameter_list
    : '(' symbol_intro_list ')' { PTRACE("func_decl_parameter_list:(symbol_intro_list)"); }
    | '(' ')' { PTRACE("func_decl_parameter_list:()"); }
    ;

func_definition
    : type_spec SYMBOL func_decl_parameter_list func_body { PTRACE("func_definition:type_spec SYMBOL func_decl_parameter_list func_body"); }
    ;

func_body_statement_list
    : func_body_statement { PTRACE("func_body_statement_list:func_body_statement"); }
    | func_body_statement_list func_body_statement { PTRACE("func_body_statement_list:func_body_statement_list func_body_statement"); }
    ;

func_body
    : '{' func_body_statement_list '}' { PTRACE("func_body:{func_body_statement_list}"); }
    | '{' '}' { PTRACE("func_body:{}"); }
    ;

else_clause
    : ELSE '(' expression ')' func_body { PTRACE("else_clause:ELSE(expression) func_body"); }
    ;

else_clause_final
    : { PTRACE("else_clause_final:"); }
    | ELSE '(' ')' func_body { PTRACE("else_clause_final:ELSE() func_body"); }
    | ELSE func_body { PTRACE("else_clause_final:ELSE func_body"); }
    ;

else_clause_list
    : else_clause { PTRACE("else_clause_list:else_clause"); }
    | else_clause_list else_clause { PTRACE("else_clause_list:else_clause_list else_clause"); }
    ;

    /* expression stacks? */
if_clause
    : IF '(' expression ')' func_body { PTRACE("if_clause:IF (expression) func_body"); }
    ;

if_statement
    : if_clause else_clause_final { PTRACE("if_statement:if_clause else_clause_final"); }
    | if_clause else_clause_list else_clause_final { PTRACE("if_statement:if_clause else_clause_list else_clause_final"); }
    | error { PTRACE("if_statement:error"); }
    ;

while_statement
    : WHILE '(' expression ')' func_body { PTRACE("while_statement:WHILE(expression) func_body"); }
    | WHILE '(' ')' func_body { PTRACE("while_statement:WHILE() func_body"); }
    | WHILE func_body { PTRACE("while_statement:WHILE func_body"); }
    ;

do_statement
    : DO func_body WHILE '(' expression ')' { PTRACE("do_statement:DO func_body WHILE(expression)"); }
    | DO func_body WHILE '(' ')' { PTRACE("do_statement:DO func_body WHILE()"); }
    | DO func_body WHILE { PTRACE("do_statement:DO func_body WHILE"); }
    ;

break_statement
    : BREAK { PTRACE("break_statement:BREAK"); }
    ;

continue_statement
    : CONTINUE { PTRACE("continue_statement:CONTINUE"); }
    ;

yield_statement
    : YIELD { PTRACE("yield_statement:YIELD"); }
    | YIELD '(' expression ')' { PTRACE("yield_statement:YIELD(expression)"); }
    ;

trace_statement
    : TRACE { PTRACE("trace_statement:TRACE"); }
    | TRACE '(' ')' { PTRACE("trace_statement:TRACE()"); }
    | TRACE '(' STRG ')' { PTRACE("trace_statement:TRACE(STRG)"); }
    ;

return_statement
    : RETURN { PTRACE("return_statement:RETURN"); }
    | RETURN '(' ')' { PTRACE("return_statement:RETURN()"); }
    | RETURN '(' expression ')' { PTRACE("return_statement:RETURN(expression)"); }
    ;

print_statement
    : PRINT { PTRACE("print_statement:PRINT"); }
    | PRINT '(' ')' { PTRACE("print_statement:PRINT()"); }
    | PRINT '(' expression ')' { PTRACE("print_statement:PRINT(expression)"); }
    ;

exit_statement
    : EXIT { PTRACE("exit_statement:EXIT"); }
    | EXIT '(' expression ')' { PTRACE("exit_statement:EXIT(expression)"); }
    ;

type_statement
    : TYPE '(' expression ')' { PTRACE("type_statement:TYPE(expression)"); }
    ;

assignment
    : SYMBOL '=' expression { PTRACE("assignment:SYMBOL=expression"); }
    | SYMBOL ADD_ASSIGN expression { PTRACE("assignment:SYMBOL ADD_ASSIGN expression"); }
    | SYMBOL SUB_ASSIGN expression { PTRACE("assignment:SYMBOL SUB_ASSIGN expression"); }
    | SYMBOL MUL_ASSIGN expression { PTRACE("assignment:SYMBOL MUL_ASSIGN expression"); }
    | SYMBOL DIV_ASSIGN expression { PTRACE("assignment:SYMBOL DIV_ASSIGN expression"); }
    | SYMBOL MOD_ASSIGN expression { PTRACE("assignment:SYMBOL MOD_ASSIGN expression"); }
    ;

func_body_statement
    : symbol_decl { PTRACE("func_body_statement:symbol_decl"); }
    | assignment { PTRACE("func_body_statement:assignment"); }
    | if_statement { PTRACE("func_body_statement:if_statement"); }
    | while_statement { PTRACE("func_body_statement:while_statement"); }
    | do_statement { PTRACE("func_body_statement:do_statement"); }
    | function_reference { PTRACE("func_body_statement:function_reference"); }
    | trace_statement { PTRACE("func_body_statement:trace_statement"); }
    | return_statement { PTRACE("func_body_statement:return_statement"); }
    | print_statement { PTRACE("func_body_statement:print_statement"); }
    | exit_statement { PTRACE("func_body_statement:exit_statement"); }
    | type_statement { PTRACE("func_body_statement:type_statement"); }
    | break_statement { PTRACE("func_body_statement:break_statement"); }
    | continue_statement { PTRACE("func_body_statement:continue_statement"); }
    | yield_statement { PTRACE("func_body_statement:yield_statement"); }
    | func_body { PTRACE("func_body_statement:func_body"); }
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
