#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>

#include "linklist.h"
#include "linkhash.h"
#include "hash.h"
#include "logging.h"

static void free_logger(struct Logger *logger) ;

static struct LinkHashTable *g_lh_table = NULL ;
static struct Logger *root_logger = NULL ;

static void init_logger_manager()
{
    if(!g_lh_table)
    {
        g_lh_table  = malloc_link_hash_table(0) ;
        g_lh_table ->free_value = (void (*)(void *))free_logger ;
        g_lh_table ->cmp = (int (*)(const void *, const void *))strcmp ;
        g_lh_table ->hash = (size_t (*)(const void *))SDBMHash ;
        g_lh_table ->dup_key = (void *(*)(void *))strdup ;
    }
}

static void free_logger(struct Logger *logger)
{
    if(!logger)
        return ;
    printf("free logger:[%-10s] at %p\n", logger ->name ? logger ->name :"root", logger) ;
    free(logger ->name) ;
    logger ->name = NULL ;
    logger ->parent = NULL ;
    free_linked_list(logger ->handler_list) ; 
    logger ->handler_list = NULL ;
    free(logger) ;
    logger = NULL ;
}


static void free_handler(void *handler)
{
    ((struct Handler *)handler) ->free(handler) ;
}

static void free_file_handler(void *handler)
{
    if(!handler)
        return ;
    struct FileHandler *f_handler = &(((struct Handler *)handler) ->handler.f_handler) ;
    if(f_handler ->fp)
        fclose(f_handler ->fp) ;
    f_handler ->fp = NULL ;
    free(f_handler ->file_name) ;
    f_handler = NULL ;
    free(handler) ;
}

static int file_write(void *handler, const char *msg, int sz, enum LogLevel lvl)
{
    struct FileHandler *f_handler = &(((struct Handler *)handler) ->handler.f_handler) ;
    int ret = fwrite(msg, sz, 1, f_handler ->fp) == 1?sz:0;
    fflush(f_handler ->fp) ;
    return ret ;
}

static int std_stream_write(void *handler, const char *msg, int sz, enum LogLevel lvl)
{
    //NOT_SET, DEBUG, INFO, WARN, ERROR
    struct Handler *base_handler = (struct Handler *)handler ;
    char *buff[] = {"\33[2;39m", "\33[0;37m", "\33[2;39m", "\33[0;33m", "\33[0;35m"} ; 
    struct StreamHandler *stm_handler = &(base_handler ->handler.stm_handler) ;
    char *color = buff[lvl - LOG_LEVEL_NOT_SET] ;
    fwrite(color, strlen(color), 1, stm_handler ->fp) ;
    int ret = fwrite(msg, sz, 1, stm_handler ->fp) == 1?sz:0;
    color = buff[LOG_LEVEL_NOT_SET] ;
    fwrite(color, strlen(color), 1, stm_handler ->fp) ;
    return ret ;
}

static void free_std_stream_handler(void *handler)
{
    free(handler) ; 
}

static struct Handler *new_file_handler(const char *file_name, const char *mode)
{
    if(!file_name)
        return NULL ;
    struct Handler *handler = (struct Handler *)calloc(1, sizeof(struct Handler)) ;
    if(!handler)
        return NULL ;
    struct FileHandler *f_handler = & handler ->handler.f_handler ;
    if(!mode)
        f_handler ->mode[0] = 'a' ;
    else
        strncpy(f_handler ->mode, mode, sizeof(f_handler ->mode)-1) ;
    f_handler ->file_name = strdup(file_name) ;
    f_handler ->fp = fopen(f_handler ->file_name, f_handler ->mode) ;
    if(!f_handler ->fp)
    {
        printf("fail to open:%s error:%s\n", file_name, strerror(errno)) ;
        free_handler(f_handler) ;
        return NULL ;
    }
    handler ->level = LOG_LEVEL_NOT_SET; 
    handler ->write = file_write;
    handler ->free = free_file_handler;
    return handler ;
}

void exit_logger()
{
    free_logger(root_logger) ;
    root_logger = NULL;
    free_link_hash_table(g_lh_table) ;
    g_lh_table = NULL ;
}

void disable_logger(struct Logger *logger)
{
    if(!logger)
        return ;
    logger ->log_level = LOG_LEVEL_NOT_SET ;
}

struct Logger *get_root_logger()
{
    if(root_logger)
        return root_logger ;

    struct Logger *logger = (struct Logger *)calloc(1, sizeof(struct Logger)) ;
    if(!logger)
        return NULL ; 
    logger ->log_level = LOG_LEVEL_NOT_SET;    
    logger ->handler_list = malloc_linked_list();
    logger ->handler_list ->free_value = free_handler ;
    logger ->parent = NULL;
    root_logger = logger ;
    return logger ;

}

enum LogLevel log_name_to_level(const char *level_name)
{
    if(!level_name)
        return -1 ;
    if(strcasecmp(level_name, "INFO") == 0)
        return LOG_LEVEL_INFO ;
    else if(strcasecmp(level_name, "DEBUG") == 0)
        return LOG_LEVEL_DEBUG ;
    else if(strcasecmp(level_name, "WARN") == 0)
        return LOG_LEVEL_WARN ;
    else if(strcasecmp(level_name, "ERROR") == 0)
        return LOG_LEVEL_ERROR ;
    else
        return -1 ;
}

struct Logger *get_logger(const char *log_name)
{
    init_logger_manager() ;
    struct Logger *root_logger = get_root_logger() ;
    if(!log_name)
        return root_logger ;
    
    struct Logger *logger = get_from_link_hash_table(g_lh_table, log_name) ;
    if(logger)
        return logger ;
    logger = (struct Logger *)calloc(1, sizeof(struct Logger)) ;
    if(!logger)
        return NULL ; 
    logger ->name = strdup(log_name) ;
    struct Logger *parent_logger = root_logger ; 
    char *parent = strrchr(logger ->name, '.') ;
    if(parent)
    {
        *parent = '\0' ;
        parent_logger = get_logger(logger ->name) ;
        *parent = '.' ;
    }
    logger ->log_level = LOG_LEVEL_NOT_SET;    
    logger ->handler_list = malloc_linked_list();
    logger ->handler_list ->free_value = free_handler ;
    if(!parent_logger)
    {
        free_logger(logger) ;
        return NULL ;
    } 
    logger ->parent = parent_logger;
    set_to_link_hash_table(g_lh_table, logger ->name, logger) ;
    return logger ;
}

void set_logger_level(struct Logger *logger, enum LogLevel lvl)
{
    if(!logger)
        return ;
    logger ->log_level = lvl ;
}

void set_handler_level(struct Handler *handler, enum LogLevel lvl)
{
    if(!handler)
        return ;
    handler ->level = lvl ;
}

struct Handler *new_std_stream_handler(FILE *fp)
{
    if(!fp) 
        return NULL ;
    struct Handler *handler = (struct Handler *)calloc(1, sizeof(struct Handler)) ;
    if(!handler)
        return NULL ;
    struct StreamHandler *stm_handler = &handler ->handler.stm_handler ;
    if(fp == stdout)
        strcpy(stm_handler ->stream_name, "stdout") ;
    else
        strcpy(stm_handler ->stream_name, "stderr") ;
    stm_handler ->fp = fp ;
    handler ->free =  free_std_stream_handler ;
    handler ->write = std_stream_write ;
    handler ->level = LOG_LEVEL_NOT_SET;
    return handler ;
}

struct Handler *add_file_handler(struct Logger *logger, const char *file_name, const char *mode, enum LogLevel level)
{
    if(!logger)
        return NULL;
    struct Handler *f_handler = new_file_handler(file_name, mode) ;
    if(!f_handler)
        return NULL;
    f_handler ->level = level ;
    append_to_linked_list(logger ->handler_list, f_handler) ; 
    return f_handler;
}

struct Handler *add_stream_handler(struct Logger *logger, const char *stm_name, enum LogLevel lvl)
{
    struct Handler *stm_handler = NULL ;
    if(strcasecmp(stm_name, "stdout") == 0)
        stm_handler = new_std_stream_handler(stdout) ;
    else if(strcasecmp(stm_name, "stderr") == 0)
        stm_handler = new_std_stream_handler(stderr) ;
    else
    {
        printf("unsupport standard stream:%s\n", stm_name) ;
        return NULL ;
    }
    if(!stm_handler)
        return NULL ;
    append_to_linked_list(logger ->handler_list, stm_handler) ;
    stm_handler ->level = lvl ;
    return stm_handler ;
}

#define LEVEL_NAME(lvl) (lvl == LOG_LEVEL_DEBUG ? "DEBUG":\
                        (lvl == LOG_LEVEL_INFO  ? "INFO":\
                        (lvl == LOG_LEVEL_WARN  ? "WARN":\
                        (lvl == LOG_LEVEL_ERROR ? "ERROR":"NOT_SET")))) 

static int _log(struct Logger *logger, enum LogLevel lvl, const char *fmt, va_list argument_lst)
{
    char buff[4096] = {0} ;
    int total_sz = vsnprintf(buff, sizeof(buff)-1, fmt, argument_lst) ;
    if(total_sz > 0)
        buff[total_sz++] = '\n' ;
    for(; logger != NULL; logger = logger ->parent)
    { 
        if(logger ->log_level > lvl || logger ->log_level == LOG_LEVEL_NOT_SET)
            continue ;
            
        struct Handler *handler ;
        struct LinkedList *lst = logger ->handler_list ;
        for_each_in_linked_list(lst, handler)
        {
            if(handler ->level <= lvl)
                handler ->write(handler, buff, total_sz, lvl) ;
        }
    }

    return total_sz ;
}

#define __log_with_level(logger, fmt, lvl) ({\
    va_list argument_lst ;\
    va_start(argument_lst, fmt) ;\
    int ret = _log(logger, lvl, fmt, argument_lst) ;\
    va_end(argument_lst) ;\
    ret ;})

int debug(struct Logger *logger, const char *fmt, ...)
{
    return __log_with_level(logger, fmt, LOG_LEVEL_DEBUG) ;
}

int info(struct Logger *logger, const char *fmt, ...)
{
    return __log_with_level(logger, fmt, LOG_LEVEL_INFO) ;
}

int warn(struct Logger *logger, const char *fmt, ...)
{
    return __log_with_level(logger, fmt, LOG_LEVEL_WARN) ;
}

int error(struct Logger *logger, const char *fmt, ...)
{
    return __log_with_level(logger, fmt, LOG_LEVEL_ERROR) ;
}


#ifdef LOGGING
void print_logger(char *log_name)
{ 
    struct Logger *logger = get_logger(log_name) ;
    printf("{logger [%s] at %p} --->", logger ->name?logger ->name:"root", logger) ;
    char *parent_log_name = strrchr(log_name, '.') ;
    if(parent_log_name)
    {
        *parent_log_name = '\0' ;
        print_logger(log_name) ; 
        *parent_log_name = '.' ;
        return ;
    }
    printf("{logger [root] at %p}\n", root_logger) ;
}

void test_logger(int argc, char *argv[])
{
    if(argc < 2)
        return ;
    for(; --argc > 0; )
    {    
        get_logger(argv[argc]) ;
        print_logger(argv[argc]) ;
    }
    exit_logger() ;
}

int main(int argc, char *argv[])
{
    test_logger(argc, argv) ;
    struct Logger *logger = get_logger("a.b.c.d") ;
    logger = get_logger(NULL) ;
    set_logger_level(logger, LOG_LEVEL_INFO) ;
    //add_file_handler(logger, "test.log", NULL, LOG_LEVEL_INFO) ;
    //add_file_handler(logger, "test.log", NULL, LOG_LEVEL_ERROR) ;
    add_stream_handler(logger, "stdout", LOG_LEVEL_WARN) ;
    add_stream_handler(logger, "stdout", LOG_LEVEL_INFO) ;
    add_stream_handler(logger, "stdout", LOG_LEVEL_DEBUG) ;
    add_stream_handler(logger, "stdout", LOG_LEVEL_ERROR) ;
    logger = get_logger("a.b.c.d") ;
    int sz = info(logger, "%s:%d this is a info message\n", __FILE__, __LINE__) ;
    sz = debug(logger, "%s:%d this is a debug message\n", __FILE__, __LINE__) ;
    sz = warn(logger, "%s:%d this is a warn message\n", __FILE__, __LINE__) ;
    sz = error(logger, "%s:%d this is a error message", __FILE__, __LINE__) ;
    printf("---------------------shutdown logging system---------------------\n") ;
    exit_logger() ;
}
#endif
