#include <stdlib.h> 
#include <string.h>
#include <stdio.h>

#include "linklist.h"

static ListNode *new_linked_list_node(LinkedList *llst)
{
    return (ListNode *)CALLOC(1, sizeof(ListNode)+llst ->item_sz); ;
}

static int do_remove_linked_list(LinkedList *llst, ListNode *node)
{
    ListNode *pre = node ->pre;    
    ListNode *next  = node ->next;
    if(pre)
        pre ->next = next;
    if(next)
        next ->pre = pre;
    if(node == llst ->head)
        llst ->head = next;
    if(node == llst ->tail)
        llst ->tail = pre;
    if(llst ->free_item)
        llst ->free_item(node ->item);
    FREE(node);
    llst ->count -= 1;
    return 0; 
}

static int do_insert_linked_list(LinkedList *llst, void *val, ListNode *pre, ListNode *next)
{
    ListNode *node = new_linked_list_node(llst);
    if(!node)
        return -1;
    memmove(node ->item, val, llst ->item_sz);

    if(pre)
        pre ->next = node;
    else
        llst ->head = node;
    node ->pre = pre;

    if(next)
        next ->pre = node;
    else
        llst ->tail = node;
    node ->next = next;
    llst ->count += 1;
    return 0;
}

void init_linked_list(LinkedList *llst, unsigned int item_sz, ITEM_CMP_FUNC cmp, FREE_ITEM_FUNC free_item)
{
    if(!llst)
        return ;
    llst ->item_sz = item_sz;
    llst ->cmp = cmp;
    llst ->free_item = free_item;
}

LinkedList *new_linked_list(unsigned item_sz)
{
    LinkedList *llst = (LinkedList *)CALLOC(1, sizeof(LinkedList)) ;
    llst ->item_sz = item_sz;
    llst ->head = llst ->tail =  NULL;
    llst ->count = 0;
    llst ->cmp  = NULL;
    llst ->free_item = NULL;
    return llst ;
}

void free_item_linked_list(LinkedList *llst)
{
    if(!llst)
        return ;

    ListNode *node = llst ->head;
    while(node)
    {
        llst ->head = node ->next ; 
        if(llst ->free_item)
            llst ->free_item(node ->item);
        FREE(node);
        node = llst ->head;
    } 
    llst ->count = 0;
    llst ->head = llst ->tail = NULL;
}

void free_linked_list(LinkedList *llst)
{
    free_item_linked_list(llst);
    FREE(llst);
}


int append_to_linked_list(LinkedList *llst, void *value)
{
    if(!llst || !value)
        return -1;
    return do_insert_linked_list(llst, value, llst ->tail, NULL);
}

ListNode *find_linked_list_node(LinkedList *llst, const void *value)
{
    if(!llst || !llst ->cmp)
        return NULL ;
    ListNode *node = llst ->head ;
    while(node)
    {
        if(llst ->cmp(node ->item, value) == 0)
            return node ;
        node = node ->next ;
    }
    return NULL ;
}

//#define do_remove_linked_list(llst, node) {                  \
//    ListNode *pre = (node) ->pre;                            \
//    ListNode *next  = (node) ->next;                         \
//    pre && (pre ->next = next);                              \
//    next && (next ->pre = pre);                              \
//    (node == llst ->head) && (llst ->head = next);           \
//    (node == llst ->head) && (llst ->tail = pre);            \
//    (llst ->free_item) && (llst ->free_item((node)->item));  \
//    FREE(node);                                              \
//    llst ->count -= 1;                                       \
//}


int insert_before_linked_list(LinkedList *llst, void *value, const void *cmp_val)
{
    if(!llst || !value)
        return -1;
    ListNode *node = find_linked_list_node(llst, cmp_val);
    if(!node)
        return -1;
    return do_insert_linked_list(llst, value, node ->pre, node);
}

int insert_after_linked_list(LinkedList *llst, void *value, const void *cmp_val)
{
    if(!llst || !value)
        return -1;
    ListNode *node = find_linked_list_node(llst, cmp_val);
    if(!node)
        return -1;
    return do_insert_linked_list(llst, value, node, node ->next);
}

void *front_linked_list(LinkedList *llst)
{
    if(!llst || !llst ->head)
        return NULL;
    return (void *)llst ->head->item;
}

int dequeue_linked_list(LinkedList *llst)
{
    if(!llst || !llst ->head)
        return -1;
    return do_remove_linked_list(llst, llst ->head);
}

int enqueue_linked_list(LinkedList *llst, void *val)
{
    if(!llst || !val)
        return -1;
    return do_insert_linked_list(llst, val, llst ->tail, NULL);
}

void *top_linked_list(LinkedList *llst)
{
    if(!llst || !llst ->head)
        return NULL;
    return (void *)llst ->tail ->item;
}

int pop_linked_list(LinkedList *llst)
{
    if(!llst || !llst ->head)
        return -1;
    return do_remove_linked_list(llst, llst ->tail);
}

int push_linked_list(LinkedList *llst, void *value)
{
    return enqueue_linked_list(llst, value);
}

int reset_value_linked_list(LinkedList *llst, void *value, const void *cmp_val)
{
    if(!llst || !value)
        return -1;
    ListNode *node = find_linked_list_node(llst, cmp_val);
    if(!node)
        return -1;
    if(llst ->free_item)
        llst ->free_item(node ->item);
    memmove(node ->item, value, llst ->item_sz);
    return 0;
}

int is_in_linked_list(LinkedList *llst, const void *value)
{
    return find_linked_list_node(llst, value) != NULL;
}

void *find_in_linked_list(LinkedList *llst, const void *value)
{
   ListNode *node =  find_linked_list_node(llst, value); 
   return node ? node ->item : NULL ;
}

int remove_linked_list(LinkedList *llst, void *value)
{
    if(!llst || !llst ->head)
        return -1;
    ListNode *node = find_linked_list_node(llst, value);
    if(!node)
        return -1;
    return do_remove_linked_list(llst, node); 
}

int len_linked_list(LinkedList *llst)
{
    return llst ? llst ->count : -1;
}

#ifdef TEST
#include <time.h>

void print_linked_list(LinkedList *llst)
{
    const void *val = NULL;
    printf("len=%d [", len_linked_list(llst));
    for_each_in_linked_list(llst, val)
        printf("%s,", val);
    end_for_each_in_linked_list
    printf("]\n");
}

void print_int_linked_list(LinkedList *llst)
{   
    const void *val = NULL;
    printf("len=%d [", len_linked_list(llst));
    for_each_in_linked_list(llst, val)
        printf("%d,", *(int*)val);
    end_for_each_in_linked_list
    printf("]\n");
}

void free_item(void *item)
{
    printf("delete item:[%s]\n", item);
}

int cmp(const void *v1, const void* v2)
{
    //printf("v1=[%s]\n v2=[%s]\n", v1, v2);
    return strcmp(v1, v2);
}

int cmp_int(const void *v1, const void *v2)
{
    return !(*(int*)v1 == *(int*)v2);
}

void free_int_item(void *v1)
{
    printf("delete item:[%d]\n", *(int*)v1);
}

void test_stack(int cnt)
{
    LinkedList *llst = new_linked_list(sizeof(int));
    llst ->cmp = cmp_int;
    llst ->free_item = free_int_item;
    int ret = pop_linked_list(llst);
    printf("pop_linked_list ret=%d\n", ret);
    int i = 0;
    for(i=0; i < cnt; i++)
    {
        push_linked_list(llst, &i);
    }
    printf("pop[");
    while(len_linked_list(llst))
    {
        int *p = top_linked_list(llst);
        pop_linked_list(llst);
        printf("%d,", *p);

    }
    printf("]\n");
    for(i=0; i < cnt; i++)
    {
        push_linked_list(llst, &i);
    }
    free_linked_list(llst);
}

void test_queue(int cnt)
{
    LinkedList *llst = new_linked_list(sizeof(int));
    llst ->cmp = cmp_int;
    llst ->free_item = free_int_item;
    int ret = dequeue_linked_list(llst);
    printf("dequeue_linked_list ret=%d\n", ret);
    int i = 0;
    for(i=0; i < cnt; i++)
    {
        enqueue_linked_list(llst, &i);
    }
    printf("pop[");
    while(len_linked_list(llst))
    {
        int *p = front_linked_list(llst);
        dequeue_linked_list(llst);
        printf("%d,", *p);

    }
    printf("]\n");
    for(i=0; i < cnt; i++)
    {
        enqueue_linked_list(llst, &i);
    }
    free_linked_list(llst);
}

void test_insert(int cnt)
{
    LinkedList *llst = new_linked_list(sizeof(int));
    llst ->cmp = cmp_int;
    llst ->free_item = free_int_item;
    int i = 0;
    int ret = insert_after_linked_list(llst, &i, NULL);
    print_linked_list(llst);
    for(i=0; i < cnt; i++)
    {
        append_to_linked_list(llst, &i);
    }
    print_int_linked_list(llst);
    int j = -1;
    i = 0;
    insert_after_linked_list(llst, &j, &i);
    print_int_linked_list(llst);
    j = 10;
    i = 9;
    insert_after_linked_list(llst, &j, &i);
    print_int_linked_list(llst);
    i = 5;
    j = -5;
    insert_after_linked_list(llst, &j, &i);
    print_int_linked_list(llst);
    free_linked_list(llst);

    printf("----------------------insert_before_linked_list--------------------\n");
    llst = new_linked_list(sizeof(int));
    llst ->cmp = cmp_int;
    llst ->free_item = free_int_item;
    i = 0;
    ret = insert_before_linked_list(llst, &i, NULL);
    print_linked_list(llst);
    for(i=0; i < cnt; i++)
    {
        append_to_linked_list(llst, &i);
    }
    print_int_linked_list(llst);
    j = -1;
    i = 0;
    insert_before_linked_list(llst, &j, &i);
    print_int_linked_list(llst);
    j = 10;
    i = 9;
    insert_before_linked_list(llst, &j, &i);
    print_int_linked_list(llst);
    i = 5;
    j = -5;
    insert_before_linked_list(llst, &j, &i);
    print_int_linked_list(llst);
    free_linked_list(llst);
}

void test_append(int cnt)
{
    LinkedList *llst = new_linked_list(8) ;
    int i = 0 ;
    char buff[1024] = {0} ;
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%02d", i+1) ;
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
    LinkedList *llst = new_linked_list(8) ;
    llst ->cmp = cmp;
    llst ->free_item = free_item;
    int i = 0 ;
    char buff[1024] = {0} ;
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%02d", i+1) ;
        append_to_linked_list(llst, buff) ;
        if(!is_in_linked_list(llst, buff))
        {
            printf("missing value: %s\n", buff) ;
        }
    }
    print_linked_list(llst);
    remove_linked_list(llst, "test:01");
    print_linked_list(llst);
    remove_linked_list(llst, "test:10");
    print_linked_list(llst);
    remove_linked_list(llst, "test:05");
    print_linked_list(llst);
    srand(time(NULL)) ;  
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%02d", rand() % cnt+1) ;
        if(remove_linked_list(llst, buff) == -1) 
        {
            printf("missing value: %s\n", buff) ;
        }
        //print_linked_list(llst);
    }
    print_linked_list(llst);
    for(i=0; i < cnt; i++)
    {
        sprintf(buff, "test:%02d", i+1) ;
        append_to_linked_list(llst, buff) ;
        print_linked_list(llst);
        if(!is_in_linked_list(llst, buff))
        {
            printf("missing value: %s\n", buff) ;
        }
    }
    print_linked_list(llst);
    free_linked_list(llst) ;
}

int main(int argc, char *argv[])
{
    //test_append(atoi(argv[1])) ;
    //test_delete(atoi(argv[1])) ;
    //test_stack(atoi(argv[1]));
    //test_queue(atoi(argv[1]));
    test_insert(atoi(argv[1]));
    return 0 ;
}

#endif
