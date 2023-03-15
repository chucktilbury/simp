%{
#include "parser.h"

#include <stdio.h>

void yyerror(const char* s);

typedef struct _file_stack_ {
    const char* fname;
    int line;
    int col;
    FILE* fptr;
    YY_BUFFER_STATE buffer;
    struct _file_stack_* next;
} FileStack;

#define MAX_INCL 15
FileStack* fstack = NULL;
int incl_depth = 0;

int bidx = 0;
char buffer[1024 * 64];
const char* file_name;

static void __append_char(char ch) {
    // TODO: bounds checking
    buffer[bidx] = ch;
    bidx++;
}

static void __append_str(const char* str) {
    // TODO: bounds checking
    strcpy(&buffer[bidx], str);
    bidx += strlen(str);
}

int yycolumn = 1;

/* This is executed before every action.
#define YY_USER_ACTION                                                   \
  fstack->line = prev_yylineno; fstack->col = yycolumn;                  \
  if (yylineno == prev_yylineno) yycolumn += yyleng;                     \
  else {                                                                 \
    for (yycolumn = 1; yytext[yyleng - yycolumn] != '\n'; ++yycolumn) {} \
    prev_yylineno = yylineno;                                            \
  }
 */
#define YY_USER_ACTION                                                   \
  fstack->col = yycolumn;                  \
  if (yylineno == prev_yylineno) yycolumn += yyleng;                     \
  else {                                                                 \
    for (yycolumn = 1; yytext[yyleng - yycolumn] != '\n'; ++yycolumn) {} \
    prev_yylineno = yylineno;                                            \
  }

%}

/* state definitions */
%x SQUOTES
%x DQUOTES
%option noinput nounput
%option yylineno
%option noyywrap

%%

   /* Any indented text before the first rule goes at the top of the lexer.  */
   // int start_line, start_column;
   int prev_yylineno = yylineno;

\n      { fstack->line++; fstack->col = 1; }
[ \t\r] {}

"break"     { return BREAK; }
"continue"  { return CONTINUE; }
"const"     { return CONST; }
"import"    { return IMPORT; }
"do"        { return DO; }
"else"      { return ELSE; }
"if"        { return IF; }
"return"    { return RETURN; }
"while"     { return WHILE; }
"true"      { return TRUE; }
"false"     { return FALSE; }
"yield"     { return YIELD; }
"exit"      { return EXIT; }
"trace"     { return TRACE; }
"print"     { return PRINT; }
"type"      { return TYPE; }
"namespace" { return NAMESPACE; }
"list"      { return LIST; }
"dict"      { return DICT; }
"struct"    { return STRUCT; }
"class"     { return CLASS; }
"try"       { return TRY; }
"except"    { return EXCEPT; }
"raise"     { return RAISE; }

"unsigned"|"uint"   { yylval.type = UNSIGNED; return UNSIGNED; }
"nothing"|"noth"    { yylval.type = NOTHING; return NOTHING; }
"string"|"strg"     { yylval.type = STRING; return STRING; }
"bool"|"boolean"    { yylval.type = BOOLEAN; return BOOLEAN; }
"float"             { yylval.type = FLOAT; return FLOAT; }
"integer"|"int"     { yylval.type = INTEGER; return INTEGER; }

"<="        { return LORE; }
">="        { return GORE; }
"=="        { return EQU; }
"!="        { return NEQU; }
"||"|"or"   { return OR; }
"&&"|"and"  { return AND; }
"+="        { return ADD_ASSIGN; }
"-="        { return SUB_ASSIGN; }
"*="        { return MUL_ASSIGN; }
"/="        { return DIV_ASSIGN; }
"%="        { return MOD_ASSIGN; }
"not"|"!"   { return NOT; }

"+"         { return '+'; }
"-"         { return '-'; }
"*"         { return '*'; }
"/"         { return '/'; }
"%"         { return '%'; }
"="         { return '='; }
"("         { return '('; }
")"         { return ')'; }
"<"         { return '<'; }
">"         { return '>'; }
"["         { return '['; }
"]"         { return ']'; }
"{"         { return '{'; }
"}"         { return '}'; }
"."         { return '.'; }
","         { return ','; }
"^"         { return '^'; }

[a-zA-Z_][a-zA-Z_0-9]* {
        yylval.symbol = (char*)strdup(yytext);
        return SYMBOL;
    }

    /*
    [a-zA-Z_][a-zA-Z_0-9]*(\.[a-zA-Z_][a-zA-Z_0-9]*)+ {
        yylval.symbol = (char*)strdup(yytext);
        return COMPOUND;
    }
    */

[0-9]+ {
        yylval.inum = (double)strtol(yytext, NULL, 10);
        return INUM;
    }

    /* recognize a float */
    /* ([0-9]*\.)?[0-9]+([Ee][-+]?[0-9]+)? { */
([0-9]*\.)?[0-9]+ {
        yylval.fnum = strtod(yytext, NULL);
        return FNUM;
    }

0[Xx][0-9a-fA-F]+ {
        yylval.unum = (double)strtol(yytext, NULL, 16);
        return UNUM;
    }

    /* double quoted strings have escapes managed */
\"  {
        bidx = 0;
        memset(buffer, 0, sizeof(buffer));
        BEGIN(DQUOTES);
    }

<DQUOTES>\" {
        yylval.str = (char*)strdup(buffer);
        BEGIN(INITIAL);
        return STRG;
    }

    /* the short rule matches before the long one does */
<DQUOTES>\\n { __append_char('\n'); }
<DQUOTES>\\r { __append_char('\r'); }
<DQUOTES>\\e { __append_char('\x1b'); }
<DQUOTES>\\t { __append_char('\t'); }
<DQUOTES>\\b { __append_char('\b'); }
<DQUOTES>\\f { __append_char('\f'); }
<DQUOTES>\\v { __append_char('\v'); }
<DQUOTES>\\\\ { __append_char('\\'); }
<DQUOTES>\\\" { __append_char('\"'); }
<DQUOTES>\\\' { __append_char('\''); }
<DQUOTES>\\\? { __append_char('\?'); }
<DQUOTES>\\.  { __append_char(yytext[1]); }
<DQUOTES>\\[0-7]{1,3} { __append_char((char)strtol(yytext + 1, 0, 8)); }
<DQUOTES>\\[xX][0-9a-fA-F]{1,3} { __append_char((char)strtol(yytext + 2, 0, 16)); }
<DQUOTES>[^\\\"\n]*  { __append_str(yytext); }
<DQUOTES>\n     { fstack->line++; fstack->col = 1; } // yylineno++; } /* track line numbers, but strip new line */

    /* single quoted strings are absolute literals */
\'  {
        bidx = 0;
        memset(buffer, 0, sizeof(buffer));
        BEGIN(SQUOTES);
    }

<SQUOTES>\' {
        yylval.str = (char*)strdup(buffer);
        BEGIN(INITIAL);
        return STRG;
    }

<SQUOTES>[^\\'\n]*  { __append_str(yytext); }
<SQUOTES>\\.    { __append_str(yytext); }
<SQUOTES>\n     {
        __append_str(yytext);
        //yylineno++;
        fstack->line++; fstack->col = 1;
    } /* don't strip new lines */

.   {
        printf("Warning: unrecognized character: %c (0x%02X)\n", yytext[0], yytext[0]);
        //return yytext[0];
    }

    /* comments */
";".*\n { fstack->line++; fstack->col = 1; }

<<EOF>> {

        incl_depth--;

        FileStack* tmp = fstack;

#ifdef ENABLE_PARSER_TRACE
        fprintf(stderr, ">>>>>> closing file: %s\n", tmp->fname);
#endif

        free((void*)tmp->fname);
        fclose(tmp->fptr);
        free(tmp);

        fstack = fstack->next;

        if(fstack == NULL) {
            yyterminate();
        }
        else {
            yy_delete_buffer(YY_CURRENT_BUFFER);
            yy_switch_to_buffer(fstack->buffer);
        }
    }


%%

void open_file(const char *fname) {

#ifdef ENABLE_PARSER_TRACE
    fprintf(stderr, "<<<<<<< opening file: %s\n", fname);
#endif

    if(incl_depth > MAX_INCL) {
        fprintf(stderr, "FATAL ERROR: Maximum include depth exceeded\n");
        exit(1);
    }
    incl_depth++;

    FileStack* fs = malloc(sizeof(FileStack));
    fs->fname = strdup(fname);
    fs->line = 1;
    fs->col = 1;
    fs->next = NULL;

    yyin = fopen(fname, "r");
    if(yyin == NULL) {
        fprintf(stderr, "fatal error: cannot open input file: %s: %s\n", fname,
                strerror(errno));
        exit(1);
    }

    fs->fptr = yyin;
    yy_switch_to_buffer(yy_create_buffer(yyin, YY_BUF_SIZE));
    fs->buffer = YY_CURRENT_BUFFER;

    if(fstack != NULL)
        fs->next = fstack;

    fstack = fs;
}

int get_line_no() {

    if(fstack != NULL)
        return fstack->line;
    else
        return -1;
}

int get_col_no() {

    if(fstack != NULL)
        return fstack->col;
    else
        return -1;
}

const char* get_file_name() {

    if(fstack != NULL)
        return fstack->fname;
    else
        return "no file open";
}

int main(int argc, char** argv) {

    if(argc < 2) {
        fprintf(stderr, "Need input file");
        exit(1);
    }

    open_file(argv[1]);
    return yyparse();
}