#ifndef ___logging___h
#define ___logging___h

#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>

enum LogLevel
{
    LOG_LEVEL_NOT_SET=0,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR
};

struct FileHandler
{
	char *file_name ;
	char mode[10] ;
	FILE *fp ;
	struct stat st ;
};

struct StreamHandler
{
    //stderr, stdout
    char stream_name[10];
    FILE *fp ;
};

struct Handler
{
    enum LogLevel level ;
    union
    {
        struct FileHandler f_handler ;
        struct StreamHandler stm_handler ;
    }handler ;

    int (*write)(void *handler, const char *msg, int sz, enum LogLevel lvl) ;
    void (*free)(void *handler) ;
};

struct LinkedList ;

struct Logger
{
	char *name ;
	struct Logger *parent ;
	struct LinkedList *handler_list ;
	enum LogLevel log_level ;
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct Logger *get_root_logger() ;
extern enum LogLevel log_name_to_level(const char *level_name) ;
extern void disalbe_logger(struct Logger *logger) ;
extern struct Logger *get_logger(const char *log_name) ;
extern void exit_logger() ;
extern void set_handler_level(struct Handler *handler, enum LogLevel lvl) ;
extern void set_logger_level(struct Logger *logger, enum LogLevel lvl) ;
extern struct Handler *add_file_handler(struct Logger *logger, const char *file_name, const char *mode, enum LogLevel level);
extern struct Handler *add_stream_handler(struct Logger *logger, const char *stm_name, enum LogLevel lvl) ;

extern int debug(struct Logger *logger, const char *fmt, ...) ;
extern int info(struct Logger *logger, const char *fmt, ...) ;
extern int warn(struct Logger *logger, const char *fmt, ...) ;
extern int error(struct Logger *logger, const char *fmt, ...) ;
#ifdef __cplusplus
}
#endif

#endif
