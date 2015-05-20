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
    LinkHashEntry *entry = lh_table ->dict[idx] ;
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

void *set(LinkHashTable *lh_table, void *key, void *value)
{
    if(!lh_table || !key || !value)
        return nullptr ;
    
    if(lh_table ->total_sz * 0.6 < lh_table ->use_sz)
        rehash(lh_table, lh_table ->total_sz<<1) ;
    LinkHashEntry *entry = nullptr ;
    size_t idx = lh_table ->hash(key) % (lh_table ->total_sz) ;
    LinkHashEntry *next = lh_table ->dict[idx] ;    
    //first key-value
    if(next == nullptr)
    {
        entry = malloc_link_hash_entry() ;
        entry ->key = lh_table ->dup_key(key) ;
        entry ->value = lh_table ->dup_value(value) ;
        lh_table ->dict[idx]  = entry ;
        lh_table ->use_sz++ ;
        return value ;
    }
    int flag = 0 ;
    lh_table ->collisions++ ;
    while(next ->next && next ->key != nullptr)
    {
        if(lh_table ->cmp(next ->key, key) == 0)
        {
            flag = 1 ;
            break ;
        }
        next = next ->next ;
    }
    if(next ->key == nullptr || flag)
    {
        if(next ->key)
            lh_table ->free_value(next ->value) ;
        else
            next ->key = lh_table ->dup_key(key) ;
        next ->value = lh_table ->dup_value(value) ;
    }
    else
    {
        entry = malloc_link_hash_entry();
        entry ->key = lh_table ->dup_key(key) ;
        entry ->value = lh_table ->dup_value(value) ;
        next ->next = entry ;
    }
    lh_table ->use_sz++ ;
    return value ;
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
    void *value = nullptr ;
    if(entry)
    {
        value = entry ->value ;
        lh_table ->use_sz-- ;
    }
    else
        entry ->value = nullptr ;

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
    LinkHashEntry **entry = (LinkHashEntry **)calloc(sz, sizeof(LinkHashEntry *)) ;
    LinkHashEntry **old_entry = lh_table ->dict ;
    size_t old_total_sz = lh_table ->total_sz ;
    size_t i = 0 ;
    lh_table ->total_sz = sz ;
    lh_table ->dict = entry ;
    lh_table ->collisions = 0 ;
    lh_table ->use_sz = 0 ;
    for(i=0; i < old_total_sz; i++ )
    {
        LinkHashEntry *entry = old_entry[i] ;
        while(entry)
        {
            set(lh_table, entry ->key, entry ->value) ;
            LinkHashEntry *tmp = entry ->next ;
            free(entry) ;
            entry = tmp ;
        }
    }
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
        LinkHashEntry *entry = lh_table ->dict[i] ;
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
    }
    free((void *)lh_table ->dict) ;
    free(lh_table) ;
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
    for(i=0; i < cnt; i++)
    { 
        int *key = (int *)malloc(sizeof(int)) ;
        *key = i + 1;
        int *value = (int *)malloc(sizeof(int)) ;
        *value = 10 + i ;
        set(lh_table, key,  value) ;
        int *old_value = pop(lh_table, key) ;
        if(old_value == nullptr || *value != *old_value)
        {
            printf("missing key:%d value:%d\n", *key, *value) ;
        }
        free(old_value) ;
        old_value = get(lh_table, key) ;
        if(old_value != nullptr)
        {
            printf("not pop out key:%d value:%d", *key, *old_value) ;
        }
    }
    printf("total_sz:%u use_sz:%u collisions:%u\n", lh_table ->total_sz, lh_table ->use_sz, lh_table ->collisions) ;
    free_link_hash_table(lh_table) ;
}

int main(int argc, char *argv[])
{
    //test_set(atoi(argv[1])) ;
    test_pop(atoi(argv[1])) ;
    return 0 ;
}
