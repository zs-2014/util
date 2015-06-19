#ifndef __http__request__h
#define __http__request__h

#ifdef __cplusplus
extern "C" {
#endif


extern char *make_http_request(struct event_base *base, const char *json_string) ;

#ifdef __cplusplus
}
#endif

#endif
