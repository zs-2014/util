#ifndef __JSON__OBJECT__H
#define __JSON__OBJECT__H

typedef enum JsonObjectType
{
	NUMBER_OBJECT_TYPE = 0,
	BOOLEAN_OBJECT_TYPE,
	NULL_OBJECT_TYPE,
	STRING_OBJECT_TYPE,
	ARRAY_OBJECT_TYPE, 
	DICT_OBJECT_TYPE,

    //细分类型
    INTEGER_OBJECT_TYPE,
    DOUBLE_OBJECT_TYPE
}JsonObjectType;


typedef int boolean ;

typedef struct JsonObject
{
	JsonObjectType type ;
	union
	{	
		void *object;	
		int64_t i ;
		double d ;
		boolean b ;
	}object ;
}JsonObject ;

typedef struct JsonObjectFunc
{
	JsonObject* (*malloc)(JsonObjectType type) ;
	void (*free) (JsonObject *json_object) ;
}JsonObjectFunc;

#ifdef __cplusplus
extern "C" {
#endif

extern JsonObjectFunc *get_json_object_func() ;

#ifdef __cplusplus
}
#endif

#endif
