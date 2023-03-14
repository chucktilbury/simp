#ifndef _FUNCTION_H
#define _FUNCTION_H

#include "ast.h"

typedef struct _function_body_ FunctionBody;
typedef struct _func_def_params_ FuncDefParams;
typedef struct _function_def_ FunctionDef;

void* function_definition(TypeSpec* type, CompoundName* name);
void* function_params();
void* function_body();

#endif /* _FUNCTION_H */
