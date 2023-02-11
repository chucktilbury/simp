
#include <stdio.h>
#include <assert.h>

#include "ptrlst.h"
#include "memory.h"

PtrLst* create_ptr_list() {

    PtrLst* lst = _alloc_ds(PtrLst);
    lst->cap = 1 << 3;
    lst->len = 0;
    lst->idx = 0;
    lst->list = _alloc_ds_array(void*, lst->cap);

    return lst;
}

PtrLstResult destroy_ptr_list(PtrLst* lst) {

    if(lst != NULL) {
        _free(lst->list);
        _free(lst);
        return PTRLST_OK;
    }

    return PTRLST_ERROR;
}

PtrLstResult append_ptr_list(PtrLst* lst, void* data) {

    assert(lst != NULL);
    if(lst->len + 1 > lst->cap) {
        lst->cap <<= 1;
        lst->list = _realloc_ds_array(lst->list, void*, lst->cap);
    }

    lst->list[lst->len] = data;
    lst->len++;

    return PTRLST_OK;
}

void* get_ptr_list(PtrLst* lst, int idx) {

    assert(lst != NULL);
    void* ptr = NULL;
    if(idx < lst->len) {
        ptr = lst->list[idx];
    }

    return ptr;
}

void* reset_ptr_list(PtrLst* lst) {

    assert(lst != NULL);
    lst->idx = 0;

    if(lst->len > 0)
        return lst->list[0];
    else
        return NULL;
}

void* iterate_ptr_list(PtrLst* lst) {

    assert(lst != NULL);
    void* ptr = get_ptr_list(lst, lst->idx);
    if(ptr) {
        lst->idx++;
        return ptr;
    }
    else
        return NULL;
}

PtrLstResult push_ptr_list(PtrLst* lst, void* data) {

    return append_ptr_list(lst, data);
}

void* pop_ptr_list(PtrLst* lst) {

    assert(lst != NULL);

    void* ptr = NULL;
    if(lst->len > 0) {
        lst->len--;
        ptr = lst->list[lst->len];
    }

    return ptr;
}

void* peek_ptr_list(PtrLst* lst) {

    assert(lst != NULL);

    void* ptr = NULL;
    if(lst->len > 0)
        ptr = lst->list[lst->len - 1];

    return ptr;
}

void** get_raw_ptr_list(PtrLst* lst) {

    return lst->list;
}

int get_len_ptr_list(PtrLst* lst) {

    return lst->len;
}
