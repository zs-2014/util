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
#ifdef __cplusplus
}
#endif
#endif
