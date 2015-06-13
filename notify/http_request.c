#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/http.h>
#include "notify_message.h"

#define HTTP_DEFAULT_PORT 80
#define HTTPS_DEFAULT_PORT 443
//2G
#define UNKNOWN_BODY_SIZE 0x7fffffff
#define SUCCESS_CODE 200

#define SUCCESS_STR "SUCCESS"
#define FAIL_STR "FAIL"


static char *to_upper(char *str)
{
    char *tmp = str ;
    while(*str != '\0')
    {
        //'a' - 'A' == 32
        if(*str >= 'a' && *str <= 'z')
            *str = *str - 32  ;
        str++ ;
    }
    return tmp ;
}


static const char *get_value_from_header(struct evhttp_request *request, const char *key)
{
    if(!request || !key)
        return NULL ;

    struct evkeyvalq *input_header = evhttp_request_get_input_headers(request) ;
    return evhttp_find_header(input_header, key) ;
}

static int64_t get_content_length(struct evhttp_request *request)
{
    const char *content_length = get_value_from_header(request, "Content-Length") ;
    if(!content_length)
        return UNKNOWN_BODY_SIZE ;

    return strtoll(content_length, NULL, 10) ;
}

static int request_is_ok(struct evhttp_request *request)
{
    return request && evhttp_request_get_response_code(request) == SUCCESS_CODE ;
}

static char *copy_response_body(struct evhttp_request *request)
{
    if(!request) 
        return NULL ;
    int64_t content_length = get_content_length(request)  ;
    if(content_length == UNKNOWN_BODY_SIZE)
        return NULL ;

    struct evbuffer *input_buffer = evhttp_request_get_input_buffer(request) ;
    if(!input_buffer)
        return NULL ;

    char *data = (char *)calloc(1, (content_length + 1) * sizeof(char)) ;
    if(!data)
        return NULL ;

    evbuffer_copyout(input_buffer, data, content_length)  ;
    return data ;
}

void request_callback(struct evhttp_request *request, void *args)
{
    if(!request_is_ok(request))
    {
       //request失败了 
       //log it
    }
    char *data = copy_response_body(request) ;
    if(!data)
    {
        //no response
        //log the event
    }
    //log.info(data) ; 
    if(strcmp(to_upper(data), SUCCESS_STR) != 0)
    {
        //not success
    }
    //delete the notify event from event list
    free(data) ;
}

inline static struct evhttp_request *new_request(struct NotifyMessage *msg)
{
   return evhttp_request_new(request_callback, (void *)msg)  ;
}

const char *make_http_request(struct event_base *base, const char *json_string)
{
    struct NotifyMessage *msg = make_message(json_string) ;
    if(!msg)
        return NULL ;

    struct evhttp_uri *evuri = evhttp_uri_parse(msg ->notify_url) ;
    if(!evuri)
    {
        free_message(msg) ;
        return NULL ;
    }
    const char *host = evhttp_uri_get_host(evuri) ;
    const char *schema = evhttp_uri_get_scheme(evuri) ;
    int port = evhttp_uri_get_port(evuri) ;
    if(port == -1)
    {
        if(!schema || strcasecmp("http", schema) == 0)
            port = HTTP_DEFAULT_PORT ;
        else if (strcasecmp("https", schema) == 0)
            port = HTTPS_DEFAULT_PORT;
        else
        {
            printf("[DEBUG] not supported protocol:%s\n", schema) ;
            evhttp_uri_free(evuri) ;
            free_message(msg) ;
            return NULL ;
        }
    }
    struct evhttp_request *req = new_request(msg) ;
    struct evhttp_connection *conn = evhttp_connection_base_new(base, NULL, host, port) ; 
    evhttp_request_own(req) ;
    struct evbuffer *buffer = evhttp_request_get_output_buffer(req) ;
    evbuffer_add(buffer, msg ->notify_data, msg ->notify_data_len) ;
    struct evkeyvalq *header = evhttp_request_get_output_headers(req) ;
    evhttp_add_header(header, "Content-Type", "application/json; charset=UTF-8") ;
    char uri[2048] = {0} ;
    const char *path = evhttp_uri_get_path(evuri) ;
    const char *query = evhttp_uri_get_query(evuri) ;
    snprintf(uri, sizeof(uri)-1, "%s?%s", path?path:"/", query?query:"") ;
    evhttp_make_request(conn, req, EVHTTP_REQ_POST, uri) ;
    return msg ->notify_id ;
    //event_base_dispatch(base) ;
    //event_base_free(base) ;
    //evhttp_connection_free(conn) ;
}

#ifdef HTTP_REQUEST
int main(int argc, char *argv[])
{
    run();
    return 0 ;
}

#endif

