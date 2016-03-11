#ifndef __common___h
#define __common___h

#ifndef MALLOC
#define MALLOC(sz) malloc(sz)
#endif

#ifndef FREE
#define FREE(ptr) free(ptr)
#endif

#ifndef CALLOC
#define CALLOC(n, sz) calloc(n, sz)
#endif

#ifndef REALLOC
#define REALLOC(ptr, sz) realloc(ptr, sz);
#endif

typedef int (*ITEM_CMP_FUNC)(const void *v1, const void *v2);
typedef void (*FREE_ITEM_FUNC)(void *);

#ifndef MAX
#define MAX(a, b) (a) > (b) ? (a):(b)
#endif

#ifndef MIN
#define MIN(a, b) (a) > (b) ? (b):(a)
#endif

#endif
