#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stddef.h>
#include "linkhash.h"

#ifndef nullptr
#define nullptr NULL
#endif

#define default_hash_table_size 1024

static void *dup_key(void *key)
{
    return key ;
}

static void free_key(void *key)
{
    free(key) ;
}

static int cmp_key(const void *k1, const void *k2)
{
    return !(k1 == k2) ;
}

static size_t hash(const void *key)
{
    return ((ptrdiff_t)key * 0x9e370001UL) >> 4 ;
}

static void *dup_value(void *value)
{
    return value ;
}

static void free_value(void *value)
{
    free(value) ;
}

static LinkHashEntry *look_up(const LinkHashTable *lh_table, const void *key)
{
    if(!lh_table || !key)    
        return 0;
    size_t idx = lh_table ->hash(key) % (lh_table ->total_sz) ;
    LinkHashEntry *entry = lh_table ->dict+idx ;
    while(entry)
    {
        if(entry ->key && lh_table ->cmp(entry ->key, key) == 0)
            return entry ;
        entry = entry ->next ;
    }
    return nullptr ;
}

static LinkHashEntry *malloc_link_hash_entry()
{
    return (LinkHashEntry *)calloc(1, sizeof(LinkHashEntry)) ;
}

int is_exist(const LinkHashTable *lh_table, const void *key)
{
    return look_up(lh_table, key) != nullptr ;
}

static void *__set(LinkHashTable *lh_table, void *key, void *value)
{
    size_t idx = lh_table ->hash(key) % (lh_table ->total_sz) ;
    LinkHashEntry *entry = lh_table ->dict+idx ; 
    int flag = 0 ;
    while(entry ->next)
    {
        if(!entry ->key || lh_table ->cmp(entry ->key, key) == 0) 
        {
            if(entry ->key) 
                flag = 1 ;
            break ;
        }
        entry = entry ->next;
    }
    if(!entry ->key)
    {
        if(entry != lh_table ->dict+idx)
            lh_table ->collisions++ ;
        entry ->key = key ;
        entry ->value = value ;
    }
    else
    {
        lh_table ->collisions++ ;
        //already has one
        if(flag == 1)
        {
            lh_table ->free_key(entry ->key) ;
            lh_table ->free_value(entry ->value) ;
            entry ->key = key ;
            entry ->value = value ;
        }
        else
        {
            entry ->next = malloc_link_hash_entry() ;
            entry ->next ->key = key ;
            entry ->next ->value = value ;
        }
    }
    lh_table ->use_sz++ ;
    return value ;

}
void *set(LinkHashTable *lh_table, void *key, void *value)
{
    if(!lh_table || !key || !value)
        return nullptr ;
    
    if(lh_table ->total_sz * 0.6 < lh_table ->use_sz)
        rehash(lh_table, lh_table ->total_sz<<1) ;
    return __set(lh_table, lh_table ->dup_key(key), lh_table ->dup_value(value)) ;
}

void *get(LinkHashTable *lh_table, void *key) 
{
    if(!lh_table || !key)
        return nullptr ;
    LinkHashEntry *entry = look_up(lh_table, key) ;
    return entry != nullptr ? entry ->value : nullptr;
}

void *pop(LinkHashTable *lh_table, void *key)
{
    if(!lh_table || !key)
        return nullptr ;
    LinkHashEntry *entry = look_up(lh_table, key) ;
    if(!entry)
        return nullptr ;

    void *value = entry ->value ;
    lh_table ->use_sz-- ;
    lh_table ->free_key(entry ->key) ;
    entry ->key = nullptr ;
    entry ->value = nullptr ;
    return value;
}

LinkHashTable *rehash(LinkHashTable *lh_table, size_t sz)
{
    if(!lh_table) 
        return nullptr ;
    sz = sz ? sz : default_hash_table_size ;
    LinkHashEntry *entry = (LinkHashEntry *)calloc(sz, sizeof(LinkHashEntry)) ;
    LinkHashEntry *old_entry = lh_table ->dict ;
    size_t old_total_sz = lh_table ->total_sz ;
    size_t i = 0 ;
    lh_table ->total_sz = sz ;
    lh_table ->dict = entry ;
    lh_table ->collisions = 0 ;
    lh_table ->use_sz = 0 ;
    for(i=0; i < old_total_sz; i++ )
    {
        LinkHashEntry *entry = old_entry[i].next ;
        while(entry)
        {
            __set(lh_table, entry ->key, entry ->value) ;
            LinkHashEntry *tmp = entry ->next ;
            free(entry) ;
            entry = tmp ;
        } 
        if(old_entry[i].key)
            __set(lh_table, old_entry[i].key, old_entry[i].value) ;
    }
    free(old_entry) ;
    return lh_table ;
}

LinkHashTable *malloc_link_hash_table(size_t sz)
{
    LinkHashTable *lh_table = (LinkHashTable *)calloc(1, sizeof(LinkHashTable)) ;
    lh_table ->cmp = cmp_key ;
    lh_table ->dup_key = dup_key ;
    lh_table ->free_key = free_key ;

    lh_table ->hash = hash ;

    lh_table ->dup_value = dup_value ;
    lh_table ->free_value = free_value ;
    return rehash(lh_table, sz) ;
}

void free_link_hash_table(LinkHashTable *lh_table)
{
    if(!lh_table)
        return ;
    size_t i = 0 ;
    for(i=0; i < lh_table ->total_sz; i++)
    {
        LinkHashEntry *entry = lh_table ->dict[i].next;
        while(entry)
        {
            if(entry ->key) 
                lh_table ->free_key(entry ->key) ;
            if(entry ->value)
                lh_table ->free_value(entry ->value) ;
            LinkHashEntry *tmp = entry ->next ;
            free(entry) ;
            entry = tmp ;
        }
        if(lh_table ->dict[i].key)
            lh_table ->free_key(lh_table ->dict[i].key) ;
        if(lh_table ->dict[i].value)
            lh_table ->free_value(lh_table ->dict[i].value) ;
    }
    free(lh_table ->dict) ;
    free(lh_table) ;
}

#include <string.h>
#include <time.h>
#include "hash.h"

void test_string_set(int cnt)
{
    LinkHashTable *lh_table = malloc_link_hash_table(0) ;
    lh_table ->dup_key = strdup ;
    lh_table ->dup_value = strdup ;
    lh_table ->cmp = strcmp ;
    lh_table ->hash = BKDRHash;
    int i = 0;
    char key_buff[1024] = {"key-test"} ;
    srand(time(NULL)) ; 
    for(i=0; i < cnt; i++)
    {
        sprintf(key_buff, "key-test:%u", (size_t)rand()) ;  
        set(lh_table, key_buff, key_buff) ;
        char *value = get(lh_table, key_buff) ;
        if(strcmp(value, key_buff) != 0)
        {
            printf("missing find key:%s\n", key_buff) ;
        }
    }
    printf("total_sz:%u use_sz:%u collisions:%u\n", lh_table ->total_sz, lh_table ->use_sz, lh_table ->collisions) ;
    free_link_hash_table(lh_table) ;
}


void test_set(int cnt)
{
    LinkHashTable *lh_table = malloc_link_hash_table(0) ; 
    int i = 0 ;
    for(i=0; i < cnt; i++)
    { 
        int *key = (int *)malloc(sizeof(int)) ;
        *key = i + 1;
        int *value = (int *)malloc(sizeof(int)) ;
        *value = 10 + i ;
        set(lh_table, key,  value) ;
        int *old_value = get(lh_table, key) ;
        if(old_value == nullptr || *value != *old_value)
        {
            printf("missing key:%d value:%d\n", *key, *value) ;
        }
    }
    printf("total_sz:%u use_sz:%u collisions:%u\n", lh_table ->total_sz, lh_table ->use_sz, lh_table ->collisions) ;
    free_link_hash_table(lh_table) ;
}

void test_pop(int cnt)
{
    LinkHashTable *lh_table = malloc_link_hash_table(0) ; 
    int i = 0 ;
    int **p = (int **)malloc(sizeof(int *)*cnt) ;
    for(i=0; i < cnt; i++)
    { 
        int *key = (int *)malloc(sizeof(int)) ;
        *key = i + 1;
        int *value = (int *)malloc(sizeof(int)) ;
        *value = 10 + i ;
        set(lh_table, key,  value) ;
        p[i] = key ;
    }
    for(i=0; i < cnt; i++)
    {
        int *value = pop(lh_table, p[i]) ;
        if(value == nullptr)
        {
            printf("missing key:%s\n", *p[i]) ;
        }
        free(value) ;
        value = get(lh_table, p[i]) ;
        if(value != nullptr)
        {
            printf("key not pop\n") ;
        }
    }
    printf("total_sz:%u use_sz:%u collisions:%u\n", lh_table ->total_sz, lh_table ->use_sz, lh_table ->collisions) ;
    free_link_hash_table(lh_table) ;
    free((void *)p) ;
}

int main(int argc, char *argv[])
{
    test_string_set(atoi(argv[1])) ;
    test_set(atoi(argv[1])) ;
    test_pop(atoi(argv[1])) ;
    return 0 ;
}
