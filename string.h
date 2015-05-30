#ifndef ___string___h
#define ___string___h


#ifdef __cplusplus
extern "C" {
#endif

extern char *strip(char *str, char ch) ;
extern char *tail_strip(char *str, char ch) ;
extern char *head_strip(char *str, char ch) ;
extern LinkedList *split(const char *buff, char c, int n, int drop_null_str) ;

#ifdef __cplusplus
}
#endif

#endif
