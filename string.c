
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdarg.h>

#include "linklist.h"
#include "string.h"
#include "buffer.h"

#ifndef null
#define null NULL
#endif

LinkedList *split(const char *buff, char c, int n, int drop_null_str)
{
    if(!buff)
        return null ;

    LinkedList *llst = malloc_linked_list() ;
    if(n == 0)
    {
        append_to_linked_list(llst, strdup(buff)) ;
        return llst ;
    }
    n = (n < 0 ? INT_MAX : n );
    const char *start = buff ;
    const char *end = null;
    do
    {
        end = strchr(start, c) ; 
        if(end == start)
        {
            if(!drop_null_str)
                append_to_linked_list(llst, strdup("\0")) ;
        }
        else if(end)
            append_to_linked_list(llst, strndup(start, end - start)) ;
        else
            append_to_linked_list(llst, strdup(start)) ;
        start = end+1 ;
        n-- ;
    }while(end && n > 0) ;

    return llst ;
}

char *join(const char *sp, LinkedList *llst)
{
    if(!sp || !llst)
        return null ;

    Buffer buffer ; 
    init_buffer(&buffer, 0) ;
    int sp_len = strlen(sp) ;
    char *str ;
    for_each_in_linked_list(llst, str)
    {
        if(!append_str_to_buffer(&buffer, str, strlen(str)) || !append_str_to_buffer(&buffer, sp, sp_len))
        {
            deconstruct_buffer(&buffer) ;
            return null ;
        } 
    }
    if(!append_char_to_buffer(&buffer, '\0'))
    {
        deconstruct_buffer(&buffer) ;
        return null ;
    }
    int reset_pos = buffer.use_sz - 1; 
    if(reset_pos >= sp_len)
        reset_pos = reset_pos - sp_len ;
    char *buff = (char *)buffer.buff ;
    buff[reset_pos] = '\0' ;
    return buff; 
}

char *head_strip(char *str, char ch)
{
    if(!str)
        return null ;
    char *orig = str ; 
    while(*str != '\0' && *str == ch)
        str++ ;
    strcpy(orig, str) ;
    return orig ;
}

char *tail_strip(char *str, char ch)
{
    if(!str)
        return null ;
    int len = strlen(str)-1 ;
    while(len >= 0 && str[len] == ch)
        str[len--] = '\0'  ; 
    return str ;
}

char *strip(char *str, char ch)
{
    if(!str)
        return null ;
    return tail_strip(head_strip(str, ch), ch) ;
}

#ifdef STRING
int main(int argc, char *argv[])
{
    LinkedList *llst = malloc_linked_list() ; 
    llst ->dup_value = strdup ;
    char *str = join(".", llst) ;
    printf("'.'.join([]):[%s]\n", str) ;
    free(str) ;

    append_to_linked_list(llst, "hello") ; 
    str = join(".", llst) ;
    printf("'.'.join([hello]):[%s]\n", str) ;
    free(str) ;

    append_to_linked_list(llst, "world") ; 
    str = join(".", llst) ;
    printf("'.'.join([hello, world]):[%s]\n", str) ;
    free(str) ;

    str = join("", llst) ;
    printf("''.join([hello, world]): [%s]\n", str) ;
    return 0 ;
}
#endif
