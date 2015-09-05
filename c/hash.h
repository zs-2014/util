#ifndef __HASH__H
#define __HASH__H

#ifdef __cplusplus
extern "C" {
#endif

extern size_t BKDRHash(const char *str) ;
extern size_t SDBMHash(const char *str) ;
extern size_t RSHash(const char *str) ;
extern size_t APHash(const char *str) ;
extern size_t JSHash(const char *str) ;
extern size_t DEKHash(const char *str) ;
extern size_t FNVHash(const char *str) ;
extern size_t DJBHash(const char *str) ;
extern size_t time33(const char *str) ;
extern size_t ptrHash(const void *key) ;
#ifdef __cplusplus
}
#endif

#endif
