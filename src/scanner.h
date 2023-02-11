#ifndef _SCANNER_H
#define _SCANNER_H

#include "tokens.h"

void init_scanner(const char* fname);
Token* crnt_token();
void consume_token();

#ifdef ENABLE_SCANNER_TRACE
# define STRACE fprintf(stderr, "scanner\t%s()\t%s:%d:%d\n", \
            __func__, get_file_name(), get_line_no(), get_col_no())
#else
# define STRACE
#endif


#endif /* _SCANNER_H */
