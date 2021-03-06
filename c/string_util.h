#ifndef ___string___h
#define ___string___h


struct LinkedList ;
#ifdef __cplusplus
extern "C" {
#endif

extern char *to_lower(char *str) ;
extern char *to_upper(char *str) ;
extern char *strip(char *str, char ch) ;
extern char *tail_strip(char *str, char ch) ;
extern char *head_strip(char *str, char ch) ;
extern char *itostr(int n, char *dst) ;
extern struct LinkedList *split(const char *buff, char c, int n, int drop_null_str) ;

#ifdef __cplusplus
}
#endif

#endif
