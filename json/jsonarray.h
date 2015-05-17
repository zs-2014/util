#ifndef __JSON_ARRAY__H
#define __JSON_ARRAY__H

struct JsonObject ;

typedef struct JsonArray
{
	struct JsonObject **json_objects ;	
	int total_sz ;
	int use_sz ;
}JsonArray ;

typedef struct JsonArrayFunc
{
	JsonArray* (*malloc)(size_t sz) ;		
	void (*free)(JsonArray *json_ay) ;
	int (*append)(JsonArray *json_ay, struct JsonObject *json_object) ;
}JsonArrayFunc;

#endif
