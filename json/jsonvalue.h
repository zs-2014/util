#ifndef __JSON__VALUE__H
#define __JSON__VALUE__H

typedef enum JsonValueType
{
	NUMBER_VALUE_TYPE = 0, 
	BOOLEAN_VALUE_TYPE, 
	NULL_VALUE_TYPE,
	STRING_VALUE_TYPE,
	ARRAY_VALUE_TYPE, 
	DICT_VALUE_TYPE,

    //细分类型
    INTEGER_VALUE_TYPE,
    DOUBLE_VALUE_TYPE
}JsonValueType;

#include "jsondict.h"
#include "jsonstring.h"
#include "jsonarray.h"

typedef int boolean ;

typedef struct JsonValue
{
	JsonValueType type ;
	union
	{
		struct JsonDict   json_dict ;
		struct JsonArray  json_array ;
		struct JsonString json_string ;
		double     d ;
		int64_t    i ;
		boolean    b ;
	}object ;
}JsonValue ;

typedef struct JsonValueFunc
{
	JsonValue* (*malloc)(JsonValueType type) ;
    int (*init) (JsonValue *json_value) ;
	void (*free) (JsonValue *json_value) ;
    void (*deconstruct) (JsonValue *json_value) ;
}JsonValueFunc;

#ifdef __cplusplus
extern "C" {
#endif

extern JsonValueFunc *get_json_object_func() ;
extern JsonValue *get_null_json_value() ;
extern JsonValue *get_false_json_value() ;
extern JsonValue *get_true_json_value() ;
#ifdef __cplusplus
}
#endif

#endif
