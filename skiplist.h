#ifndef __skip__list__h
#define __skip__list__h

typedef struct SkipListNode
{		
	void *value ;
	struct SkipListNode *next ;
	struct SkipListNode *pre ;
	struct SkipListNode *down;
}SkipListNode;

typedef struct SkipList
{
	SkipListNode *nodes ;
	size_t layers_num ;
	size_t curr_node_num ;
	int (*cmp)(const void *val1, const void *val2) ;
	void * (*dup_value)(void *value) ;
	void (*free_value)(void *value) ;
	SkipListNode **__pre_nodes;
}SkipList;


#ifdef __cplusplus
extern "C"{
#endif

extern SkipList *malloc_skip_list(size_t layers) ;
extern void free_skip_list(SkipList *splist) ;
extern void *insert_into_skip_list(SkipList *splist, void *value) ;
extern void *delete_from_skip_list(SkipList *splist, const void *value) ;
extern int is_in_skip_list(SkipList *splist, const void *value) ;
extern void *find_in_skip_list(SkipList *splist, const void *value) ;

#ifdef __cplusplus
}
#endif

#endif
