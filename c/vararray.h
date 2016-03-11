#ifndef __var_array__h
#define __var_array__h

#include "linklist.h"
#define DEFAULT_VARARRAY_BLOCK_SZ 64

typedef struct _vararray
{
    unsigned int ay_block_sz;
    unsigned int item_sz;
    unsigned int total_sz;
    ITEM_CMP_FUNC cmp;
    FREE_ITEM_FUNC free_item;
    LinkedList *llst;
}VarArray;


#ifdef __cplusplus
extern "C"{
#endif


extern VarArray *new_vararray(unsigned int item_sz, int ay_block_sz, ITEM_CMP_FUNC cmp, FREE_ITEM_FUNC free_item);
extern void free_varray(VarArray *vay);

extern int append_vararray(VarArray *vay, void *val);
extern int remove_vararray(VarArray *vay, const void *cmp_val);

#ifdef __cplusplus
}
#endif

#endif
