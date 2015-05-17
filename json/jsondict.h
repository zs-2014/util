#ifndef __JSON_DICT__H
#define __JSON_DICT__H

struct JsonString;
struct Dict;
struct DictNode;
struct JsonObject;

typedef struct DictNode
{
	struct JsonString *key ;
	struct JsonObject *value ;	
	struct DictNode *next ;
}DictNode; 

typedef struct Dict
{
	DictNode *head_node ;
	DictNode *tail_node ;
}Dict;

typedef struct JsonDict
{
	Dict *dict ;
	size_t total_sz ;
	size_t use_sz ;
}JsonDict ;


typedef struct JsonDictFunc
{
	JsonDict* (*malloc) (size_t sz) ;		
	void (*free) (JsonDict *json_dict) ;
    int (*hash)(const JsonDict *json_dict, const JsonString *key) ;
	int (*is_exist) (const JsonDict *json_dict, const struct JsonString *key) ;
	struct JsonObject* (*set) (JsonDict *json_dict, struct JsonString *key, struct JsonObject *value) ;
	struct JsonObject* (*get) (const JsonDict *json_dict, const struct JsonString *key) ;
}JsonDictFunc;

#ifdef __cplusplus
extern "C" {
#endif

extern JsonDictFunc* get_json_dict_func() ;

#ifdef __cplusplus
}
#endif

#endif
