#ifndef __JSON__READER__H
#define __JSON__READER__H

#define nullptr NULL
#define INVALID_POS 0xFFFFFFFF

#include "jsonstring.h"

struct JsonValue;

typedef struct JsonReader
{
	struct JsonValue *json_value ;
    char *buff ;
    size_t total_sz;
    size_t curr_pos ;
	char errbuff[1024] ;	
}JsonReader;

typedef struct JsonReaderFunc
{	
	JsonReader *(*malloc)(const char *buff, size_t sz) ;
	int (*init) (JsonReader *json_reader, const char *buff, size_t sz) ;
	void (*free) (JsonReader *json_reader) ;
	void (*deconstruct) (JsonReader *json_reader) ;
	int (*parse)(JsonReader *json_reader) ; 
	struct JsonValue *(*get_json_value)(JsonReader *json_reader) ;
	int (*is_error) (JsonReader *json_reader) ;
}JsonReaderFunc;
#endif
