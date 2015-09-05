#include <string.h>
#include <time.h>
#include "logging.h"
#include "log.h"

struct Logger *install_logger(const char *level, const char *access_log_file, const char *error_log_file)
{
    enum LogLevel log_level = log_name_to_level(level) ;
    if(log_level == -1)
        return NULL ;
    struct Logger *logger = get_logger(NULL) ;
    set_logger_level(logger, log_level) ;
    if(!strcasecmp(access_log_file, "stdout") || !strcasecmp(access_log_file, "stderr"))
    {
        if(add_stream_handler(logger, access_log_file, log_level) == NULL)
            return NULL ;
    }
    else
    {
        if(add_file_handler(logger, access_log_file, "a", log_level) == NULL) 
            return NULL ;
    }

    log_level = log_name_to_level("ERROR") ;
    if(!strcasecmp(error_log_file, "stdout") || !strcasecmp(error_log_file, "stderr"))
    {
        if(add_stream_handler(logger, error_log_file, log_level) == NULL)
            return NULL ;
    }
    else
    {
        if(add_file_handler(logger, error_log_file, "a", log_level) == NULL)
            return NULL ;
    }
    return logger ;
}

void shut_down_loggger()
{
    exit_logger() ;
}


#ifdef LOG
int main(int argc, char *argv[])
{
    struct Logger *logger = install_logger("DEBUG", "stdout", "stdout") ;
    DEBUG("this is message from DEBUG macro") ;
    DEBUG("this is message from DEBUG macro: %s", "one-argument") ;
    char buff[] = {"hello,world"} ;
    DEBUG(buff) ;
    INFO("this is message from macro") ;
    INFO("this is message from macro: %s", "one-argument") ;

    WARN("this is message from macro") ;
    WARN("this is message from macro: %s", "one-argument") ;

    ERROR("this is message from macro") ;
    ERROR("this is message from macro: %s", "one-argument") ;
}
#endif
