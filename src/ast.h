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

typedef struct _namespace_ {
    Ast ast;
    String name;
    PtrLst* list;
} Namespace;

typedef struct _compound_name_ {
    Ast ast;
    PtrLst* symbol;
    String str;
    bool is_single;
} CompoundName;

typedef struct _type_spec_ {
    Ast ast;
    CompoundName* symbol; // NULL if it's a native type.
    int type;
    bool is_const;
} TypeSpec;

typedef struct _scope_operator_ {
    Ast ast;
    int type;
} ScopeOperator;

// TODO: add the guts of this.....
typedef struct _expression_ {
    Ast* ast;
} Expression;

typedef struct _variable_def_ {
    Ast ast;
    CompoundName* name;
    TypeSpec* type;
    Expression* expr;
} VariableDef;

typedef struct _function_body_ {
    Ast ast;
    PtrLst* list;
} FunctionBody;

// This is a list of VariableDef's
typedef struct _func_def_params_ {
    Ast ast;
    PtrLst* list;
} FuncDefParams;

typedef struct _function_def_ {
    Ast ast;
    TypeSpec* type;
    CompoundName* name;
    PtrLst* parms;
    FunctionBody* body;
} FunctionDef;

typedef struct _class_body_ {
    Ast ast;
    PtrLst* list;
} ClassBody;

typedef struct _class_def_ {
    Ast ast;
    String name;
    CompoundName* parent_name;
    ClassBody* body;
} ClassDef;

#endif /* _AST_H */
