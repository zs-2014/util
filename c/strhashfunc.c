#include <stdio.h>
#include <stddef.h>

#include "hash.h"

size_t BKDRHash(const char *str)
{
    //31 131 1313 and so on
    size_t v = 0 ;
    while(*str != '\0')
        v = (v << 5) - v + *str++ ;

    return v ;
}

size_t SDBMHash(const char *str)
{
    size_t v = 0;
    while(*str != '\0')
        v = 65599 * v + *str++ ;
    return v ;
}

size_t RSHash(const char *str)
{
    size_t v = 0 ;
    size_t magic = 63689 ;
    while(*str != '\0')
    {
        v = v * magic + *str++ ;
        magic *= 378551 ;
    }
    return v ;
}

size_t aphash(const char *str)
{
    size_t v = 0 ;
    int flag = 0 ;
    while(*str != '\0')
    {
        if (flag)
            v ^= (~((v << 11) ^ (*str) ^ (v >> 5)));
        else
            v ^= (v << 7) ^ (*str) ^ (v >> 3) ; 
        flag = !flag ;
        str++ ;
    }
    return v ;
}

size_t JSHash(const char *str)
{
    size_t v = 1315423911 ;
    while(*str != '\0')
        v ^= (v << 5) + (*str++) + (v >> 2) ;
    return v ;
}

size_t DEKHash(const char *str)
{
    size_t v = 1315423911 ;
    while(*str != '\0')
        v = ((v << 7) ^ (v >> 27)) ^ (*str++) ;
    return v ;
}

size_t FNVHash(const char *str) 
{
    size_t v = 2166136261 ;
    while(*str != '\0')
    {
        v = v * 16777619 ;
        v = v ^ *str ;
        str++;
    }
    return v ;
}

size_t DJBHash(const char *str)
{
    size_t v = 5381 ;
    while(*str != '\0')
        v += (v << 5) + *str++ ;
    return v ;
}

size_t time33(const char *str)
{
    size_t v = 5381 ;
    while(*str != '\0')
        v = (v << 5) + v + *str++ ;

    return v ;
}

size_t ptrHash(const void *key)
{
    return ((ptrdiff_t)key* 0x9e370001UL) >> 4 ;
}

#if 0
int main(int argc, char *argv[])
{
    char *str = argc > 1 ? argv[1] : argv[0] ; 
    printf("input: [%s]\n", str) ;
    printf("BKDRHash:%u\nSDBMHash:%u\nRSHash:%u\n", BKDRHash(str), SDBMHash(str), RSHash(str)) ;
    printf("APHash:%u\nSHash:%u\nDEKHash:%u\n", APHash(str), JSHash(str), DEKHash(str)) ; 
    printf("FNVHash:%u\nDJBHash:%u\ntime33:%u\n", FNVHash(str), DJBHash(str), time33(str)) ; 
    printf("ptrHash:%u\n", ptrHash(str)) ; 
    return 0 ;
}
#endif
