#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "jsonstring.h"

#ifndef MIN
#define MIN(a, b) (a) > (b) ? (b):(a)
#endif

/*
struct JsonStringFunc
{
	int (*get_grow_size) (JsonString *json_string) ; 
	int (*ensure_memory)(JsonString *json_string, size_t need_sz) ;
	int (*append_char_to_json_string)(JsonString *json, char ch) ;
	int (*append_string_to_json_string)(JsonString *json, const char *sz, size_t sz) ;
	JsonString* (*malloc_json_string)(size_t sz) ;
	void (*free_json_string)(JsonString *json_string) ;
}
*/

static int get_grow_size(const JsonString *json_string, size_t need_sz) ;
static int ensure_memory(JsonString *json_string, size_t need_sz) ;
static void free_json_string(JsonString *json_string) ;
static JsonString *malloc_json_string(size_t sz) ;
static int append_char_to_json_string(JsonString *json_string, char ch) ;
static int append_string_to_json_string(JsonString *json_string, const char *str, size_t sz) ;
static int hash(const JsonString *json_string) ;
static int len(const JsonString *json_string) ;
static int cmp(const JsonString *json_string1, const JsonString *json_string2) ;
static char at(const JsonString *json_string, int idx) ;

JsonStringFunc json_string_func = { .get_grow_size = get_grow_size,
                               .hash = hash,
                               .at = at, 
                               .len = len,
                               .cmp = cmp, 
                               .ensure_memory = ensure_memory,
                               .append_char = append_char_to_json_string,
                               .append_string = append_string_to_json_string,
                               .malloc = malloc_json_string,
                               .free = free_json_string} ;

JsonStringFunc *get_json_string_func()
{
    return &json_string_func ;
}

static char at(const JsonString *json_string, int idx)
{
    return json_string ->buff[idx] ;
}
static int cmp(const JsonString *json_string1, const JsonString *json_string2)
{
    if(!json_string1 || !json_string2)    
        return -1 ;
    int sz1 = json_string1 ->use_sz ;
    int sz2 = json_string2 ->use_sz ;
    while(--sz1 >= 0 && --sz2 >= 0)
    {
        if(json_string1 ->buff[sz1] != json_string2 ->buff[sz2])
            return json_string1 ->buff[sz1] - json_string2 ->buff[sz2] > 0 ? 1 : -1 ;
    }
    if(sz1 >= 0)
        return 1 ;  
    else if(sz2 >= 0)
        return -1 ;
    else
        return 0 ;
}

static int len(const JsonString *json_string)
{
    if(!json_string)
        return -1 ;
    return json_string ->use_sz ;
}

static int hash(const JsonString *json_string)
{
    if(!json_string)
        return -1 ;

    unsigned v = 0 ;
    int i = 0 ;
    for(i=0; i < json_string ->use_sz; i++)
    {
        v = (v << 5) + json_string ->buff[i] ;
    }
    return (int)v;
}

static int get_grow_size(const JsonString *json_string, size_t need_sz)
{
    return need_sz + 32 ;    
}

static int ensure_memory(JsonString *json_string, size_t need_sz)
{
    if(!json_string)
        return 0 ;
    if(!need_sz)
        return 1;
    if(need_sz + json_string ->use_sz <= json_string ->total_sz)
        return 1 ;
    size_t new_sz = sizeof(char)*(json_string ->total_sz + json_string_func.get_grow_size(json_string, need_sz)) ;
    char *new_buff = (char *)malloc(new_sz) ;
    if(!new_buff)
        return 0 ;

    if(json_string ->buff)
    {
        memcpy(new_buff, json_string ->buff, json_string ->use_sz) ;
        free(json_string ->buff) ;
    }
    json_string ->buff = new_buff ;
    json_string ->total_sz = new_sz ;
    return 1 ;
}

static void free_json_string(JsonString *json_string)
{
    if(!json_string)
        return ;
    if(json_string ->buff)
        free(json_string ->buff) ;
    json_string ->buff = nullptr ;
    json_string ->total_sz = 0 ;
    json_string ->use_sz = 0 ;
    free(json_string) ;
    json_string = nullptr ;
}

static JsonString *malloc_json_string(size_t sz)
{     
    JsonString *json_string = (JsonString *)malloc(sizeof(JsonString)) ;
    if(!json_string)
        return nullptr ;
    json_string ->buff = nullptr ;
    json_string ->total_sz = 0 ;
    json_string ->use_sz = 0 ;
    if(!json_string_func.ensure_memory(json_string, sz))
    {   
        json_string_func.free(json_string) ;
        json_string = nullptr ;
    }
    return json_string ;
}

static int append_char_to_json_string(JsonString *json_string, char ch)
{
    if(!json_string_func.ensure_memory(json_string, 1))
        return 0 ;
    json_string ->buff[json_string ->use_sz++] = ch ;
    return 1 ;
}

static int append_string_to_json_string(JsonString *json_string, const char *str, size_t sz)
{
    if(!json_string_func.ensure_memory(json_string, sz))
        return 0 ;
    memcpy(json_string ->buff+json_string ->use_sz, str, sz) ;
    json_string ->use_sz += sz ;
    return 1;
}

void print_json_string(JsonString *json_string)
{  
    if(!json_string)
        return ;
    printf("total_sz=%d\nuse_sz=%d\nbuff:\"") ;
    int i = 0 ;
    for(i=0; i< json_string ->use_sz; i++)
    {
        printf("%c", json_string ->buff[i]) ;
    }
    printf("\"\n") ;
}

//int main(int argc, char *argv[])
//{
//    JsonString *json_string = string_func.malloc_json_string(0) ;
//    print_json_string(json_string) ;
//    string_func.append_char_to_json_string(json_string, '1') ;
//    print_json_string(json_string) ;
//    string_func.append_string_to_json_string(json_string, "123456789", strlen("123456789")) ;
//    print_json_string(json_string) ;
//    return 0 ;
//}
