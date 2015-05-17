#include <stdlib.h>
#include <stdio.h>

#include "jsonstring.h"
#include "jsonobject.h"
#include "jsondict.h"

#ifndef HASH_TABLE_SIZE
#define HASH_TABLE_SIZE  31
#endif

/*
struct JsonDictFunc
{
	JsonDict* (*malloc_json_dict) (int sz) ;		
    void* (*free_json_dict) (JsonDict *json_dict) ;
	JsonObject* (*set) (JsonDict *json_dict, JsonString *key, JsonObject *value) ;
	JsonObject* (*is_exist) (JsonDict *json_dict, JsonString *key) ;
	JsonObject* (*get) (JsonDict *json_dict, JsonString *key) ;
}
*/

static JsonObject *dict_set(JsonDict *json_dict, JsonString *key, JsonObject *value) ;
static JsonObject *dict_get(const JsonDict *json_dict, const JsonString *key) ;
static int dict_is_exist(const JsonDict *json_dict, const JsonString *key) ;
static int dict_hash(const JsonDict *json_dict, const JsonString *key) ;
static JsonDict *malloc_json_dict(size_t sz) ;
static void free_json_dict(JsonDict *json_dict) ;

extern JsonStringFunc json_string_func ;
extern JsonObjectFunc json_object_func ;

JsonDictFunc json_dict_func = {.malloc = malloc_json_dict,
                               .free = free_json_dict,
                               .get = dict_get,
                               .set = dict_set,
                               .hash = dict_hash,
                               .is_exist = dict_is_exist} ;

JsonDictFunc* get_json_dict_func()
{
    return &json_dict_func ;    
}

static void free_dict(Dict *dict)
{
    if(!dict)
        return ;

    DictNode *next = dict ->head_node;
    while(next)
    {
        DictNode *tmp = next ->next ;
        json_object_func.free(next ->value) ;
        free(next) ; 
        next = tmp ;
    }
}

static void free_json_dict(JsonDict *json_dict)
{
    if(!json_dict)
        return ; 
    size_t i = 0 ;
    for(i=0; i < json_dict ->total_sz; i++)
    {
        free_dict(json_dict ->dict+i) ;    
    }
    free(json_dict ->dict) ;
    json_dict ->dict = nullptr ;  
    json_dict ->use_sz = 0 ;
    json_dict ->total_sz = 0 ;
    free(json_dict) ;
    json_dict = nullptr ;
}

static JsonDict *malloc_json_dict(size_t sz)
{
    JsonDict *json_dict = (JsonDict *)malloc(sizeof(JsonDict)) ; 
    if(!json_dict)
        return nullptr;
    json_dict ->use_sz = 0 ;
    json_dict ->dict = nullptr;
    sz = sz == 0 ? HASH_TABLE_SIZE : sz ;
    Dict *dict = (Dict *)malloc(sizeof(Dict)*sz) ;
    if(!dict)
    {
        json_dict_func.free(json_dict) ;  
        return nullptr ;
    }
    json_dict ->total_sz = sz ;
    size_t i = 0 ;
    for(i=0; i < sz; i++)
    {
        dict ->head_node = dict ->tail_node = nullptr ;
    }
    json_dict ->dict = dict ;
    return json_dict ;
}

static int dict_hash(const JsonDict *json_dict, const JsonString *key)
{
    if(!key || !json_dict)
        return -1 ;
    int v = json_string_func.hash(key) ; 
    if(v <= -1)
        return -1 ;
    return v % json_dict ->total_sz ;
}

static int dict_is_exist(const JsonDict *json_dict, const JsonString *key)
{
    return json_dict_func.get(json_dict, key) != nullptr ;
}

static JsonObject *dict_get(const JsonDict *json_dict, const JsonString *key)
{
    int idx = json_dict_func.hash(json_dict, key) ;
    if(idx == -1)
        return nullptr;
    DictNode *dict_node = json_dict ->dict[idx].head_node ;
    while(dict_node)
    {
        if(json_string_func.cmp(dict_node ->key, key) == 0)
            return dict_node ->value ;
        dict_node = dict_node ->next ;
    }
    return nullptr ;
}

static JsonObject *dict_delete(JsonDict *json_dict, const JsonDict *key)
{
    return nullptr ;
}

static JsonObject *dict_set(JsonDict *json_dict, JsonString *key, JsonObject *value)
{
    int idx = json_dict_func.hash(json_dict, key) ;
    if(idx == -1)
        return nullptr ;
    DictNode *dict_node = (DictNode *)malloc(sizeof(DictNode)) ;
    if(!dict_node)
        return nullptr ;
    dict_node ->next = nullptr ;
    dict_node ->key = key ;
    dict_node ->value = value ;
    
    Dict *dict = json_dict ->dict ;
    //第一个节点设置头部
    if(!dict[idx].head_node)
    {
        dict[idx].head_node = dict_node ;
        dict[idx].tail_node = dict_node ;
    }
    else
    {
        dict[idx].tail_node ->next = dict_node ;
        dict[idx].tail_node = dict_node ;
    }
    json_dict ->use_sz += 1;
    return value ; 
}

//int main(int argc, char *argv[])
//{
//    return  0;
//}
