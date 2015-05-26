#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "skiplist.h"

#define default_skip_list_layers 16

#ifndef nullptr
#define nullptr NULL
#endif


int default_cmp(const void *val1, const void *val2)
{
    return val1 == val2 ;
}

void *default_dup_value(void *value)
{
    return value ;
}

void default_free_value(void *value)
{
    free(value) ;
}

SkipList *malloc_skip_list(size_t layers)
{
    layers = layers ? layers: default_skip_list_layers ;    
    SkipList *splist = (SkipList *)calloc(1, sizeof(SkipList)) ;
    splist ->layers_num = layers ;
    splist ->nodes = (SkipListNode *)calloc(layers, sizeof(SkipListNode)) ;
    splist ->__pre_nodes = (SkipListNode **)calloc(layers, sizeof(SkipListNode *)) ;
    splist ->cmp = default_cmp ;
    splist ->dup_value = default_dup_value ;
    splist ->free_value = default_free_value ;
    SkipListNode *head_node = splist ->nodes;
    int i = 1 ;
    for(i=1; i < layers; i++)
    {
        splist ->nodes[i].down = head_node ;
        head_node = splist ->nodes+i ;
    }
    return splist;
}

void free_skip_list(SkipList *splist)
{
    SkipListNode *node = splist ->nodes[0].next;
    while(node)
    {
        splist ->free_value(node ->value) ;
        SkipListNode *tmp = node ->next ;
        free(node) ;
        node = tmp  ;
    }
    free(splist ->__pre_nodes) ;
    free(splist ->nodes) ;
    free(splist) ; 
}

static int gen_pre_node(SkipList *splist, const void *value)
{
    SkipListNode *pre_node = splist ->nodes+splist ->layers_num - 1 ;
    size_t layers = splist ->layers_num ;
    while(pre_node)
    {
        --layers ;
        SkipListNode *curr_node = pre_node ->next ;
        while(curr_node)
        {
            int ret = splist ->cmp(curr_node ->value, value) ;
            if(ret == 0)
                return 0;
            else if(ret > 0)
                break ;
            pre_node = curr_node ;
            curr_node = curr_node ->next ;
        }
        splist ->__pre_nodes[layers] = pre_node ;
        pre_node = pre_node ->down ;
    }
    return 1;
}

int random_level()
{
    int level = 1 ; 
    while((rand() & 0xffff) < (0xffff >> 1))
        level += 1 ;
    return level ;
}

void *insert_into_skip_list(SkipList *splist, void *value) 
{
    if(!splist)
        return nullptr ; 
    if(gen_pre_node(splist, value) == 0)
        return nullptr ;
    size_t layers = random_level() ; 
    layers = layers > splist ->layers_num ? splist ->layers_num : layers ;
    SkipListNode *insert_nodes = (SkipListNode *)calloc(layers, sizeof(SkipListNode)) ;
    value = splist ->dup_value(value) ;
    while(layers > 0)
    {
        --layers ;
        SkipListNode *next = splist ->__pre_nodes[layers] ->next ;
        insert_nodes[layers].value = value ;

        splist ->__pre_nodes[layers] ->next = &insert_nodes[layers];
        insert_nodes[layers].pre = splist ->__pre_nodes[layers] ;

        insert_nodes[layers].next = next;
        if(next)
            next ->pre = &insert_nodes[layers] ;

        if(layers != 0)
            insert_nodes[layers].down = insert_nodes+layers-1 ;
        else
            insert_nodes[0].down = nullptr ;
    }
    splist ->curr_node_num++ ;
    return value ;
}

static SkipListNode *find_skip_list_node(SkipList *splist, const void *value)
{
    SkipListNode *pre_node = splist ->nodes+splist ->layers_num - 1 ;
    while(pre_node)
    {
        SkipListNode *curr_node = pre_node ->next ;
        while(curr_node)
        {
            int ret = splist ->cmp(curr_node ->value, value) ;
            if(ret == 0)
                return curr_node ;
            else if(ret > 0)
                break ;
            pre_node = curr_node ;
            curr_node = curr_node ->next ;
        }
        pre_node = pre_node ->down ;
    }
    return nullptr ;
}

void *delete_from_skip_list(SkipList *splist, const void *value)
{
    if(!splist)
        return nullptr ;
    SkipListNode *node = find_skip_list_node(splist, value) ;
    if(!node)
        return nullptr ;
    void *value1 = node ->value ;
    while(node)
    {
        node ->pre ->next = node ->next ;
        if(node ->next)
            node ->next ->pre = node ->pre ;
        //第一层释放节点空间
        if(!node ->down)
            free(node) ;
        node = node ->down ;
    }
    return value1 ;
}


void *find_in_skip_list(SkipList *splist, const void *value)
{
    SkipListNode *node = find_skip_list_node(splist, value);
    return node ? node ->value : nullptr ;
}

int is_in_skip_list(SkipList *splist, const void *value)
{
    return find_skip_list_node(splist, value) != nullptr ; 
}

void print_skip_list(SkipList *splist)
{
    int i = 0 ;
    for(i=0; i < splist ->layers_num; i++)
    {
        SkipListNode *node = splist->nodes[i].next ;
        printf("Layer %d: [Null]->", i+1) ;
        while(node) 
        {
            printf("[%s]->", node ->value) ;
            node = node ->next;
        }
        printf("[Null]\n") ;
    }
}

#if 1
#include <string.h>

void test_insert_to(int cnt)
{
    char buff[128] = {0} ;
    int i = 0 ;
    SkipList *splist = malloc_skip_list(0) ;
    splist ->cmp = strcmp ;
    splist ->dup_value = strdup ;

    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%d", i+1) ;
        insert_into_skip_list(splist, buff) ;
        if(find_in_skip_list(splist, buff) == nullptr)
        {
            printf("missing value:[%s]\n", buff) ;
        }
    }
    //print_skip_list(splist) ;
    for(i=cnt; i < cnt+10; i++)
    {
        sprintf(buff, "test:%d", i+1) ;
        if(find_in_skip_list(splist, buff) == nullptr)
        {
            printf("missing value:[%s]\n", buff) ;
        }
    }
    free_skip_list(splist) ;
}

void test_delete_node(int cnt)
{    
    char buff[128] = {0} ;
    int i = 0 ;
    SkipList *splist = malloc_skip_list(0) ;
    splist ->cmp = strcmp ;
    splist ->dup_value = strdup ;
    srand(time(nullptr)) ;
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%d", i+1) ;
        insert_into_skip_list(splist, buff) ;
        if(find_in_skip_list(splist, buff) == nullptr)
        {
            printf("missing value:[%s]\n", buff) ;
        }
    }
    print_skip_list(splist) ;
    cnt += 10 ;
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%d", i+1) ;
        void *value = delete_from_skip_list(splist, buff) ;
        if(value != nullptr)
        {
            printf("delete value [%s]\n", buff) ; 
            free(value) ;
        }
        else
            printf("not found value:[%s]\n", buff) ;
        if(find_in_skip_list(splist, buff) != nullptr)
        {
            printf("cann't delete value:[%s]\n", buff) ;
        }
    }
    print_skip_list(splist) ;
    free_skip_list(splist) ;

}

int main(int argc, char *argv[])
{
   //test_insert_to(atoi(argv[1])) ; 
   test_delete_node(atoi(argv[1])) ;
}

#endif
