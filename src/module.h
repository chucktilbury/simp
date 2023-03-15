#ifndef _MODULE_H
#define _MODULE_H

typedef struct _namespace_ Namespace;
typedef struct _type_spec_ TypeSpec;
typedef struct _scope_operator_ ScopeOperator;
typedef struct _variable_def_ VariableDef;

void* namespace_definition();
void* variable_param_def();
void* variable_definition(TypeSpec* type, CompoundName* name);
void* import_definition();
void* type_spec();
void* scope_operator();

#endif /* _MODULE_H */
