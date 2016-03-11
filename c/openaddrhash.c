#include<stdlib.h>
#include<stdio.h>
#include <string.h>

#include "openaddrhash.h"
#define MIN_TABLE_SIZE 8

static struct OpenAddrHashTable *rehash(struct OpenAddrHashTable *oah_table) ;
static void *set_without_dup(struct OpenAddrHashTable *oah_table, void *key, void *val, unsigned hash) ;

static void *dummy_value = (void *)-1 ;

static int default_cmp_key(const void *key1, const void *key2)
{
    return !(key1 == key2) ;     
}

static void *default_dup_key(void *key)
{
    return key ;
}

static void default_free_key(void *key)
{
    return ;
}

static void *default_dup_value(void *value)
{
    return value ;
}

static void default_free_value(void *value)
{
    return ;
}

static unsigned default_hash(const void *key)
{
    return (unsigned)key ;
}

struct OpenAddrHashTable *new_open_addr_hash_table()
{
    struct OpenAddrHashTable *oah_table = (struct OpenAddrHashTable *)calloc(1, sizeof(struct OpenAddrHashTable));
    if(!oah_table)
        return NULL ;
    oah_table ->entrys = (struct Entry *)calloc(MIN_TABLE_SIZE, sizeof(struct Entry)) ;
    if(!oah_table ->entrys)
    {
        free_open_addr_hash_table(oah_table) ;
        return NULL;
    }
    oah_table ->mask = 7 ; //8 - 1 ;
    oah_table ->cmp_key = default_cmp_key ;
    oah_table ->dup_key = default_dup_key ;
    oah_table ->free_key = default_free_key ;
    oah_table ->dup_value = default_dup_value ;
    oah_table ->free_value = default_free_value ;
    oah_table ->hash = default_hash ;
    return oah_table ;
}

void free_open_addr_hash_table(struct OpenAddrHashTable *oah_table)
{
    if(!oah_table)
        return ;
    unsigned i = 0 ;
    struct Entry *entrys = oah_table ->entrys ;
    for(i=0; i < oah_table ->used; i++)
    {
        if(!entrys[i].value || entrys[i].value == dummy_value)
            continue ;
        oah_table ->free_key(entrys[i].key) ;
        oah_table ->free_value(entrys[i].value) ;
    }
    free(entrys) ;
    free(oah_table) ;
}

static struct Entry *look_up_entry(struct OpenAddrHashTable *oah_table, 
                                   const void *key, 
                                   unsigned hash,
                                   unsigned *collisions)
{
    printf("look_up_key: [%s]", key) ; 
    unsigned __ = 0 ;
    if(!collisions)
        collisions = &__;
    *collisions = 0 ;
    unsigned perturb = hash ;
    unsigned i = hash & oah_table ->mask ;
    printf("hash [%u] ", hash) ;
    printf("-->%d", i) ;
    struct Entry *entrys = oah_table ->entrys ; 
    struct Entry *dummy_entry = NULL ;
    if(entrys[i].value == NULL)
        return entrys + i ; 
    else if(entrys[i].value != dummy_value)
    {
        if(hash == entrys[i].hash && oah_table ->cmp_key(entrys[i].key, key) == 0)
            return entrys + i ;
    }
    else 
        dummy_entry = entrys+i ;
    *collisions += 1 ;
    while(1)
    {
        //i*5 + i + 1 + perturb
        i = (i << 2) + i + 1 + perturb ; 
        perturb >>= 5 ;
        unsigned idx = i & oah_table ->mask ;
        printf("--->%d", idx) ;
        if(entrys[idx].value == NULL)
            return dummy_entry ? dummy_entry : entrys + idx ;
        else if(entrys[idx].value != dummy_value)
        {
            if(entrys[idx].hash == hash && oah_table ->cmp_key(entrys[idx].key, key) == 0)
                return entrys + idx ;
        }
        else if(dummy_entry == NULL)
            dummy_entry = entrys + idx;
        *collisions += 1;
    }
    return NULL ;
}

static void *set_without_dup(struct OpenAddrHashTable *oah_table, 
                             void *key, 
                             void *val,
                             unsigned hash)
{
    unsigned collisions = 0 ;
    struct Entry *entry = look_up_entry(oah_table, key, hash, &collisions) ; 
    if(!entry ->value || entry ->value == dummy_value)
    {
        entry ->value = val ;
        entry ->key = key ;
        entry ->hash = hash ;
        oah_table ->used += 1 ;
    }
    else
    {
        //oah_table ->free_key(entry ->key) ;
        //entry ->key = key ;
        oah_table ->free_key(key) ;
        oah_table ->free_value(entry ->value) ;
        entry ->value = val ;
        entry ->hash = hash ;
    }
    return val ;
}

static struct OpenAddrHashTable *rehash(struct OpenAddrHashTable *oah_table)
{
    unsigned new_sz = 0 ; 
    unsigned used = oah_table ->used+1 ;
    unsigned total_sz = oah_table ->mask + 1;
    //增加容量
    if(3*used >= total_sz*2)
        new_sz = total_sz << 1;
    //减少容量
    //used < 2/3*total_sz
    else if(3*used < total_sz)
        new_sz = total_sz >> 1 ;
    if(new_sz == 0)
        return oah_table ;
    new_sz = new_sz > MIN_TABLE_SIZE ? new_sz : MIN_TABLE_SIZE ;
    struct Entry *entrys = (struct Entry *)calloc(new_sz, sizeof(struct Entry)) ;
    if(!entrys)
        return oah_table ;
    struct Entry *old_entrys = oah_table ->entrys ;
    oah_table ->entrys = entrys ;
    oah_table ->mask = new_sz - 1;
    oah_table ->used = 0 ;
    unsigned i = 0 ;
    for(i=0; i< total_sz; i++)
    {
        if(old_entrys[i].value && old_entrys[i].value != dummy_value)
            set_without_dup(oah_table, old_entrys[i].key, old_entrys[i].value, old_entrys[i].hash); 
    }
    free(old_entrys) ;
    return oah_table ;
}


void *set(struct OpenAddrHashTable *oah_table, void *key, void *val)
{
    if(!oah_table)
        return NULL ;
    key = oah_table ->dup_key(key) ;
    val = oah_table ->dup_value(val) ;
    unsigned hash = oah_table ->hash(key) ;     
    val = set_without_dup(oah_table, key, val, hash);
    rehash(oah_table);
    return val ;
}

void *get(struct OpenAddrHashTable *oah_table, const void *key)
{
    if(!oah_table)
        return NULL ;
    unsigned hash = oah_table ->hash(key) ;
    struct Entry *entry = look_up_entry(oah_table, key, hash, NULL) ;
    if(entry && entry ->value != dummy_value)
        return entry ->value ;
    return NULL ;
}

void *get_with_default(struct OpenAddrHashTable *oah_table, 
                       const void *key, 
                       void *default_val)
{
    void *val = get(oah_table, key) ;
    return val ? val: default_val ;
}

void *pop(struct OpenAddrHashTable *oah_table, const void *key)
{
    if(!oah_table)
        return NULL ;
    unsigned hash = oah_table ->hash(key) ;
    struct Entry *entry = look_up_entry(oah_table, key, hash, NULL) ;
    void *old_val = NULL ;
    if(entry && entry ->value && entry ->value != dummy_value)
    {
        old_val = entry ->value ;
        oah_table ->free_key(entry ->key) ;
        entry ->value = dummy_value ;
        oah_table ->used -= 1 ;
        rehash(oah_table);
    }
    return old_val;
}

#ifdef __OPENADDRHASH__
unsigned hash(const char *str)
{
    //31 131 1313 and so on
    unsigned v = 0 ;
    while(*str != '\0')
        v = (v << 5) - v + *str++ ;

    return v ;
}

#include <stdio.h>
void test_string()
{
    char *str1[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i"};
    struct OpenAddrHashTable *oah_table = new_open_addr_hash_table() ;
    oah_table ->dup_key = (void *(*)(void *))strdup ; 
    oah_table ->cmp_key = (int (*)(const void *, const void *))strcmp ;
    oah_table ->free_key = free ;
    oah_table ->hash = (unsigned (*)(const void *))hash ;
    int i = 0 ;
    for(i=0; i < sizeof(str1)/sizeof(char*)-2; i++)
    {
        set(oah_table, str1[i], (void *)1) ;
        printf("used:%d total_sz=%d\n", oah_table ->used, oah_table ->mask+1) ;
    }
    for(i=0; i < sizeof(str1)/sizeof(char*)-2; i++)
    {
        pop(oah_table, str1[i]) ;
        printf("used:%d total_sz=%d\n", oah_table ->used, oah_table ->mask+1) ;
    }

    for(i=0; i < sizeof(str1)/sizeof(char*)-2; i++)
    {
        set(oah_table, str1[i], (void *)1) ;
        printf("used:%d total_sz=%d\n", oah_table ->used, oah_table ->mask+1) ;
    }
    char *str2[] = {"str1", "oah_table", "dup_key", "dup_value", "free_key", "hash",
                   "mask", "set", "i", "total_sz", "str2", "used"} ;
    for(i=0; i < sizeof(str2)/sizeof(char*); i++)
    {
        set(oah_table, str2[i], (void *)1) ;
        printf("used:%d total_sz=%d\n", oah_table ->used, oah_table ->mask+1) ;
    }
    for(i=0; i < sizeof(str2)/sizeof(char*); i++)
    {
        pop(oah_table, str2[i]) ;
        printf("used:%d total_sz=%d\n", oah_table ->used, oah_table ->mask+1) ;
    }

    for(i=0; i < sizeof(str2)/sizeof(char*); i++)
    {
        set(oah_table, str2[i], (void *)1) ;
        printf("used:%d total_sz=%d\n", oah_table ->used, oah_table ->mask+1) ;
    }
    free_open_addr_hash_table(oah_table) ;
}

int main(int argc, char *argv[])
{
    printf("%p\n", dummy_value) ;
    test_string() ;
    return 0;
}
#endif
