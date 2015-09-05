#include <stdlib.h>
#include <stdio.h>

#include "jsonstring.h"
#include "jsonvalue.h"
#include "jsondict.h"

#ifndef HASH_TABLE_SIZE
#define HASH_TABLE_SIZE 31 
#endif

/*
struct JsonDictFunc
{
	JsonDict* (*malloc_json_dict) (int sz) ;		
    void* (*free_json_dict) (JsonDict *json_dict) ;
	JsonValue* (*set) (JsonDict *json_dict, JsonString *key, JsonValue *value) ;
	JsonValue* (*is_exist) (JsonDict *json_dict, JsonString *key) ;
	JsonValue* (*get) (JsonDict *json_dict, JsonString *key) ;
}
*/

static DictNode* dict_find(const JsonDict *json_dict, const JsonString *key) ;
static JsonValue *dict_set(JsonDict *json_dict, JsonString *key, JsonValue *value) ;
static JsonValue *dict_get(const JsonDict *json_dict, const JsonString *key) ;
static int init_json_dict(JsonDict *json_dict, size_t sz) ;
static int dict_is_exist(const JsonDict *json_dict, const JsonString *key) ;
static int dict_hash(const JsonDict *json_dict, const JsonString *key) ;
static JsonDict *malloc_json_dict(size_t sz) ;
static void free_json_dict(JsonDict *json_dict) ;
static void deconstruct_json_dict(JsonDict *json_dict) ;

extern JsonStringFunc json_string_func ;
extern JsonValueFunc json_value_func ;

JsonDictFunc json_dict_func = {.malloc = malloc_json_dict,
                               .init = init_json_dict,
                               .free = free_json_dict,
                               .deconstruct = deconstruct_json_dict,
                               .get = dict_get,
                               .set = dict_set,
                               .hash = dict_hash,
                               .is_exist = dict_is_exist,
                               .__find = dict_find} ;

JsonDictFunc* get_json_dict_func()
{
    return &json_dict_func ;    
}

static int init_json_dict(JsonDict *json_dict, size_t sz)
{
    if(!json_dict)
        return 0 ;
    sz = sz ? sz: HASH_TABLE_SIZE ;
    json_dict ->dict = (Dict *)calloc(sz, sizeof(Dict));
    if(!json_dict ->dict)
        return 0 ;
    size_t i = 0 ;
    for(i=0; i < sz; i++)
    {
        json_dict ->dict[i].head_node = null ;
        json_dict ->dict[i].tail_node = null ;
    }
    json_dict ->total_sz = sz ;
    json_dict ->use_sz = 0 ;
    return 1 ;
}

static void free_dict(Dict *dict)
{
    if(!dict)
        return ;

    DictNode *next = dict ->head_node;
    while(next)
    {
        DictNode *tmp = next ->next ;
        json_value_func.free(next ->value) ;
        free(next) ; 
        next = tmp ;
    }
}

static void deconstruct_json_dict(JsonDict *json_dict)
{
    if(!json_dict)
        return ; 
    size_t i = 0 ;
    for(i=0; i < json_dict ->total_sz; i++)
    {
        free_dict(json_dict ->dict+i) ;    
    }
    free(json_dict ->dict) ;
    json_dict ->dict = null ;  
    json_dict ->use_sz = 0 ;
    json_dict ->total_sz = 0 ;
}


static void free_json_dict(JsonDict *json_dict)
{
    json_dict_func.deconstruct(json_dict) ;
    free(json_dict) ;
    json_dict = null ;
}

static JsonDict *malloc_json_dict(size_t sz)
{
    JsonDict *json_dict = (JsonDict *)calloc(1, sizeof(JsonDict)) ; 
    if(!json_dict)
        return null;
    if(!json_dict_func.init(json_dict, sz))
    {
        json_dict_func.free(json_dict) ;
        return null ;
    }
    return json_dict ;
}

static int dict_hash(const JsonDict *json_dict, const JsonString *key)
{
    if(!key || !json_dict)
        return -1 ;
    unsigned v = json_string_func.hash(key) ; 
    return v % json_dict ->total_sz ;
}

static int dict_is_exist(const JsonDict *json_dict, const JsonString *key)
{
    return json_dict_func.__find(json_dict, key) != null ;
}

static DictNode* dict_find(const JsonDict *json_dict, const JsonString *key)
{
    if(!json_dict || !key)
        return null ;
    int idx = json_dict_func.hash(json_dict, key) ;
    if(idx == -1)
        return null ;
    DictNode *dict_node = json_dict ->dict[idx].head_node ;
    while(dict_node)
    {
        if(json_string_func.cmp(dict_node->key, key) == 0)
            return dict_node ;
        dict_node = dict_node ->next ;
    }
    return null ;
}


static JsonValue *dict_get(const JsonDict *json_dict, const JsonString *key)
{
    DictNode *dict_node = json_dict_func.__find(json_dict, key) ;
    return dict_node ? dict_node ->value : null;
}

static JsonValue *dict_delete(JsonDict *json_dict, const JsonString *key)
{
    
}

static JsonValue *dict_set(JsonDict *json_dict, JsonString *key, JsonValue *value)
{
    DictNode *dict_node = json_dict_func.__find(json_dict, key) ;
    if(dict_node)
    {
        json_value_func.free(dict_node ->value) ;
        dict_node ->value = value ;
        return value ;
    }
    int idx = json_dict_func.hash(json_dict, key) ;
    if(idx == -1)
        return null ;
    dict_node = (DictNode *)malloc(sizeof(DictNode)) ;
    if(!dict_node)
        return null ;
    dict_node ->next = null ;
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
