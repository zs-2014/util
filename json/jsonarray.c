#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "jsonarray.h"
#include "jsonobject.h"

#ifndef nullptr
#define nullptr NULL
#endif

/*struct JsonArrayFunc
{
	JsonArray* (malloc)(size_t sz) ;		
	void (*free)(JsonArray *json_ay) ;
	int (*append)(JsonArray *json_ay, JsonObject *json_object) ;
}JsonArrayFunc;
*/

static int ensure_memory(JsonArray *json_ay, size_t need_sz) ;
static void free_json_array(JsonArray *json_ay) ;
static JsonArray *malloc_json_array(size_t sz) ;
static int append_to_json_array(JsonArray *json_ay, JsonObject *json_object) ;

extern JsonObjectFunc json_object_func ;
JsonArrayFunc json_array_func = {.malloc = malloc_json_array,
                                 .free = free_json_array,
                                 .append = append_to_json_array} ;

static void free_json_array(JsonArray *json_ay)
{
    if(!json_ay)
        return ;
    size_t i = 0 ;
    for(;i < json_ay ->use_sz; i++)
    {
        json_object_func.free(json_ay ->json_objects[i]) ;
    }
    json_ay ->use_sz = 0 ;
    json_ay ->total_sz = 0 ;
    free(json_ay) ;
}

static int ensure_memory(JsonArray *json_ay, size_t need_sz)
{
    if(!json_ay)
        return 0 ;
    if(!need_sz)
        return 1 ;

    if(json_ay ->use_sz + need_sz <= json_ay ->total_sz)
        return 1 ;
    size_t new_sz = need_sz + json_ay ->use_sz + 1 ;
    JsonObject **json_objects = (JsonObject **)malloc(sizeof(JsonObject *) * new_sz) ; 
    if(!json_objects)
        return 0 ;
    if(json_ay ->use_sz > 0)
    {
        memcpy((void *)json_objects, (void *)(json_ay ->json_objects), sizeof(JsonObject *)*json_ay ->use_sz) ;
        json_ay ->total_sz = new_sz ;
        free((void *)(json_ay ->json_objects)) ;
    }
    json_ay ->json_objects = json_objects ;
    return 1 ;
}

static JsonArray *malloc_json_array(size_t sz)
{
    JsonArray *json_array = (JsonArray *)malloc(sizeof(JsonArray)) ; 
    if(!json_array)
        return nullptr ;
    json_array ->json_objects = nullptr ;
    json_array ->total_sz = 0 ;
    json_array ->use_sz = 0 ;
    if(!ensure_memory(json_array, sz))
    {
        json_array_func.free(json_array) ;
        return nullptr ;
    }
    return json_array ;
}

static int append_to_json_array(JsonArray *json_ay, JsonObject *json_object)
{
    if(!ensure_memory(json_ay, 1))
        return 0;
    json_ay ->json_objects[json_ay ->use_sz++] = json_object ;
    return 1 ;
}

