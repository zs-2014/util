#ifndef __open_addr_hash__h
#define __open_addr_hash__h

struct Entry
{
    void *key ;
    void *value ;
    unsigned hash ;
};

struct OpenAddrHashTable
{
    unsigned mask; 
    unsigned used ;
    struct Entry *entrys ;

    int (*cmp_key)(const void *key1, const void *key2);
    void * (*dup_key)(void *key);
    void (*free_key)(void *key);

    void * (*dup_value)(void *val);
    void (*free_value)(void *val);
    unsigned (*hash)(const void *key);
};


#ifdef __cplusplus
extern "C" {
#endif

extern struct OpenAddrHashTable * new_open_addr_hash_table();
extern void free_open_addr_hash_table(struct OpenAddrHashTable *oah_table);
extern void *set(struct OpenAddrHashTable *oah_table, void *key, void *val);
extern void *get(struct OpenAddrHashTable *oah_table, const void *key);
extern void *get_with_default(struct OpenAddrHashTable *oah_table, const void *key, void *default_val);
extern void *pop(struct OpenAddrHashTable *oah_table, const void *key);

#ifdef __cplusplus
}
#endif

#endif
