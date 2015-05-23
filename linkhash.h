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
	LinkHashEntry *dict;
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

#define for_each_in_link_hash_table(lh_table, k, v)\
    size_t ___i = 0;\
    LinkHashEntry *___entry = 0;\
    for(;lh_table&&___i < lh_table ->total_sz; ___i++)\
        for(___entry=lh_table->dict+___i;\
			___entry&&((k=___entry->key)&&(v=___entry->value));\
			___entry = ___entry ->next)


extern void free_link_hash_table(LinkHashTable *lh_table) ;  
extern LinkHashTable *malloc_link_hash_table(size_t sz) ;
extern LinkHashTable *rehash(LinkHashTable *lh_table, size_t sz) ;
extern void * set(LinkHashTable *lh_table, void *key, void *value) ;
extern void * get(LinkHashTable *lh_table, const void *key) ;
extern void * pop(LinkHashTable *lh_table, const void *key) ;
int is_in_link_hash_table(const LinkHashTable *lh_table, const void *key) ;

#ifdef __cplusplus
}
#endif

#endif
