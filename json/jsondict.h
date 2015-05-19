#ifndef __JSON_DICT__H
#define __JSON_DICT__H

struct JsonString;
struct Dict;
struct DictNode;
struct JsonValue;

typedef struct DictNode
{
	struct JsonString *key ;
	struct JsonValue *value ;	
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
	int (*init) (JsonDict *json_dict, size_t sz) ;
    void (*deconstruct) (JsonDict *json_dict) ;
	void (*free) (JsonDict *json_dict) ;
    int (*hash) (const JsonDict *json_dict, const struct JsonString *key) ;
	int (*is_exist) (const JsonDict *json_dict, const struct JsonString *key) ;
	struct JsonValue* (*set) (JsonDict *json_dict, struct JsonString *key, struct JsonValue *value) ;
	struct JsonValue* (*get) (const JsonDict *json_dict, const struct JsonString *key) ;
    struct DictNode *(*__find) (const JsonDict *json_dict, const struct JsonString *key) ;

}JsonDictFunc;

#ifdef __cplusplus
extern "C" {
#endif

extern JsonDictFunc* get_json_dict_func() ;

#ifdef __cplusplus
}
#endif

#endif
