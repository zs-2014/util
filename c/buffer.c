#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"

#define default_buffer_increase_size 32

#ifndef null
#define null NULL
#endif

static int ensure_mem(Buffer *buffer, size_t need_sz)
{
    if(buffer ->use_sz + need_sz <= buffer ->total_sz)
        return 1;
    size_t new_sz = buffer ->total_sz + need_sz + default_buffer_increase_size;
    unsigned char *new_buff = (unsigned char *)calloc(1, need_sz) ;
    if(!new_buff)
        return 0 ;
    if(buffer ->buff)
        memcpy(new_buff, buffer ->buff, buffer ->use_sz) ;
    free(buffer ->buff) ;
    buffer ->buff = new_buff ;
    buffer ->total_sz = new_sz ;
    return 1 ;

}

int init_buffer(Buffer *buffer, size_t need_sz)
{
    if(!buffer)
        return 0 ;

    buffer ->use_sz = 0 ;
    buffer ->total_sz = 0 ;
    buffer ->buff = null ;
    return ensure_mem(buffer, need_sz) ;

}

void deconstruct_buffer(Buffer *buffer)
{
    if(buffer) 
    {
        free(buffer ->buff) ; 
        buffer ->buff = null ;
        buffer ->total_sz = 0;
        buffer ->use_sz = 0 ;
    } 
}

void free_buffer(Buffer *buffer)
{
    deconstruct_buffer(buffer) ;  
    free(buffer) ;
    buffer = null ;
}

Buffer *malloc_buffer(size_t sz)
{
    Buffer *buffer = (Buffer *)calloc(1, sizeof(Buffer)) ;
    buffer ->buff = null ;
    buffer ->total_sz = 0 ;
    buffer ->use_sz = 0 ;
    if(!ensure_mem(buffer, sz))
    {
        free_buffer(buffer) ;
        return null ;
    }
    return buffer ;
}

int append_str_to_buffer(Buffer *buffer, const char *str, size_t sz)
{
    if(!buffer || !str || !ensure_mem(buffer, sz))
        return 0 ;
    if(!sz)
        return 1;
    memcpy(buffer ->buff + buffer ->use_sz, str, sz) ;
    buffer ->use_sz += sz ;
    return 1 ;
}

int append_char_to_buffer(Buffer *buffer, char ch)
{
    if(!buffer || ! ensure_mem(buffer, 1))
        return 0 ;
    buffer ->buff[buffer ->use_sz++] = ch ;
    return 1;
}

int len(const Buffer *buffer)
{
    return buffer ? buffer ->use_sz: 0 ;
}

