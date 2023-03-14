#ifndef _AST_H
#define _AST_H

#include <stdbool.h>
#include "strptr.h"
#include "ptrlst.h"

typedef enum {
    AST_NAMESPACE,
    AST_IMPORT,
    AST_TYPE_SPEC,
    AST_SCOPE_OPERATOR,
    AST_COMPOUND_NAME,
    AST_VAR_DEF,
    AST_FUNC_DEF,
    AST_FUNC_PARAMS,
    AST_FUNC_BODY,
    AST_CLASS_DEF,
    AST_CLASS_BODY,
} AstType;

typedef struct {
    AstType type;
} Ast;

typedef struct _tree_root_ {
    Ast ast;
    PtrLst* list;
} TreeRoot;

#include "reference.h"
#include "expressions.h"
#include "module.h"
#include "function.h"
#include "class.h"

#endif /* _AST_H */
