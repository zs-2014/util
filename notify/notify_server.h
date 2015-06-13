#ifndef __notify__server__h
#define __notify__server__h

struct Config ;
struct event_base ;
struct Server
{
	struct Config *config ;
    struct event_base *base ;
} ;

extern struct Server *g_server ;
#endif
