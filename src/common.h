#ifndef _COMMON_H
#define _COMMON_H

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>
#include <string.h>

#include "errors.h"
#include "ptrlst.h"
#include "memory.h"
#include "strptr.h"

#ifdef ENABLE_PARSER_TRACE
# define PTRACE fprintf(stderr, "parser\t%s()\t%s:%d:%d\n", \
            __func__, get_file_name(), get_line_no(), get_col_no())
#else
# define PTRACE
#endif


#endif /* _COMMON_H */
