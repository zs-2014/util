#ifndef __time_span__h
#define __time_span__h

struct TimeSpan
{
	int *data ;
	unsigned total_sz;
	int curr_sz ;
};

#ifdef __cplusplus
extern "C" {
#endif

extern struct TimeSpan *new_time_span(const char *time_span_str) ;
extern void free_time_span(struct TimeSpan *time_span) ;
extern int next_time_span(struct TimeSpan *time_span, int idx) ;

#ifdef __cplusplus
}
#endif

#endif
