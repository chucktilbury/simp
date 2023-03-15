#ifndef _REFERENCE_H
#define _REFERENCE_H

typedef struct _compound_name_ CompoundName;
typedef struct _array_reference_ ArrayReference;
typedef struct _compound_reference_element_ CompoundReferenceElement;
typedef struct _function_reference_ FunctionReference;
typedef struct _compound_reference_ CompoundReference;

void* compound_name();
void* compound_reference();
void* compound_reference_element();
void* function_reference();
void* array_reference();

#endif /* _REFERENCE_H */
