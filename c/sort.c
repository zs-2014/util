
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SWAP(a, b) {int __tmp = a ; a = b ; b=__tmp ;}
int parition(int *ay, int low, int high)
{
    int privot = ay[(low+high)/2] ;
    SWAP(ay[low], ay[(low+high)/2]) ;
    while(low < high)
    {
        while(low < high && ay[high] > privot) --high ;
        ay[low] = ay[high] ;
        while(low < high && ay[low] <= privot) ++low ;
        ay[high] = ay[low] ;
    }
    ay[low] = privot ;
    return low ;
}

/*
 * 排序区间为[low, high]所以当low == high的时候[low, high]区间内的
 * 元素的个数为1,所以不需要排序,对于区间[low, high]内的元素,经过一
 * 次parition 之后则有[low, privot) < [privot] < (privot, high]
 * 所以再次排序时,则只需要排序[low, privot), (privot, high]这两个
 * 区间即可,因为区间有如下关系
 * [low, privot) + [privot] + (privot, high] == [low, privot-1] +[privot]+[privot+1, high] == [low, high] 
 * 而quick_sort排序的区间为[low, high],所以在递归调用的时候,
 * 将区间分成[low, privot-1], [privot+1, high]来递归的排序,直到有序为止
 * 如果quick_sort排序的区间为[low, high)则quick_sort可以写成:
 * void quick_sort(int *ay, int low, int high)
 * {
 *      //[low,low+1)  == 1个元素,则不必进行排序
 *      if(low+1 >= high)
 *          return ;
 *      //因为pariton所操作的区间是[low, high] 所以high要减去1
 *      int privot = parition(ay, low, high-1) ;
 *      quick_sort(ay, low, privot) ;  //==== [low, privot)
 *      quick_sort(ay, privot+1, high) ; //====[privot+1, high)
 *      //[low, privot) + [privot] + [privot+1, high) === [low, high)
 *      //只不过在排序的时候high得变一下变成quick_sort(ay, 0, sizeof(ay)/sizeof(int))而不是quick_sort(ay, 0, sizeof(ay)/sizeof(int)-1)
 * }
 * */

void quick_sort(int *ay, int low, int high)
{
    if(low >= high)
        return ;
    int privot = parition(ay, low, high) ;
    //再次调用quick_sort时, privot是否该减去1要看quick_sort
    //所排序的区间是什么
    quick_sort(ay, low, privot-1) ;
    quick_sort(ay, privot+1, high);
}


void adjust_heap(int *ay, int node, int high)
{
    while(node < high)
    {
        int right_chld = node * 2 ;
        int left_chld = node * 2 + 1 ;
        if(right_chld < high && left_chld < high && ay[right_chld] > ay[left_chld])
            right_chld++ ;
        if(right_chld < high && ay[right_chld] >= ay[node])
            break ;
        else if(right_chld < high)
        {
            SWAP(ay[right_chld], ay[node])
            node = right_chld ;
        }
        else break ;
    }
}


void heap_sort(int *ay, int high)
{
    int i = 0 ;
    int len = high/2 ;
    for(; len >= 0; --len) 
        adjust_heap(ay, len, high) ;
    for(; --high > 0; )
    {
        SWAP(ay[0], ay[high]) ;
        adjust_heap(ay, 0, high) ;
    }
}

void do_merge_sort(int *ay, int *tmpay, int low, int high)
{
    //not low >= high
    if(low+1 >= high)
        return ;
    int mid = (high+low)/2 ;
    //再次调用do_merge_sort的时候,mid是+1还是减去1要看
    //do_merge_sort本身所归并的区间是什么来决定,而不是
    //固定的
    do_merge_sort(ay, tmpay, low, mid) ;
    do_merge_sort(ay, tmpay, mid, high) ;
    //合并操作
    int cnt = 0 ;
    int tmp_l = low ;
    int tmp_m = mid ;
    while(low < tmp_m &&  mid < high)
    {
        if(ay[low] <= ay[mid])
            tmpay[cnt++] = ay[low++] ;
        else
            tmpay[cnt++] = ay[mid++] ;
    }
    while(low < tmp_m)
        tmpay[cnt++] = ay[low++] ;
    while(mid < high)
        tmpay[cnt++] = ay[mid++] ;
    int i = 0 ;
    while(i < cnt)
    {
        ay[tmp_l+i] = tmpay[i] ;
        i++ ;
    }
}

void merge_sort(int *ay, int low, int high)
{
    int *tmpay = (int *)malloc(sizeof(int) * (high - low)) ;
    do_merge_sort(ay, tmpay, low, high) ;
    free(tmpay) ;
}

int binary_search(int *ay, int l, int h, int v)
{
    while(l < h)
    {
        int mid = (h-l)/2 + l ;
        if(ay[mid] < v)
            l = mid+1 ;
        else if (ay[mid] > v)
            h = mid ;
        else return mid ;
    }
    return -1;
}

int binary_search_low(int *ay, int l, int h, int v)
{
    int last = -1 ;
    while(l < h)
    {
        int mid = (h-l)/2 + l ;
        if(ay[mid] < v)
        {
            l = mid + 1 ; 
        }
        else if(ay[mid] > v)
        {
            h = mid ;
        }
        else
        {
            last = mid ;
            h = mid ;
        }
    }
    return last ;

}

int binary_search_up(int *ay, int l, int h, int v)
{
    int last = -1 ;
    while(l < h)
    {
        int mid = (h-l)/2 + l ;
        if(ay[mid] < v)
        {
            l = mid + 1 ; 
        }
        else if(ay[mid] > v)
        {
            h = mid ;
        }
        else
        {
            last = mid ;
            l = mid + 1 ;
        }
    }
    return last ;
}

int main(int argc, char *argv[])
{
    int a[] = {1,2,5,342,12,0,0,0,0,0,0} ;
    quick_sort(a, 0, sizeof(a)/sizeof(int)-1) ;
    int i = 0 ;
    for(i=0; i < sizeof(a)/sizeof(int); i++)
    {
        printf("%d,", a[i]) ;
    }
    printf("\n") ;
    int b[] = {1,2,5,342,12,0,0,0,0,0,0} ;
    heap_sort(b, sizeof(b)/sizeof(int)) ;
    for(i=0; i < sizeof(b)/sizeof(int); i++)
    {
        printf("%d,", b[i]) ;
    }
    printf("\n") ;

    int d[] = {1,2,5,342,12,0,0,0,0,0,0} ;
    int c[] = {10,9} ;
    merge_sort(c, 0, sizeof(c)/sizeof(int)) ;
    for(i=0; i < sizeof(c)/sizeof(int); i++)
    {
        printf("%d,", c[i]) ;
    }
    printf("\n") ;
    int e[] = {0,1,2,3,4,5,5,5,5,6,7,7,7,8,9,9,10,10,10,10} ;
    int v = atoi(argv[1]) ;
    printf("%d at [%d, %d]\n", v, binary_search_low(e, 0, sizeof(e)/sizeof(int), v), binary_search_up(e, 0, sizeof(e)/sizeof(int), v)) ;
    return 0 ;
}
