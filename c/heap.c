#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

struct Heap
{
    int *ay ;
    int curr_sz ;
    int total_sz ;
}Heap;

void delete_heap(struct Heap *hp)
{
    if(!hp)
        return ;
    if(hp ->ay)
        free(hp ->ay) ;
    hp ->ay = NULL ;
    hp ->total_sz = 0 ;
    hp ->curr_sz = 0 ;
    free(hp) ;
    hp = NULL ;
}

struct Heap *new_heap(int sz)
{
    struct Heap *hp = (struct Heap *)calloc(1, sizeof(struct Heap)) ;
    if(!hp)
        return hp ;
    if(!sz)
        return hp ;
    hp ->ay = (int *)calloc(1, sz*sizeof(int)) ;
    if(!hp ->ay)
    {
        delete_heap(hp) ;
        return NULL ;
    }
    hp ->curr_sz = 0;
    hp ->total_sz = sz ;
    return hp ;
}

int realloc_heap(struct Heap *hp, size_t need_sz)
{
    if(!hp)
        return -1 ;
    if(hp ->curr_sz + need_sz <= hp ->total_sz)    
        return 0 ;
    int *ay = (int *)calloc(1, sizeof(int)*(hp ->total_sz + need_sz + 32)) ;
    if(!ay)
        return -1 ;
    hp ->total_sz = hp ->total_sz + need_sz + 32 ;
    if(hp ->curr_sz)
        memcpy(ay, hp ->ay, sizeof(int)*(hp ->curr_sz)) ;
    free(hp ->ay) ;
    hp ->ay = ay ;
    return 0 ;
}

void __adjust_heap(int *ay, int node_idx, int high)
{
    while(node_idx < high)
    {
        int lchld_idx = node_idx*2+1;
        int rchld_idx = lchld_idx + 1; 
        if(lchld_idx < high && rchld_idx < high && ay[lchld_idx] > ay[rchld_idx])
            lchld_idx++ ;
        if(lchld_idx < high && ay[lchld_idx] < ay[node_idx])
        {
            int tmp = ay[node_idx] ;
            ay[node_idx] = ay[lchld_idx] ;
            ay[lchld_idx] = tmp ;
            node_idx = lchld_idx ;
        }
        else break ;
    }
}

int heapify(struct Heap *hp)
{
    int  len = hp ->curr_sz/2 ;
    for(; len >= 0; --len)
        __adjust_heap(hp ->ay, len, hp ->curr_sz) ;
    return 0 ;
}

int heap_peek_top(struct Heap *hp)
{
    if(!hp || hp ->curr_sz <= 0)
        return INT32_MIN ;
    return hp ->ay[0] ;
}

int heap_pop(struct Heap *hp)
{
    if(!hp || hp ->curr_sz <= 0)    
        return INT32_MIN ;
    int min_v = hp ->ay[0] ; 
    hp ->curr_sz-- ;
    hp ->ay[0] = hp ->ay[hp ->curr_sz] ;
    __adjust_heap(hp ->ay, 0, hp ->curr_sz) ;
    return min_v ;
}

int heap_push(struct Heap *hp, int v)
{
    if(!hp)
        return -1 ;
    if(realloc_heap(hp, 1) < 0)
        return -1 ;
    hp ->ay[hp ->curr_sz] = v ;
    int *ay = hp ->ay ;
    int chld_idx = hp ->curr_sz ;
    hp ->curr_sz++ ;
    while(chld_idx > 0)
    {
        int parent_idx = (chld_idx-1)/2 ;
        if(ay[chld_idx] >= ay[parent_idx])
            break ;
        int tmp = ay[chld_idx] ;
        ay[chld_idx] = ay[parent_idx] ;
        ay[parent_idx] = tmp ;
        chld_idx = parent_idx ;
    }
    return 0;
}

#include <time.h>
int test()
{
    struct Heap *hp = new_heap(100000) ;
    int i = 0 ;
    srand(time(NULL)) ;
    for(i=0; i< hp ->total_sz; i++)
    {
        int v = rand() ;
        heap_push(hp, v) ;
        fprintf(stderr, "%d\n", v) ;
    }
    for(i=0; i< hp ->total_sz; i++)
    {
        fprintf(stdout, "%d\n", heap_pop(hp)) ;
    }
    delete_heap(hp) ;
    return 0 ;
}

int main(int argc, char *argv[])
{
    test() ;
    return 0 ;
}
