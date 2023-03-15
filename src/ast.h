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
    AST_VAR_DEF,
    AST_FUNC_DEF,
    AST_FUNC_PARAMS,
    AST_FUNC_BODY,
    AST_CLASS_DEF,
    AST_CLASS_BODY,
    AST_EXPRESSION,
    AST_EXPR_LIST,
    AST_NAME,   // appears in a compound_reference
    AST_COMPOUND_NAME, // left side
    AST_ARRAY_REF,
    // AST_COMPOUND_REF_ELEM,
    AST_FUNCTION_REF,
    AST_COMPOUND_REF, // right side
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
