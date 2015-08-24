#ifndef __notify__server__h
#define __notify__server__h

struct Config ;
struct event_base ;
struct TimeSpan ;
struct Server
{
	struct Config *config ;
    struct event_base *base ;
    struct TimeSpan *time_span ;
} ;

#ifdef __cplusplus
extern "C" {
#endif

extern struct Server *get_server() ;

#ifdef __cplusplus
}
#endif

#endif
