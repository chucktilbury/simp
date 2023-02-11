#ifndef _PARSER_H_
#define _PARSER_H_

void* parser(const char* fname);

#ifdef ENABLE_PARSER_TRACE
# define PTRACE fprintf(stderr, "parser\t%s()\t%s:%d:%d\n", \
            __func__, get_file_name(), get_line_no(), get_col_no())
#else
# define PTRACE
#endif

#endif
