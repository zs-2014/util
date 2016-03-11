#include "array.h"
#include <stdlib.h>
#include <string.h>

static int _index_array(const Array *ay, const void *val, int start_idx)
{
    if(!ay || !ay ->cmp)
        return -1;
    unsigned int i = 0;
    unsigned int offset = 0;
    for(i=start_idx; i < ay ->curr_sz; i++)
    {
        if(ay ->cmp(ay ->item + offset, val) == 0)
            return i;
        offset += ay ->item_sz;
    }
    return -1;
}

static int _index_array(const Array *ay, const void *val, int end_idx)
{
    if(!ay || !ay ->cmp)
        return -1;
    unsigned int i = 0;
    unsigned int offset = end_idx * ay ->item_sz;
    for(i=end_idx; end_idx >= 0; end_idx--)
    {
        if(ay ->cmp(ay ->item+offset, val) == 0)
            return i;
        offset -= ay ->item_sz;
    }
    return -1;
}

Array *new_array(unsigned int total_sz, unsigned int item_sz)
{
    Array *ay = (Array *)MALLOC(sizeof(Array)+total_sz*item_sz);
    if(!ay)
        return NULL;

    ay ->total_sz = total_sz;
    ay ->curr_sz = 0;
    ay ->item_sz = item_sz;
    ay ->cmp = NULL;
    ay ->free_item = NULL;
    bzero(ay ->item, total_sz*item_sz);
    return ay;
}

void free_array(Array *ay)
{
    if(!ay)
        return ;
    if(!ay ->free_item)
    {
        FREE(ay);
        return ;
    }
    unsigned int i = 0;
    unsigned offset = 0;
    for(i=0; i < ay ->curr_sz; i++)
    {
        ay ->free_item(ay ->item+offset);
        offset += ay ->item_sz;
    }
    FREE(ay); 
    return ;
}

int append_array(Array *ay, void *val)
{
    if(!ay || !val|| ay ->curr_sz >= ay ->total_sz)
        return -1;
    memmove(ay ->item + ay ->curr_sz * ay ->item_sz, val, ay ->item_sz);
    ay ->curr_sz += 1;
    return 0;
}

//可以将src拼接到dst后
//两者的现有的容量之和不能超过dst的总大小
Array *extend_array(const Array *src, Array *dst)
{
    if(!src || !dst || src ->item_sz != dst ->item_sz || src ->curr_sz + dst ->curr_sz > dst ->total_sz)
        return NULL;
    memmove(dst ->item + dst ->curr_sz*dst ->item_sz, src ->item, src ->curr_sz * src ->item_sz);
    dst ->curr_sz += src ->curr_sz;
    return dst;
}

unsigned int len_array(const Array *ay)
{
    if(!ay)
        return (unsigned int)-1;
    return ay ->curr_sz;
}


/*
 *  [start, end]闭区间
 *  假设(end-start)/step == n
 *  start, start+1*step, start+2*step, ..., start+n*step
 *  且有start+n*step <= end
 *  则在[start:end:step]区间内元素的个数为 (end-start)/step+1
 */

Array *slice_array(const Array *ay, int start, int end, int step)
{
    if(!ay || ay ->curr_sz == 0 || step == 0)
        return NULL;
    if(start < 0)
        start += (int)ay ->curr_sz;
    if(end < 0)
        end += (int)ay ->curr_sz;
    if(end < 0 || start < 0)
        return NULL;
    end = MIN(end, ay ->curr_sz-1);
    start = MIN(start, ay ->curr_sz-1);
    if((start > end && step > 0) || (start < end && step < 0))
        return NULL;
    Array *tmp = NULL;
    //step为负时，逆序
    if(step < 0)
    {
        tmp = new_array((start-end)/abs(step)+1, ay ->item_sz);
        if(!tmp)
            return NULL;
        unsigned int offset = 0;
        while(start >= end) 
        {
            memmove(tmp ->item + offset, ay ->item + start*ay ->item_sz, ay ->item_sz);
            tmp ->curr_sz += 1;
            offset += ay ->item_sz;
            start += step;
        }
        return tmp;
    }
    else
    {
        tmp = new_array((end-start)/step+1, ay ->item_sz);
        if(!tmp)
            return NULL;
        //当step为1时可直接拷贝
        if(step == 1)
        {
            memmove(tmp ->item, ay ->item + start*ay ->item_sz, (end-start+1)*ay ->item_sz);
            tmp ->curr_sz = end-start+1;
        }
        else
        {
            unsigned int offset = 0;
            while(end >= start)
            {
                memmove(tmp ->item+offset, ay ->item+start*ay ->item_sz, ay ->item_sz);
                tmp ->curr_sz += 1;
                offset += ay ->item_sz;
                start += step;
            }
        }

    }
    tmp ->cmp = ay ->cmp; 
    tmp ->free_item = ay ->free_item;
    return tmp;

}

int index_array(const Array *ay, const void *val)
{
    if(!ay || !ay ->cmp)
        return -1;
    return _index_array(ay, val, 0);
}

void *value_at_array(Array *ay, int idx)
{
    if(!ay)
        return NULL;
    if(idx < 0)
        idx += (int)ay ->curr_sz;
    if(idx < 0 || idx >= ay ->curr_sz)
        return NULL;
    return (void *)(ay ->item + idx*ay ->item_sz);
}

void *search_array(Array *ay, const void *val)
{
    int idx = _index_array(ay, val, 0);
    if(idx == -1)
        return NULL;
    return (void *)(ay ->item+idx*ay ->item_sz);
}

int remove_at_array(Array *ay, int idx)
{
    if(!ay)
        return -1;
    if(idx < 0)
        idx += (int) ay ->curr_sz;
    if(idx < 0 || idx >= ay ->curr_sz)
        return -1;
    
    unsigned int offset = idx * ay ->item_sz;
    unsigned int end_offset = ay ->curr_sz * ay ->item_sz;
    if(ay ->free_item)
        ay ->free_item(ay ->item + offset);
    memmove(ay ->item + offset, ay ->item + offset + ay ->item_sz, end_offset-offset-ay->item_sz);
    ay ->curr_sz -= 1;
    return 0;
}

int remove_array(Array *ay, const void *val)
{
    int idx = _index_array(ay, val, 0);
    if(idx == -1)
        return -1;
    return remove_at_array(ay, idx);
}

//删除第多少个
int remove_nth_array(Array *ay, const void *val, int n)
{
    if(!ay || n <= 0)
        return -1;
    int idx = 0;
    while(n-- > 0)
    {
        if((idx = _index_array(ay, val, idx)) < 0)
            return -1;
    }
    remove_at_array(ay, idx);
    return 0;
}

int remove_all_array(Array *ay, const void *val)
{
    if(!ay)
        return -1;
    int idx = 0;
    while((idx=_index_array(ay, val, idx)) >= 0)
        remove_at_array(ay, idx);
    return 0;
}

int count_array(Array *ay, const void *val)
{
    if(!ay) 
        return -1;
    int idx = 0;
    int n = 0;
    while((idx=_index_array(ay, val, idx)) >= 0)
        n++;
    return n;
}

//只能覆盖现有的值，而不能通过这个添加新值
//如果定义了释放函数，会释放原来的数据
int assign_array(Array *ay, int idx, void *val)
{
    if(!ay)
        return -1;
    if(idx < 0)
        idx += (int)ay ->curr_sz;
    if(idx < 0 || idx >= ay ->curr_sz)
        return -1;
    unsigned int offset = idx * ay ->item_sz;
    if(ay ->free_item)
        ay ->free_item(ay ->item + offset);
    memmove(ay ->item+offset, val, ay ->item_sz);
    return 0;
}

int is_in_array(Array *ay, const void *val)
{
    return index_array(ay, val) == -1 ? -1 : 0;
}

Array *resize_array(Array *ay, unsigned int total_sz)
{
    if(!ay)
        return NULL;
    if(ay ->total_sz == total_sz)
        return ay;
    Array *tmp = new_array(total_sz, ay ->item_sz);
    if(!tmp)
        return NULL;

    unsigned copy_items = MIN(total_sz, ay ->curr_sz);
    memcpy(tmp ->item, ay ->item, ay ->item_sz*copy_items);
    tmp ->curr_sz = copy_items;
    tmp ->cmp = ay ->cmp;
    tmp ->free_item = ay ->free_item;
    ay ->cmp = NULL;
    ay ->free_item = NULL;
    FREE(ay);
    return tmp;
}

#ifdef TEST
#include <stdio.h>

void print_array_info(Array *ay)
{
    if(!ay)
    {
        printf("NULL\n");
        return ;
    }
    printf("-------------------array info ----------------------\n");
    printf("array addr=%p\narray.total_sz=%u\narray.curr_sz=%u\narray.item_sz=%u\n", ay, ay ->total_sz, ay ->curr_sz, ay ->item_sz);
}

void print_array_value_info(Array *ay, int sz)
{
    if(!ay)
    {
        printf("NULL\n");
        return;
    }
    int i = 0;
    printf("[");
    for(i=0; i < sz; i++)
    {
        int *p = value_at_array(ay, i);
        if(p)
            printf("(%d, %d),", i, *p);
        else
            printf("(%d, NULL),", i);
    }
    printf("]\n");
}

int cmp(const void* v1, const void* v2)
{
    return !(*(int*)v1 == *(int *)v2);
}

void free_item(void *p)
{
    printf("free_item:%p value=%d\n", p, *(int*)p);
}

void test_append()
{
    Array *ay = new_array(5, sizeof(4));
    ay ->cmp = cmp;
    ay ->free_item = free_item;
    int i = 0;
    for(i=0; i<100; i++)
    {
        append_array(ay, &i);
    }
    print_array_info(ay);
    print_array_value_info(ay, 7);
    free_array(ay);
}

void test_extend_array()
{
    printf("extend two NULL array ret=%p\n", extend_array(NULL, NULL));
    Array *ay1 = new_array(10, sizeof(int));
    printf("extend NULL array ret=%p\n", extend_array(NULL, ay1));
    printf("extend NULL array ret=%p\n", extend_array(ay1, NULL));
    print_array_info(ay1);
    Array *ay2 = new_array(3, sizeof(int));
    print_array_info(ay2);
    extend_array(ay2, ay1); 
    print_array_info(ay1);
    int i = 0;
    for(i=0; i < 4; i++)
    {
        append_array(ay1, &i);
        append_array(ay2, &i);
    }
    print_array_info(ay1);
    print_array_info(ay2);
    extend_array(ay2, ay1);
    print_array_info(ay1);
    print_array_info(ay2);
    print_array_value_info(ay1, len_array(ay1));
    print_array_value_info(ay2, len_array(ay2));
    Array *ay3 = new_array(30, 5);
    printf("extend different size array ret=%p\n", extend_array(ay1, ay3));
    free_array(ay3);
    free_array(ay1);
    free_array(ay2);

    ay1 = new_array(10, sizeof(int));
    ay2 = new_array(14, sizeof(int));
    for(i=0; i< 10; i++)
    {
        append_array(ay1, &i);
        append_array(ay2, &i);
    }
    print_array_info(ay1);
    print_array_info(ay2);
    extend_array(ay1, ay2);
    print_array_info(ay1);
    print_array_info(ay2);
    print_array_value_info(ay1, len_array(ay1));
    print_array_value_info(ay2, len_array(ay2));
    free_array(ay1);
    free_array(ay2);

    ay1 = new_array(10, sizeof(int));
    ay2 = new_array(20, sizeof(int));
    for(i=0; i< 10; i++)
    {
        append_array(ay1, &i);
        append_array(ay2, &i);
    }
    print_array_info(ay1);
    print_array_info(ay2);
    extend_array(ay1, ay2);
    print_array_info(ay1);
    print_array_info(ay2);
    print_array_value_info(ay1, len_array(ay1));
    print_array_value_info(ay2, len_array(ay2));
    free_array(ay1);
    free_array(ay2);
}

void test_slice_array()
{
    Array *ay = new_array(12, sizeof(int));
    int i = 0;
    for(i=0; i < 10; i++)
    {
        append_array(ay, &i);
    }
    print_array_value_info(ay, len_array(ay));
    Array *ay1 = slice_array(ay, 0, -1, 1);
    print_array_info(ay1);
    print_array_value_info(ay, len_array(ay1));
    free_array(ay1);

    ay1 = slice_array(ay, 0, 1, 1);
    print_array_info(ay1);
    print_array_value_info(ay1, len_array(ay1));
    free_array(ay1);

    ay1 = slice_array(ay, -1, 1, -4);
    print_array_info(ay1);
    print_array_value_info(ay1, len_array(ay1));
    free_array(ay1);

    ay1 = slice_array(ay, 1, 1, -4);
    print_array_info(ay1);
    print_array_value_info(ay1, len_array(ay1));
    free_array(ay1);

    ay1 = slice_array(ay, -1, -1, -4);
    print_array_info(ay1);
    print_array_value_info(ay1, len_array(ay1));
    free_array(ay1);

    ay1 = slice_array(ay, 100, 0, -1);
    print_array_info(ay1);
    print_array_value_info(ay1, len_array(ay1));
    free_array(ay1);

    ay1 = slice_array(ay, 2, 10, 2);
    print_array_info(ay1);
    print_array_value_info(ay1, len_array(ay1));
    free_array(ay1);

    free_array(ay);
    ay = new_array(0, sizeof(0));
    ay1 = slice_array(ay, 1, 2, 3);
    print_array_info(ay1);
    print_array_value_info(ay1, len_array(ay1));
    free_array(ay1);
}

void test_remove_array()
{
    Array *ay = new_array(10, sizeof(4));
    ay ->cmp = cmp;
    ay ->free_item = free_item;
    int i = 0;
    for(i=0; i < 10; i++)
    {
        append_array(ay, &i);
    }
    print_array_value_info(ay, len_array(ay));
    printf("remove_array(10) ret=%d\n", remove_array(ay, &i));
    print_array_value_info(ay, len_array(ay));
    i = 0;
    printf("remove_array(0) ret=%d\n", remove_array(ay, &i));
    print_array_value_info(ay, len_array(ay));
    i = -1;
    printf("remove_array(-1) ret=%d\n", remove_array(ay, &i));
    print_array_value_info(ay, len_array(ay));

    for(i=0; i < 10; i++)
    {
        printf("remove_array(%d) ret=%d\n", i, remove_array(ay, &i));
        print_array_value_info(ay, len_array(ay));
    }
    for(i=0; i < 10; i++)
    {
        append_array(ay, &i);
    }
    free_array(ay);
}

void test_resize_array()
{
    Array *ay = new_array(10, sizeof(int));
    ay ->cmp = cmp;
    ay ->free_item = free_item;
    int i = 0;
    for(i=0; i < 10; i++)
    {
        append_array(ay, &i);
    }
    print_array_value_info(ay, len_array(ay));
    ay = resize_array(ay, 14);
    print_array_value_info(ay, len_array(ay));
    ay = resize_array(ay, 5);
    print_array_value_info(ay, len_array(ay));
    free_array(ay);
}

void test_assign_array()
{
    Array *ay = new_array(10, sizeof(int));
    ay ->cmp = cmp;
    ay ->free_item = free_item;
    int i = 0;
    for(i=0; i < 10; i++)
    {
        append_array(ay, &i); 
    }
    print_array_value_info(ay, len_array(ay));
    i = 0;
    assign_array(ay, 0, &i);
    print_array_value_info(ay, len_array(ay));
    i = -1;
    assign_array(ay, -1, &i);
    print_array_value_info(ay, len_array(ay));
    i = -1;
    assign_array(ay, 4, &i);
    print_array_value_info(ay, len_array(ay));
    free_array(ay);
}

void test_valid()
{
    Array *ay = new_array(10, sizeof(4));
    ay ->free_item = free_item;
    print_array_info(ay);
    int i = 0;
    for(i=0; i < 100; i++)
    {
        append_array(ay, &i);
    }
    i = 9;
    int idx = index_array(ay, &i);
    printf("cmp=%p search value(%d) at %d\n", ay ->cmp, i, idx);
    ay ->cmp = cmp;
    idx = index_array(ay, &i);
    printf("cmp=%p search value(%d) at %d\n", ay ->cmp, i, idx);
    print_array_info(ay);
    print_array_value_info(ay, 12);
    int ret = remove_at_array(ay, 0);
    printf("remove_at_array(0) ret=%d\n", ret);
    print_array_value_info(ay, len_array(ay));
    ret = remove_at_array(ay, -1);
    printf("remove_at_array(-1) ret=%d\n", ret);
    print_array_value_info(ay, len_array(ay));
    free_array(ay);
    //free_array(ay);
    while(len_array(ay) > 0)
    {
        remove_at_array(ay, 0);
    }
    print_array_info(ay);
    print_array_value_info(ay, len_array(ay));
    ret = remove_at_array(ay, 100);
    printf("remove_at_array(100) ret=%d\n", ret);
    print_array_info(ay);

    ret = remove_at_array(ay, -1);
    printf("remove_at_array(-1) ret=%d\n", ret);
    print_array_info(ay);

}

int main(int argc, char *argv[])
{
    //test_valid();
    //test_append();
    //test_extend_array();
    //test_slice_array();
    //test_remove_array();
    //test_assign_array();
    test_resize_array();
    return 0;
}
#endif
