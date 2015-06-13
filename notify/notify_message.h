#ifndef __notify_message__h
#define __notify_message__h

struct NotifyMessage
{
    char *notify_url ;
    void *notify_data ;
	char notify_id[33] ;
    unsigned notify_data_len ;
    int next_timespan ;
    long long int create_tm ;
    long long int update_tm ;
};


extern void free_message(struct NotifyMessage *msg) ;
extern struct NotifyMessage *make_message(const char *json_string) ;
extern struct NotifyMessage *new_message(const char *notify_url, const char *notify_data) ; 
#endif
