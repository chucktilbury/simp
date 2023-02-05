/**
 * @file scanner.h
 *
 * @brief Scanner header.
 *
 */
#ifndef _SCANNER_H
#define _SCANNER_H

#include <stdio.h>

char* get_file_name();
int get_line_no();
int get_col_no();
void open_file(const char* fname);

/*
 * Defined by flex. Call one time to isolate a symbol and then use the global
 * symbol struct to access the symbol.
 */
extern int yylex();
extern int yyparse();
extern FILE* yyin;

// void yyerror(char *s, ...);
void yyerror(const char* s);

#endif /* _SCANNER_H */
