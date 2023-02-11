#ifndef _PTRLST_H_
#define _PTRLST_H_

typedef enum {
    PTRLST_OK,
    PTRLST_ERROR,
    PTRLST_EMPTY,
} PtrLstResult;

typedef struct _ptr_lst_ {
    void** list;
    int cap;
    int len;
    int idx;
} PtrLst;

PtrLst* create_ptr_list();
PtrLstResult destroy_ptr_list(PtrLst* lst);

PtrLstResult append_ptr_list(PtrLst* lst, void* data);
void* get_ptr_list(PtrLst* lst, int idx);

void* reset_ptr_list(PtrLst* lst);
void* iterate_ptr_list(PtrLst* lst);

PtrLstResult push_ptr_list(PtrLst* lst, void* data);
void* pop_ptr_list(PtrLst* lst);
void* peek_ptr_list(PtrLst* lst);

void** get_raw_ptr_list(PtrLst* lst);
int get_len_ptr_list(PtrLst* lst);

#endif
