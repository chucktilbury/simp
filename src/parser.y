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

%token BREAK CONTINUE CONST LIST HASH NAMESPACE CLASS
%token DO ELSE IF RETURN INCLUDE
%token YIELD EXIT WHILE TRY EXCEPT RAISE
%token EQU NEQU LORE GORE OR AND NOT
%token ADD_ASSIGN SUB_ASSIGN MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN
%token UINT INT FLOAT NOTHING STRING BOOLEAN
%token PRINT TRACE TYPE INLINE TRUE FALSE

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
    : INCLUDE STRG { PTRACE("module_item:INCLUDE STRG"); open_file($2); }
    | namespace { PTRACE("module_item:namespace"); }
    ;

namespace
    : NAMESPACE SYMBOL '{' namespace_item_list '}' { PTRACE("namespace:NAMESPACE SYMBOL"); }
    | NAMESPACE SYMBOL '{' '}' { PTRACE("namespace:NAMESPACE SYMBOL {}"); }
    ;

namespace_item_list
    : namespace_item { PTRACE("namespace_item_list:namespace_item"); }
    | namespace_item_list namespace_item { PTRACE("namespace_item_list:add"); }
    ;

namespace_item
    : namespace { PTRACE(":"); }
    | symbol_definition { PTRACE("module_item:symbol_decl"); }
    | func_definition { PTRACE("module_item:func_definition"); }
    | class_definition { PTRACE("namespace_item:class_definition"); }
    ;

compound_name
    : SYMBOL { PTRACE("compound_name:SYMBOL"); }
    | compound_name '.' SYMBOL { PTRACE("compound_name:ADD"); }
    ;

symbol_reference
    : SYMBOL '[' expression ']' { PTRACE("symbol_reference:ARRAY"); }
    | SYMBOL '(' expression_list ')' { PTRACE("symbol_reference:FUNC"); }
    | SYMBOL '(' ')' { PTRACE("symbol_reference:FUNC()"); }
    | SYMBOL { PTRACE("symbol_reference:SYMBOL"); }
    ;

compound_reference
    : symbol_reference { PTRACE("compound_reference:compound_ref_item"); }
    | compound_reference '.' symbol_reference { PTRACE("compound_reference:ADD"); }
    ;

type_name
    : UINT { PTRACE("type_name:UINT"); }
    | INT { PTRACE("type_name:INT"); }
    | FLOAT { PTRACE("type_name:FLOAT"); }
    | STRING { PTRACE("type_name:STRING"); }
    | BOOLEAN { PTRACE("type_name:BOOLEAN"); }
    | compound_name { PTRACE("type_name:compound_name"); }
    ;

type_spec
    : type_name { PTRACE("type_spec:type_name"); }
    | CONST type_name { PTRACE("type_spec:CONST type_name"); }
    ;

literal_value
    : UNUM { PTRACE("constant_expression:UNUM"); }
    | FNUM { PTRACE("constant_expression:FNUM"); }
    | INUM { PTRACE("constant_expression:INUM"); }
    | STRG { PTRACE("constant_expression:STRG"); }
    | TRUE { PTRACE("constant_expression:TRUE"); }
    | FALSE { PTRACE("constant_expression:FALSE"); }
    ;

list_initalizer
    : '[' expression_list ']' { PTRACE("list_initalizer:[expression_list]"); }
    | '[' list_initalizer ']' { PTRACE("list_initalizer:[list_initalizer]"); }
    ;

hash_item
    : STRG ':' expression { PTRACE("hash_item:"); }
    ;

hash_item_list
    : hash_item { PTRACE("hash_item_list:hash_item"); }
    | hash_item_list ',' hash_item { PTRACE("hash_item_list:add"); }
    ;

hash_initializer
    : '[' hash_item_list ']' { PTRACE("hash_initializer:[hash_item_list]"); }
    | '[' hash_initializer ']' { PTRACE("hash_initializer:[hash_initializer]"); }
    ;

symbol_declaration
    : type_spec SYMBOL { PTRACE("symbol_declaration:type_spec SYMBOL"); }
    | LIST SYMBOL { PTRACE("symbol_decl:LIST SYMBOL"); }
    | HASH SYMBOL { PTRACE("symbol_decl:HASH SYMBOL"); }
    ;

symbol_init
    : type_spec SYMBOL '=' expression { PTRACE("symbol_init:symbol_intro=expression"); }
    | LIST SYMBOL '=' list_initalizer { PTRACE("symbol_init:LIST SYMBOL"); }
    | HASH SYMBOL '=' hash_initializer { PTRACE("symbol_init:HASH SYMBOL"); }
    ;

symbol_definition
    : symbol_declaration { PTRACE("symbol_definition:symbol_declaration"); }
    | symbol_init { PTRACE("symbol_definition:symbol_init"); }
    ;

symbol_definition_list
    : symbol_definition { PTRACE("symbol_intro_list:symbol_intro"); }
    | symbol_definition_list ',' symbol_definition { PTRACE("symbol_intro_list:symbol_intro_list,symbol_intro"); }
    ;

expression_factor
    : literal_value { PTRACE("expression_factor:constant_expression"); }
    | compound_reference { PTRACE("expression_factor:SYMBOL"); }
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
    | '<' type_spec '>' expression %prec CAST { PTRACE("expression:cast_spec expression %%prec CAST"); }
    | '(' expression ')' { PTRACE("expression:(expression)"); }
    ;

expression_list
    : expression { PTRACE("expression_list:expression"); }
    | expression_list ',' expression { PTRACE("expression_list:expression_list,expression"); }
    ;

class_item
    : symbol_definition { PTRACE("module_item:symbol_definition"); }
    | func_definition { PTRACE("module_item:func_definition"); }
    ;

class_body_list
    : class_item { PTRACE("class_body_list:"); }
    | class_body_list class_item { PTRACE("class_body_list:add"); }
    ;

class_body
    : '{' '}' { PTRACE("class_body:{}"); }
    | '{' class_body_list '}' { PTRACE("class_body:{list}"); }
    ;

class_definition
    : CLASS SYMBOL class_body { PTRACE("class_definition:"); }
    | CLASS SYMBOL '(' ')' class_body { PTRACE("class_definition:()"); }
    | CLASS SYMBOL '(' compound_name ')' class_body { PTRACE("class_definition:(name)"); }
    ;

function_reference
    : compound_name '(' expression_list ')' { PTRACE("function_reference:compound_name(expression_list)"); }
    | compound_name '(' ')' { PTRACE("function_reference:compound_name()"); }
    ;

func_decl_parameter_list
    : '(' symbol_definition_list ')' { PTRACE("func_decl_parameter_list:(symbol_intro_list)"); }
    | '(' ')' { PTRACE("func_decl_parameter_list:()"); }
    ;

func_definition
    : type_spec compound_name func_decl_parameter_list func_body { PTRACE("func_definition:type_spec SYMBOL func_decl_parameter_list func_body"); }
    | NOTHING compound_name func_decl_parameter_list func_body { PTRACE("func_definition:NOTHING SYMBOL func_decl_parameter_list func_body"); }
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
    : ELSE '(' ')' func_body { PTRACE("else_clause_final:ELSE() func_body"); }
    | ELSE func_body { PTRACE("else_clause_final:ELSE func_body"); }
    ;

else_clause_list
    : else_clause { PTRACE("else_clause_list:else_clause"); }
    | else_clause_list else_clause { PTRACE("else_clause_list:else_clause_list else_clause"); }
    ;

except_clause
    : EXCEPT '(' compound_name ')' func_body { PTRACE("except_clause:EXCEPT"); }
    ;

except_clause_list
    : except_clause { PTRACE("except_clause_list:"); }
    | except_clause_list except_clause { PTRACE("except_clause_list:add"); }
    ;

except_clause_final
    : EXCEPT '(' ')' func_body { PTRACE("except_clause_final:()"); }
    | EXCEPT func_body { PTRACE("except_clause_final:"); }
    ;

try_statement
    : TRY func_body except_clause_final { PTRACE("try_statement:final"); }
    | TRY func_body except_clause_list { PTRACE("try_statement:list"); }
    | TRY func_body except_clause_list except_clause_final { PTRACE("try_statement:list_final"); }
    ;

if_clause
    : IF '(' expression ')' func_body { PTRACE("if_clause:IF (expression) func_body"); }
    ;

if_statement
    : if_clause {}
    | if_clause else_clause_final { PTRACE("if_statement:if_clause else_clause_final"); }
    | if_clause else_clause_list else_clause_final { PTRACE("if_statement:if_clause else_clause_list else_clause_final"); }
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

raise_statement
    : RAISE { PTRACE("raise_statement:"); }
    | RAISE '(' ')' { PTRACE("raise_statement:()"); }
    | RAISE '(' compound_name ')' { PTRACE("raise_statement:(compound_name)"); }
    ;

inline_statement
    : INLINE '(' symbol_definition_list ')' '{' STRG '}' { PTRACE("inline_statement:"); }
    ;

assignment
    : compound_name '=' expression { PTRACE("assignment:SYMBOL=expression"); }
    | compound_name ADD_ASSIGN expression { PTRACE("assignment:SYMBOL ADD_ASSIGN expression"); }
    | compound_name SUB_ASSIGN expression { PTRACE("assignment:SYMBOL SUB_ASSIGN expression"); }
    | compound_name MUL_ASSIGN expression { PTRACE("assignment:SYMBOL MUL_ASSIGN expression"); }
    | compound_name DIV_ASSIGN expression { PTRACE("assignment:SYMBOL DIV_ASSIGN expression"); }
    | compound_name MOD_ASSIGN expression { PTRACE("assignment:SYMBOL MOD_ASSIGN expression"); }
    ;

func_body_statement
    : symbol_definition { PTRACE("func_body_statement:symbol_decl"); }
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
    | raise_statement { PTRACE("func_body_statement:raise_statement"); }
    | try_statement { PTRACE("func_body_statement:try_statement"); }
    | inline_statement { PTRACE("func_body_statement:inline_statement"); }
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
