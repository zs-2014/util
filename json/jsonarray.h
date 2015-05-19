#ifndef __JSON_ARRAY__H
#define __JSON_ARRAY__H

struct JsonValue ;

typedef struct JsonArray
{
	struct JsonValue **json_objects ;	
	int total_sz ;
	int use_sz ;
}JsonArray ;

typedef struct JsonArrayFunc
{
	JsonArray* (*malloc)(size_t sz) ;
	int (*init) (JsonArray *json_ay, size_t sz) ;
	void (*free)(JsonArray *json_ay) ;
    void (*deconstruct) (JsonArray *json_ay) ;
	int (*append)(JsonArray *json_ay, struct JsonValue *json_value) ;
}JsonArrayFunc;

#endif
