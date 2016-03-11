#ifndef __link_list__h
#define __link_list__h

#include "common.h"

typedef struct _ListNode
{
	struct _ListNode *next ;
    struct _ListNode *pre ;
    char item[0];
}ListNode;


typedef struct LinkedList
{
	ListNode *head ;
	ListNode *tail ;
    ITEM_CMP_FUNC cmp;
    FREE_ITEM_FUNC free_item;
    //int (*cmp)(const void *v1, const void *v2);
    //void (*free_item)(void *item);
    unsigned int item_sz;
	int count ;
}LinkedList ;

#ifdef __cplusplus
extern "C"{
#endif

#define for_each_in_linked_list(llst, val)                             \
    do{                                                                \
	    ListNode *____node = llst ->head ;                             \
        for(; ____node; ____node=____node->next)                       \
        {val=____node->item;

#define end_for_each_in_linked_list                                    \
    }}while(0);


#define reverse_for_each_in_linked_list(llst, val)                     \
    do{                                                                \
        ListNode *____node = llst ->tail;                              \
        for(; ____node; ____node=____node->pre)                        \
        {val=____node->item;

#define end_reverse_for_each_in_linked_list                            \
    }}while(0);


extern void init_linked_list(LinkedList *llst, unsigned int item_sz, ITEM_CMP_FUNC cmp, FREE_ITEM_FUNC free_item);
extern void free_item_linked_list(LinkedList *llst);
extern LinkedList *new_linked_list(unsigned int item_sz) ;
extern void free_linked_list(LinkedList *llst) ;
extern int append_to_linked_list(LinkedList *llst, void *value) ;
extern int insert_before_linked_list(LinkedList *llst, void *value, const void *cmp_val);
extern int insert_after_linked_list(LinkedList *llst, void *value, const void *cmp_val);
extern int reset_value_linked_list(LinkedList *llst, void *value, const void *cmp_val);
extern int len_linked_list(LinkedList *llst) ;
extern int is_in_linked_list(LinkedList *llst, const void *value) ;
extern void *front_linked_list(LinkedList *llst);
extern int dequeue_linked_list(LinkedList *llst);
extern int enqueue_linked_list(LinkedList *llst, void *val);

extern void *top_linked_list(LinkedList *llst);
extern int pop_linked_list(LinkedList *llst);
extern int push_linked_list(LinkedList *llst, void *value);

extern void *find_in_linked_list(LinkedList *llst, const void *value) ;
extern int remove_linked_list(LinkedList *llst, void *value) ;

#ifdef __cplusplus
}
#endif

#endif
