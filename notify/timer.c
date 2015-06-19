#include <time.h>
#include <event2/event.h>

void timer_call_back(evutil_socket_t fd, short event, void *args)
{
    printf("timer_call_back is triggered\n") ;
}


int main(int argc, char *argv[])
{
    struct event_base *base = event_base_new() ;
    struct event *e = evtimer_new(base, timer_call_back, NULL) ;
    struct timeval val ;
    val.tv_sec = 3 ; 
    val.tv_usec = 0 ;
    evtimer_add(e, &val) ;
    event_base_dispatch(base) ;
    event_base_free(base) ;
    event_del(e) ;
    event_free(e) ;
    printf("the event is end\n") ;
}
