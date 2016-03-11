#include <stdlib.h>
#include <string.h>

#include "array.h"
#include "vararray.h"

static int cmp_null_array(const void *v1, const void *v2)
{
    return len_array(*(const Array**)v1);
}

static int remove_null_array(VarArray *vay)
{
    return remove_linked_list(vay ->llst, NULL); 
}

static void _free_array(void *ay)
{
    free_array(*(Array **)ay);
}

static Array *get_last_array(VarArray *vay)
{
    Array **pp_ay = (Array **)top_linked_list(vay ->llst); 
    //没有元素或者没有空间了
    if(!pp_ay || !*pp_ay || len_array(*pp_ay) == vay ->ay_block_sz)
    {
        Array *p_ay = new_array(vay ->ay_block_sz, vay ->item_sz);
        if(!p_ay)
            return NULL;
        p_ay ->cmp = vay ->cmp;
        p_ay ->free_item = vay ->free_item;
        push_linked_list(vay ->llst, (void *)&p_ay);
        pp_ay = &p_ay;
    }
    return *pp_ay;
}

VarArray *new_vararray(unsigned int item_sz, int ay_block_sz, ITEM_CMP_FUNC cmp, FREE_ITEM_FUNC free_item)
{
    if(ay_block_sz <= 0)
        ay_block_sz = DEFAULT_VARARRAY_BLOCK_SZ; 
    VarArray *vay = (VarArray *)CALLOC(1, sizeof(VarArray));
    if(!vay)
        return NULL;
    vay ->item_sz = item_sz;
    vay ->ay_block_sz = ay_block_sz;
    vay ->llst = new_linked_list(sizeof(Array *));
    if(!vay ->llst)
    {
        free_varray(vay);    
        return NULL;
    }
    vay ->llst ->cmp = cmp_null_array;
    vay ->llst ->free_item = _free_array;
    vay ->cmp = cmp;
    vay ->total_sz = 0;
    vay ->free_item = free_item;
    return vay;
}

void free_varray(VarArray *vay)
{
    if(!vay)
        return;
    free_linked_list(vay ->llst);
    vay ->llst = NULL;
    free(vay);
}

int append_vararray(VarArray *vay, void *val)
{
    if(!vay || !val)
        return -1;
    int ret = append_array(get_last_array(vay), val);
    if(ret == 0)
        vay ->total_sz += 1;
    return ret;
}

int remove_nth_vararray(VarArray *vay, const void *cmp_val, int n)
{
    if(!vay|| n < 0)
        return -1;
    void *ay = NULL;
    LinkedList *llst = vay ->llst;
    for_each_in_linked_list(llst, ay)
    end_for_each_in_linked_list
    return -1;
}

int remove_all_vararray(VarArray *vay, const void *cmp_val)
{
    if(!vay)
        return -1;
    void *ay = NULL;
    LinkedList *llst = vay ->llst;
    for_each_in_linked_list(llst, ay)
        int n = len_array(*(Array **)ay);
        remove_all_array(*(Array **)ay, cmp_val);
        vay ->total_sz -= n - len_array(*(Array **)ay);
    end_for_each_in_linked_list
    return 0;
}

int remove_vararray(VarArray *vay, const void *cmp_val)
{
    if(!vay)
        return -1;
    void *ay = NULL;
    LinkedList *llst = vay ->llst;
    for_each_in_linked_list(llst, ay)
        if(remove_array(*(Array **)ay, cmp_val) == 0)
        {
            vay ->total_sz -= 1;     
            remove_null_array(vay);
            return 0;
        }
    end_for_each_in_linked_list
    return -1;
}

int remove_at_vararray(VarArray *vay, int idx)
{
    if(!vay)
        return -1;
    if(idx < 0)
        idx += (int)vay ->total_sz;
    if(idx < 0 || idx >= vay ->total_sz)
        return -1;
    void *ay = NULL;
    LinkedList *llst = vay ->llst;
    for_each_in_linked_list(llst, ay)
        int curr_sz = len_array(*(Array **)ay);
        if(idx >= curr_sz)
        {
            idx -= curr_sz;
            continue;
        }
        if(remove_at_array(*(Array **)ay, idx) == 0)
        {
            vay ->total_sz -= 1;
            remove_null_array(vay);
            return 0;
        }
    end_for_each_in_linked_list
    return -1;
}

void *find_vararray(VarArray *vay, const void *cmp_val)
{
    if(!vay)
        return NULL;
    void *ay = NULL;
    LinkedList *llst = vay ->llst;
    for_each_in_linked_list(llst, ay)
        void *val = search_array(*(Array **)ay, cmp_val);
        if(val)
            return val;
    end_for_each_in_linked_list
    return NULL;
}

VarArray* slice_vararray(VarArray *vay, int start, int end, int step)
{
    if(!vay || !step)
        return NULL;
    if(start < 0)
        start += (int)vay ->total_sz;
    if(end < 0)
        end += (int)vay ->total_sz;
    if(end < 0 || start < 0)
        return NULL;

    end = MIN(end, vay ->total_sz);
    start = MIN(start, vay ->total_sz);
    VarArray *new_vay = new_vararray(vay ->item_sz, vay ->ay_block_sz, vay ->cmp, vay ->free_item);
    if(!new_vay)
        return NULL;
    if((start > end && step > 0) || (start < end && step < 0))
        return new_vay;

    void *ay = NULL;
    LinkedList *llst = vay ->llst;
    if(step < 0)
    {
        /*
         *                                      p
         *          [ ] <--[ ] <--[ ] ... <--[  |  ] <--[ ] ....<--[ ]
         *          \_______________/        \_____/    \____________/
         *                 l                    c             h
         *          \________________________________________________/
         *                                 t
         *          从后向前遍历，则需要求出p在当前数组的实际偏移量
         *          h，t，c是已知的
         *          则:
         *          l = t - h - c
         *          p在所取用的数组中的偏移量为
         *          p - l == p - (t-h-c) == p - t + h + c
         */
        int h = 0;
        int c = 0;
        int p = start;
        int t = vay ->total_sz;
        reverse_for_each_in_linked_list(llst, ay)
            c = len_array(*(Array **)ay); 
            if(c == 0)
                continue;
            if(t-h-c <= p)
            {
                while(p >= end)
                {
                    int idx = p-t+h+c;
                    if(idx >= 0)
                    {
                        if(append_vararray(new_vay, value_at_array(*(Array **)ay, idx)) != 0)
                            goto _fails;
                        p += step;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            h += c;
            if(t - h <= end)
                break;
        end_reverse_for_each_in_linked_list
    }
    else
    {
        for_each_in_linked_list(llst, ay)
            unsigned int curr_sz = len_array(*(Array **)ay);
            if(curr_sz == 0)
                continue;
            while(end >= start) 
            {
                if(append_vararray(new_vay, value_at_array(*(Array **)ay, start)) != 0)
                    goto _fails;
                start += step;
                if(start >= curr_sz)
                    break;
            }
            if(start > end)
                break;
            end -= curr_sz;
            start -= curr_sz;
        end_for_each_in_linked_list
    }
    return new_vay;

_fails:
    free_varray(new_vay);
    return NULL;
}



#ifdef vararray
#include <stdio.h>
void print_vararray_value(VarArray *vay)
{
    if(!vay)
    {
        printf("NULL\n");
        return;
    }
    printf("len=%d [", vay ->total_sz);
    void *ay = NULL;
    LinkedList *llst = vay ->llst;
    for_each_in_linked_list(llst, ay)
        Array **pp_ay = (Array **)ay;
        int i = 0;
        printf("len=%d [", len_array(*pp_ay));
        for(i=0; i < len_array(*pp_ay); i++)
        {
            int *p = (int *)value_at_array(*pp_ay, i);
            printf("%d,", *p);
        }
        printf("] ---->");
    end_for_each_in_linked_list
    printf("]\n");
}

void free_item(void *p)
{
    printf("delete item:[%d]\n", *(int*)p);
}

int cmp_item(const void *v1, const void *v2)
{
    return !(*(int*)v1 == *(int *)v2);
}

void test_append(int cnt)
{
    VarArray *vay = new_vararray(sizeof(int), 5, cmp_item, free_item);
    int i = 0;
    for(i=0; i < cnt; i++)
    {
        printf("append:%d, ret=%d\n", i, append_vararray(vay, &i));
    }
    print_vararray_value(vay);
    i = 9;
    remove_vararray(vay, &i);
    VarArray *vay1 = slice_vararray(vay, -1, 0, -1);
    print_vararray_value(vay1);

    VarArray *vay2 = slice_vararray(vay, 4, 0, -3);
    print_vararray_value(vay2);
    free_varray(vay);
    free_varray(vay1);
    free_varray(vay2);

}

int main(int argc,char *argv[])
{
    test_append(atoi(argv[1]));
    return 0;
}

#endif
