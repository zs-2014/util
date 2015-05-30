
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "linklist.h"
#include "string.h"

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


