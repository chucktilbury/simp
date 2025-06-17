
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "ast.h"
#include "scanner.h"
#include "parser.h"
//#include "ast.h"
#include "cmdline.h"
#include "trace.h"


void cmdline(int argc, char** argv, char** env) {

    init_cmdline("simp", "FSA parser generator", "0.1");
    add_cmdline('v', "verbosity", "verbosity", "From 0 to 10. Print more information", "0", NULL, CMD_NUM|CMD_ARGS);
    add_cmdline('h', "help", NULL, "Print this helpful information", NULL, cmdline_help, CMD_NONE);
    add_cmdline('V', "version", NULL, "Show the program version", NULL, cmdline_vers, CMD_NONE);
    add_cmdline(0, NULL, NULL, NULL, NULL, NULL, CMD_DIV);
    add_cmdline(0, NULL, "files", "File name(s) to input", NULL, NULL, CMD_REQD|CMD_ANON);

    parse_cmdline(argc, argv, env);

    INIT_TRACE(NULL);
}

int main(int argc, char** argv, char** env) {

    cmdline(argc, argv, env);

    yyin = fopen(raw_string(get_cmd_opt("files")), "r");
    if(yyin == NULL) {
        fprintf(stderr, "cannot open input file \"%s\": %s\n", argv[1], strerror(errno));
        cmdline_help();
    }

    yyparse();

    return 0;
}
