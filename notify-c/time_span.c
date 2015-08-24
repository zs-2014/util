#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "string_util.h"
#include "linklist.h"
#include "time_span.h"
#include "log.h"

//, separated
struct TimeSpan *new_time_span(const char *time_span_str)
{
    if(!time_span_str) 
        return NULL;
    DEBUG("time_span_str:%s", time_span_str) ;
    struct LinkedList *llst = split(time_span_str, ',', -1, 1) ; 
    if(!llst) 
        return NULL ;
    struct TimeSpan *time_span = (struct TimeSpan *)calloc(1, sizeof(struct TimeSpan)) ; 
    if(!time_span)
        return NULL ;
    time_span ->total_sz = count_linked_list(llst) ;
    DEBUG("time span count:%d", count_linked_list(llst)) ;
    time_span ->curr_sz = 0 ;
    time_span ->data = (unsigned *)calloc(1, sizeof(unsigned)*time_span ->total_sz + 1) ;
    char *val = NULL ;
    DEBUG("begin read time span") ;
    for_each_in_linked_list(llst, val)
    {
        DEBUG("val=%s", val) ;
        val = strip(val, ' ') ;
        if(val[0] == 0)
            continue ;
        val = to_upper(val) ;
        //毫秒为单位
        if(strstr(val, "MS") != NULL)
            time_span ->data[time_span ->curr_sz++] = atoi(val) ; 
        //秒为单位
        else if(strstr(val, "S") != NULL)
            time_span ->data[time_span ->curr_sz++] = atoi(val)*1000 ;
        //分钟
        else if(strstr(val, "M") != NULL)
            time_span ->data[time_span ->curr_sz++] = atoi(val)*1000*60 ;
        //小时
        else if(strstr(val, "H") != NULL)
            time_span ->data[time_span ->curr_sz++] = atoi(val)*1000*3600 ;
        //默认是秒为单位
        else
            time_span ->data[time_span ->curr_sz++] = atoi(val)*1000 ;
        DEBUG("%u", time_span ->data[time_span ->curr_sz-1]) ;
    }
    free_linked_list(llst) ;
    return time_span ;
}

void free_time_span(struct TimeSpan *time_span)
{
    if(!time_span)
        return ;
    free(time_span ->data) ;
    time_span ->data = NULL ;
    time_span ->total_sz = 0 ;
    time_span ->curr_sz = 0 ;
    free(time_span) ;
    time_span = NULL ;
}

int next_time_span(struct TimeSpan *time_span, int idx)
{
    if(!time_span || idx > time_span ->curr_sz)
        return -1 ;
    return time_span ->data[idx] ; 
}

#ifdef TIMESPAN
void print_time_span(struct TimeSpan *time_span)
{
    int i = 0 ;    
    for(i=0; i < time_span ->curr_sz; i++)
    {
        printf("%d,", next_time_span(time_span, i)) ;
    }
    printf("\n") ;
}

int main(int argc, char *argv[])
{
   struct TimeSpan *time_span = new_time_span(argv[1]) ;
   print_time_span(time_span) ;
   free_time_span(time_span) ;
   return 0 ;
}

#endif
