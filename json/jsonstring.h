#ifndef __JSON_STRING__H
#define __JSON_STRING__H

#ifndef nullptr
#define nullptr NULL
#endif 

typedef struct JsonString
{
	char *buff ;
	size_t total_sz ;
	size_t use_sz;
}JsonString ;

typedef struct JsonStringFunc
{
    char (*at)(const JsonString *json_string, int idx) ;
    int (*len) (const JsonString *json_string) ;
    int (*hash) (const JsonString *json_string) ;
    int (*cmp) (const JsonString *json_string1, const JsonString *json_string2) ;
	int (*get_grow_size) (const JsonString *json_string, size_t need_sz) ; 
	int (*ensure_memory)(JsonString *json_string, size_t need_sz) ;
	int (*append_char)(JsonString *json, char ch) ;
	int (*append_string)(JsonString *json, const char *str, size_t sz) ;
	JsonString* (*malloc)(size_t sz) ;
	void (*free)(JsonString *json_string) ;
}JsonStringFunc ;

#ifdef __cplusplus
extern "C" {
#endif

extern JsonStringFunc *get_json_string_func() ;

#ifdef __cplusplus
}
#endif

#endif
