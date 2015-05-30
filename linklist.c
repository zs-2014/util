#include <stdlib.h> 
#include <stdio.h>

#include "linklist.h"
#ifndef null
#define null NULL
#endif

static int cmp (const void *v1, const void *v2)
{
    return v1 == v2 ;
}

static void *dup_value(void *v)
{
    return v;
}

static void free_value(void *v)
{
    free(v) ;
}

LinkedList *malloc_linked_list()
{
    LinkedList *llst = (LinkedList *)calloc(1, sizeof(LinkedList)) ;
    llst ->cmp = cmp ;
    llst ->free_value = free_value ;
    llst ->dup_value = dup_value ;
    return llst ;
}

void deconstruct(LinkedList *llst)
{
    if(!llst)
        return ;

    ListNode *node = llst ->head ;
    while(node)
    {
        llst ->free_value(node ->value) ;
        ListNode *tmp = node ->next ;
        free(node) ;
        node = tmp ;
    }
}

void free_linked_list(LinkedList *llst)
{
    deconstruct(llst) ;
    free(llst) ;
}

static ListNode *malloc_linked_list_node()
{
    return (ListNode *)calloc(1, sizeof(ListNode)) ;
}

void *append_to_linked_list(LinkedList *llst, void *value)
{
    if(!llst || !value)
        return null ;
    ListNode *node = malloc_linked_list_node() ;
    node ->value = llst ->dup_value(value) ;
    if(llst ->tail) 
    {
        llst ->tail ->next = node ;
        llst ->tail = node ;
    }
    else
    {
        llst ->head = node ;
        llst ->tail = node ;
    }
    llst ->count++ ;
    return value ;
}

ListNode *find_linked_list_node(LinkedList *llst, const void *value)
{
    if(!llst)
        return null ;
    ListNode *node = llst ->head ;
    while(node)
    {
        if(llst ->cmp(node ->value, value) == 0)
            return node ;
        node = node ->next ;
    }
    return null ;
}

int is_in_linked_list(LinkedList *llst, const void *value)
{
    return find_linked_list_node(llst, value) != null;
}

void *find_in_linked_list(LinkedList *llst, const void *value)
{
   ListNode *node =  find_linked_list_node(llst, value); 
   return node ? node ->value : null ;
}

void *delete_from_linked_list(LinkedList *llst, void *value)
{
    if(!llst || !llst ->head || !value)
        return 0;

    ListNode *node = llst ->head ; 
    if(llst ->cmp(node ->value, value) == 0)
    {
        if(llst ->head == llst ->tail)
            llst ->head = llst ->tail = null ;
        else
            llst ->head = llst ->head ->next;
        llst ->count-- ;
        value = node ->value ;
        //llst ->free_value(node ->value) ;
        free(node) ;
        return value; 
    }

    ListNode *pre_node = llst ->head ;
    node = pre_node ->next ;
    while(node)
    {
        if(llst ->cmp(node ->value, value) == 0)
            break ;
        pre_node = pre_node ->next;
        node = node ->next ;
    }
    if(!node)
        return null;
    pre_node ->next = node ->next;
    value = node ->value ;
    //llst ->free_value(node ->value) ;
    free(node) ;
    if(node == llst ->tail)
    {
        llst ->tail = pre_node ;
        llst ->tail ->next = null ;
    }
    llst ->count-- ;
    return value;
}

int count_linked_list(LinkedList *llst)
{
    return llst ? llst ->count : -1;
}

#if 0
#include <string.h>
#include <time.h>

void test_append(int cnt)
{
    LinkedList *llst = malloc_linked_list() ;
    llst ->cmp = strcmp ;
    llst ->dup_value = strdup;
    int i = 0 ;
    char buff[1024] = {0} ;
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%d", i+1) ;
        append_to_linked_list(llst, buff) ;
        if(!is_in_linked_list(llst, buff))
        {
            printf("missing value: %s\n", buff) ;
        }
    }
    free_linked_list(llst) ;
}

void test_delete(int cnt)
{
    LinkedList *llst = malloc_linked_list() ;
    llst ->cmp = strcmp ;
    llst ->dup_value = strdup;
    int i = 0 ;
    char buff[1024] = {0} ;
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%d", i+1) ;
        append_to_linked_list(llst, buff) ;
        if(!is_in_linked_list(llst, buff))
        {
            printf("missing value: %s\n", buff) ;
        }
    }
    srand(time(null)) ;  
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%d", rand() % cnt+1) ;
        printf("delete %s from linked list\n", buff) ;
        if(delete_from_linked_list(llst, buff) == 0) 
        {
            printf("missing value: %s\n", buff) ;
        }
    }
    free_linked_list(llst) ;
}

int main(int argc, char *argv[])
{
    //test_append(atoi(argv[1])) ;
    test_delete(atoi(argv[1])) ;
    return 0 ;
}

#endif
