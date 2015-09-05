#ifndef __link_list__h
#define __link_list__h
typedef struct ListNode
{
	void *value ;
	struct ListNode *next ;
}ListNode;


typedef struct LinkedList
{
	ListNode *head ;
	ListNode *tail ;
	int count ;
	int (*cmp)(const void *, const void *) ;
	void* (*dup_value)(void *) ;
	void (*free_value) (void *) ;
}LinkedList ;

#ifdef __cplusplus
extern "C"{
#endif

#define for_each_in_linked_list(llst, val)\
	ListNode *____node = llst ->head ;\
    for(; ____node&&(val=____node->value); ____node=____node->next)


extern LinkedList *malloc_linked_list() ;
extern void deconstruct(LinkedList *llst) ;
extern void free_linked_list(LinkedList *llst) ;
extern void *append_to_linked_list(LinkedList *llst, void *value) ;
extern int count_linked_list(LinkedList *llst) ;
extern int is_in_linked_list(LinkedList *llst, const void *value) ;
extern void *find_in_linked_list(LinkedList *llst, const void *value) ;
extern void *delete_from_linked_list(LinkedList *llst, void *value) ;
#ifdef __cplusplus
}
#endif

#endif
