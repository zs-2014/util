#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jsonobject.h"
#include "jsonarray.h"
#include "jsonstring.h"
#include "jsondict.h"

/*
struct JsonObjectFunc
{
	JsonObject* (*malloc)(JsonObjectType type) ;
	void (*free) (JsonObject *json_object) ;
};
*/

#ifndef nullptr
#define nullptr NULL
#endif

static void free_json_object(JsonObject *json_object) ;
static JsonObject *malloc_json_object(JsonObjectType type) ;

JsonObjectFunc json_object_func = {.malloc = malloc_json_object,
                                   .free = free_json_object} ;
extern JsonDictFunc   json_dict_func ;
extern JsonStringFunc json_string_func ;
extern JsonArrayFunc  json_array_func ;

JsonObjectFunc *get_json_object_func()
{
    return &json_object_func ;
}

static JsonObject *malloc_json_object(JsonObjectType type)
{
    JsonObject * json_object = (JsonObject *)malloc(sizeof(JsonObject)) ;
    if(!json_object)
        return nullptr ;
    json_object ->type = type ;
    json_object ->object.object = nullptr ;
    return json_object ;
}

static int type_in(JsonObjectType type, JsonObjectType *ay, int sz)
{
    if(!ay || !sz)
        return 0;
    for(; --sz >= 0; ) 
        if(ay[sz] == type)
            return 1;
    return 0 ;
}
    
static void free_json_object(JsonObject *json_object)
{
    if(!json_object)
        return ;
    JsonObjectType directly_free_types[] = {NUMBER_OBJECT_TYPE, BOOLEAN_OBJECT_TYPE, 
                                            NULL_OBJECT_TYPE, INTEGER_OBJECT_TYPE, 
                                            DOUBLE_OBJECT_TYPE} ;
    if(!type_in(json_object ->type, directly_free_types, sizeof(directly_free_types)/sizeof(directly_free_types[0])))
    {
    }
    else if(json_object ->type == STRING_OBJECT_TYPE)
    {
        json_string_func.free(json_object ->object.object) ;
    }
    else if(json_object ->type == DICT_OBJECT_TYPE)
    {
        json_dict_func.free(json_object ->object.object) ;   
    }
    else if(json_object ->type == ARRAY_OBJECT_TYPE)
    {
        json_array_func.free(json_object ->object.object) ;
    }
    free(json_object) ;
}

//测试显示
static void print_spaces(int n)
{
    for(; n > 0; n--)
        printf(" ") ;
}

void print_json_object(JsonObject *json_object, int nspaces) ;

static void print_json_string(JsonString *json_string, int print_enter)
{
    int i = 0 ;
    for(i=0; i < json_string ->use_sz; i++)
    {
        printf("%c", json_string ->buff[i]) ;
    }
    if(print_enter)
        printf("\n") ;
}



static void print_dict_node(Dict *dict, int nspaces)
{
    if(!dict)
        return ;
    DictNode *dict_node = dict ->head_node ;
    //key:value
    while(dict_node)
    {
        print_spaces(nspaces) ;
        print_json_string(dict_node ->key, 0) ;
        printf(":") ;
        print_json_object(dict_node ->value, 0) ;
        if(dict_node ->next)
            printf(",\n") ;
        else
            printf("\n") ;
        dict_node = dict_node ->next ;
    }
}

static void print_json_dict(JsonDict *json_dict, int nspaces)
{
    if(!json_dict)
        return ;
    int i = 0 ; 
    Dict *dict = json_dict ->dict ;
    for(i=0; i < json_dict ->total_sz; i++)
    {
        print_dict_node(dict+i, nspaces) ;
    }
}

static void print_json_array(JsonArray *json_ay, int nspaces)
{
    if(!json_ay)
        return ;
    int i = 0 ;
    for(i=0; i < json_ay ->use_sz; i++)
    {
        print_json_object(json_ay ->json_objects[i], nspaces) ; 
        if(i + 1 != json_ay ->use_sz)
            printf(",\n") ;
    }
}

void print_json_object(JsonObject *json_object, int nspaces)
{
    if(!json_object)
        return;
    print_spaces(nspaces) ;
    if(json_object ->type == STRING_OBJECT_TYPE)
    {
        print_json_string(json_object ->object.object, 0) ;
    }
    else if(json_object ->type == INTEGER_OBJECT_TYPE)
    {
        printf("%ld", json_object ->object.i) ;
    }
    else if (json_object ->type == DOUBLE_OBJECT_TYPE)
    {
        printf("%lf", json_object ->object.d) ;
    }
    else if(json_object ->type == DICT_OBJECT_TYPE)
    {
        printf("{\n") ;
        print_json_dict(json_object ->object.object, nspaces+2) ;
        print_spaces(nspaces) ;
        printf("}\n") ;
    }
    else if(json_object ->type == ARRAY_OBJECT_TYPE)
    {
        printf("[\n") ;
        print_json_array(json_object ->object.object, nspaces+4) ;
        printf("]\n") ;
    }
    else if(json_object ->type == NULL_OBJECT_TYPE)
    {
        printf("null") ;
    }
    else if (json_object ->type == BOOLEAN_OBJECT_TYPE)
    {
        printf("%s", json_object ->object.b ? "true": "false") ;
    }
}

#ifdef JSON_OBJECT
int main(int argc, char *argv[])
{
    return 0;
}
#endif
