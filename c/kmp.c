#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int *get_next(const char *p, int *next)
{
    if(!p)
        return NULL;
    int l = strlen(p);
    if(!next)
        next = (int *)malloc(sizeof(int)*l) ;
    next[0] = -1;
    int i = 0 ;
    while( i < l - 1)
    {
        int j = i ;
        while(j > 0  && p[next[j]] != p[i])
            j = next[j] ;
        next[i+1] = next[j] + 1;
        i++;
    }
    return next ;
}

int kmp_search(const char *src, const char *p)
{
    const char *tmp = src ;
    int p_l = strlen(p);
    int *next = get_next(p, NULL);
    int j = 0;
    while(*src != '\0')
    {
        if(j == -1 || *src == p[j])
        {
            src++ ;
            j++;
            if(j == p_l)
                break ;
        }
        else
        {
            j = next[j] ;
        }
    }
    free(next) ;
    if(j == p_l)
        return src - j - tmp ;
    return -1;
}

void print(int *next, int sz)
{
    int i = 0;
    for(i=0; i < sz ; i++)
        printf("%-4d", next[i]) ;
    printf("\n") ;
}

int main(int argc, char *argv[])
{
    const char *p = "aaaa" ;
    int *next = get_next(p, NULL) ;
    printf("%s\n", p) ;
    print(next, strlen(p)) ;
    free(next);
    p = "abaabcac" ;
    next = get_next(p, NULL) ;
    printf("%s\n", p) ;
    print(next, strlen(p)) ;
    free(next) ;
    const char *src = "aaabaaaa" ;
    p = "aaaa" ;
    printf("[%s] at [%s] pos=[%d]\n", p, src, kmp_search(src, p)) ;
    p = "aaa" ;
    printf("[%s] at [%s] pos=[%d]\n", p, src, kmp_search(src, p)) ;

    p = "aac" ;
    printf("[%s] at [%s] pos=[%d]\n", p, src, kmp_search(src, p)) ;
    p = "aba" ;
    printf("[%s] at [%s] pos=[%d]\n", p, src, kmp_search(src, p)) ;

    return 0;
}
