#ifndef ___LINK_HASH__
#define ___LINK_HASH__

typedef struct LinkHashEntry
{
	void *key ;
	void *value ;
	struct LinkHashEntry *next ;
}LinkHashEntry; 

typedef struct LinkHashTable
{
	LinkHashEntry **dict;
	size_t total_sz ;
	size_t use_sz ;
    size_t collisions ;
	int (*cmp)(const void *, const void *) ;
	void* (*dup_key)(void *) ;
	void  (*free_key) (void *) ;

	void* (*dup_value)(void *) ;
	void (*free_value)(void *) ;
	size_t (*hash)(const void *) ;

}LinkHashTable;

#ifdef __cplusplus
extern "C" {
#endif

extern void free_link_hash_table(LinkHashTable *lh_table) ;  
extern LinkHashTable *malloc_link_hash_table(size_t sz) ;
extern LinkHashTable *rehash(LinkHashTable *lh_table, size_t sz) ;
extern void * set(LinkHashTable *lh_table, void *key, void *value) ;
extern void * get(LinkHashTable *lh_table, void *key) ;
extern void * pop(LinkHashTable *lh_table, void *key) ;

#ifdef __cplusplus
}
#endif

#endif
