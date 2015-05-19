#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jsonvalue.h"
#include "jsonarray.h"
#include "jsonstring.h"
#include "jsondict.h"

/*
struct JsonValueFunc
{
	JsonValue* (*malloc)(JsonValueType type) ;
	void (*free) (JsonValue *json_value) ;
};
*/

#ifndef nullptr
#define nullptr NULL
#endif

static void free_json_object(JsonValue *json_value) ;
static int init_json_value(JsonValue *json_value) ;
static void deconstruct_json_value(JsonValue *json_value) ;
static JsonValue *malloc_json_object(JsonValueType type) ;

JsonValueFunc json_value_func = {.malloc = malloc_json_object,
                                 .init = init_json_value,
                                 .free = free_json_object,
                                 .deconstruct = deconstruct_json_value} ;

extern JsonDictFunc   json_dict_func ;
extern JsonStringFunc json_string_func ;
extern JsonArrayFunc  json_array_func ;

JsonValue *get_null_json_value()
{
    static JsonValue json_null_value = {.type = NULL_VALUE_TYPE} ;
    return &json_null_value ;
}

JsonValue *get_false_json_value()
{
    static JsonValue json_false_value = {.type = BOOLEAN_VALUE_TYPE,
                                         .object.b = 0} ;    
    return &json_false_value ;
}

JsonValue *get_true_json_value()
{
    static JsonValue json_true_value = {.type = BOOLEAN_VALUE_TYPE,
                                        .object.b = 1} ;
    return &json_true_value ;
}

JsonValueFunc *get_json_object_func()
{
    return &json_value_func ;
}

static int init_json_value(JsonValue *json_value)
{
    if(!json_value)
        return 0 ;
    if(json_value ->type == DICT_VALUE_TYPE)
    {
        return json_dict_func.init(&json_value ->object.json_dict, 0) ;
    }
    else if(json_value ->type == STRING_VALUE_TYPE)
    {
        return json_string_func.init(&json_value ->object.json_string, 0) ;
    }
    else if(json_value ->type == ARRAY_VALUE_TYPE)
    {
        return json_array_func.init(&json_value ->object.json_array, 0) ;
    }
    return 1 ;
}

static JsonValue *malloc_json_object(JsonValueType type)
{
    JsonValue *json_value = (JsonValue *)calloc(1, sizeof(JsonValue)) ;
    if(!json_value)
        return nullptr ;

    json_value ->type = type ;
    if(!json_value_func.init(json_value))
    {
        json_value_func.free(json_value) ;
        return nullptr ;
    }
    return json_value;
}

static void deconstruct_json_value(JsonValue *json_value)
{
    if(!json_value)
        return ;

    if(json_value ->type == STRING_VALUE_TYPE)
    {
        json_string_func.deconstruct(&json_value ->object.json_string) ;
    }
    else if(json_value ->type == DICT_VALUE_TYPE)
    {
        json_dict_func.deconstruct(&json_value ->object.json_dict) ;   
    }
    else if(json_value ->type == ARRAY_VALUE_TYPE)
    {
        json_array_func.deconstruct(&json_value ->object.json_array) ;
    }
}
    
static void free_json_object(JsonValue *json_value)
{
    if(!json_value)
        return ;
    json_value_func.deconstruct(json_value) ;
    if (json_value ->type == BOOLEAN_VALUE_TYPE || json_value ->type == NULL_VALUE_TYPE)
        return ;
    free(json_value) ;
}

//测试显示
static void print_spaces(int n)
{
    for(; n > 0; n--)
        printf(" ") ;
}

void print_json_object(JsonValue *json_value, int nspaces) ;

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
            printf("\n") ;
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

void print_json_object(JsonValue *json_value, int nspaces)
{
    if(!json_value)
        return;
    print_spaces(nspaces) ;
    if(json_value ->type == STRING_VALUE_TYPE)
    {
        print_json_string(&json_value ->object.json_string, 0) ;
    }
    else if(json_value ->type == INTEGER_VALUE_TYPE)
    {
        printf("%ld", json_value ->object.i) ;
    }
    else if (json_value ->type == DOUBLE_VALUE_TYPE)
    {
        printf("%lf", json_value ->object.d) ;
    }
    else if(json_value ->type == DICT_VALUE_TYPE)
    {
        printf("{\n") ;
        print_json_dict(&json_value ->object.json_dict, nspaces+2) ;
        print_spaces(nspaces) ;
        printf("}\n") ;
    }
    else if(json_value ->type == ARRAY_VALUE_TYPE)
    {
        printf("[\n") ;
        print_json_array(&json_value ->object.json_array, nspaces+4) ;
        printf("]\n") ;
    }
    else if(json_value ->type == NULL_VALUE_TYPE)
    {
        printf("null") ;
    }
    else if (json_value ->type == BOOLEAN_VALUE_TYPE)
    {
        printf("%s", json_value ->object.b ? "true": "false") ;
    }
}

#ifdef JSON_VALUE
int main(int argc, char *argv[])
{
    return 0;
}
#endif
