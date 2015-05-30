#ifndef __buffer__h
#define __buffer__h

typedef struct Buffer
{
    unsigned char *buff ;
    size_t total_sz ;
    size_t use_sz ;

}Buffer ;

#ifdef __cplusplus
extern "C" {
#endif

extern Buffer *malloc_buffer(size_t sz) ;
extern void free_buffer(Buffer *buffer) ;
extern int init_buffer(Buffer *buffer, size_t need_sz) ;
extern void deconstruct_buffer(Buffer *buffer) ;

extern int append_str_to_buffer(Buffer *buffer, const char *str, size_t sz);
extern int append_char_to_buffer(Buffer *buffer, char ch) ;
extern len(const Buffer *buffer) ;

#ifdef __cplusplus
}
#endif

#endif
