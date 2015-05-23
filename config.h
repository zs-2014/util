#ifndef __config__h
#define __config__h

struct LinkedList ;
struct LinkHashTable ; 

typedef struct ConfigSection
{
    char *section_name ;
	struct LinkHashTable *lh_table ;
}ConfigSection ;

typedef struct Config
{		
	struct LinkedList *llst ;
	char *fname ;
	char errbuff[512] ;
}Config ;

#ifdef __cplusplus
extern "C" {
#endif

extern Config *read_config(const char *file_name) ;
extern char *read_value(Config *config, const char *section_name, const char *key, char *value_buff) ;
extern int config_is_ok(Config *config) ;
extern void free_config(Config *config) ;
#ifdef __cplusplus
}
#endif
#endif
