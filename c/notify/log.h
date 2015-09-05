#ifndef __log___h
#define __log___h

#include <time.h>
#include "logging.h"
//time [level_name] file-name:func-name line-no msg
#define __LOG(fmt, level_name, func,  ...)  do{\
	struct Logger *logger = get_logger(NULL);\
	char header_fmt[] = {"%s [%s] %s:%s:%d "};\
	char fmt_buff[1024] = {0} ;\
	strcpy(fmt_buff, header_fmt) ;\
	strncpy(fmt_buff+sizeof(header_fmt)-1, fmt, sizeof(fmt_buff)-sizeof(header_fmt)+1);\
	struct tm t ;\
    time_t _t = time(NULL) ;\
	localtime_r(&_t, &t) ;\
	char date_buff[52] = {0} ;\
	strftime(date_buff, sizeof(date_buff)-1, "%Y-%m-%d %H-%M-%S", &t) ;\
	func(logger, fmt_buff, date_buff, level_name, __FILE__, __func__, __LINE__, ##__VA_ARGS__) ;\
	}while(0);

#define DEBUG(fmt, ...) __LOG(fmt, "DEBUG", debug, ##__VA_ARGS__)
#define INFO(fmt, ...)  __LOG(fmt, "INFO", info, ##__VA_ARGS__)
#define WARN(fmt, ...)  __LOG(fmt, "WARN", warn, ##__VA_ARGS__)
#define ERROR(fmt, ...) __LOG(fmt, "ERROR", error, ##__VA_ARGS__)

#ifdef __cplusplus
extern "C" {
#endif

extern struct Logger *install_logger(const char *level, const char *access_log_file, const char *error_log_file) ;

extern void shut_down_loggger() ;
#ifdef __cplusplus
}
#endif

#endif
