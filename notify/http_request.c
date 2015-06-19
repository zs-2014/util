#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <event2/event.h>
#include <event2/http.h>
#include "notify_message.h"
#include "notify_server.h"
#include "time_span.h"
#include "log.h"

#define HTTP_DEFAULT_PORT 80
#define HTTPS_DEFAULT_PORT 443
//2G
#define UNKNOWN_BODY_SIZE 0x7fffffff
#define SUCCESS_CODE 200

#define SUCCESS_STR "SUCCESS"
#define FAIL_STR "FAIL"

//log-format |notify_id|notify_url|notify_data|notify_tmspan|respcd|resp-content|
#define LOG_MSG_FORMAT "|%s|%s|%s|%u|%d|%s|"

struct RequestContext
{
    struct NotifyMessage *msg ;
    struct evhttp_uri *evuri ;
    struct evhttp_connection *conn ;
    //timeout event
    struct event *e ;
};


static const char *send_http_request(struct event_base *base, struct RequestContext *ctx) ;

static void free_request_context(struct RequestContext *ctx)
{
    if(!ctx)
        return ;

    if(ctx ->msg)
        free_message(ctx ->msg) ;
    ctx ->msg = NULL ;

    if(ctx ->evuri)
        evhttp_uri_free(ctx ->evuri) ;
    ctx ->evuri = NULL ;

    if(ctx ->conn)
        evhttp_connection_free(ctx ->conn) ;
    ctx ->conn = NULL ;

    if(ctx ->e)
    {
        event_del(ctx ->e) ;
        event_free(ctx ->e) ;
    }
    ctx ->e = NULL ;
    free(ctx) ;
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

//时间一到,立马触发
static void timer_call_back(evutil_socket_t fd, short event, void *args)
{
    if(!(event & EV_TIMEOUT))
        return ;
    struct RequestContext *ctx = (struct RequestContext *)args ;
    if(ctx ->e)
    {
        evtimer_del(ctx ->e) ;
        event_free(ctx ->e) ;
        ctx ->e = NULL ;
    }
    send_http_request(get_server() ->base, ctx) ;
}

static void add_next_notify(struct RequestContext *ctx)
{
    struct NotifyMessage *msg = ctx ->msg ;
    struct Server *server = get_server() ;
    int tm = next_time_span(server ->time_span, msg ->next_timespan) ;
    //已经通知完了
    if(tm == -1)
    {
        free_request_context(ctx) ;
        return ;
    }
    struct event *e = evtimer_new(server ->base, timer_call_back, (void *)ctx) ;
    if(!e)
    {
        ERROR("fail to create timer for:|%s|%s|", msg ->notify_url, msg ->notify_data) ; 
        free_request_context(ctx) ;
        return ;
    }
    //释放已经存在的timer event
    if(ctx ->e)
    {
        event_del(ctx ->e) ;
        event_free(ctx ->e) ;
    }
    ctx ->e = e ;
    struct timeval val;
    val.tv_sec = tm/1000;
    val.tv_usec = (tm-val.tv_sec*1000)*1000 ;
    msg ->next_timespan++ ;
    evtimer_add(e, &val) ;
}

//log-format |notify_id|notify_url|notify_data|notify_tmspan|respcd|resp-content|
void request_callback(struct evhttp_request *request, void *args)
{
    struct RequestContext *ctx = (struct RequestContext *)args ;
    struct NotifyMessage *msg = ctx ->msg ;    
    char *data = copy_response_body(request) ;
    INFO(LOG_MSG_FORMAT, msg ->notify_id, msg ->notify_url, msg ->notify_data, msg ->next_timespan, evhttp_request_get_response_code(request), data?data:"") ;
    if(!request_is_ok(request) || !data)
    {
        add_next_notify(ctx) ;
        free(data) ;
        return ;
    }
    //log.info(data) ; 
    if(strcasecmp(data, SUCCESS_STR) != 0)
    {
        add_next_notify(ctx) ;
        free(data) ;
        return ;
    }
    //如果通知成功了的话，则不需要再通知了
    //delete the notify event from event list
    free(data) ;
    free_request_context(ctx) ;
}

inline static struct evhttp_request *new_request(struct RequestContext *ctx)
{
   return evhttp_request_new(request_callback, (void *)ctx)  ;
}

static const char *send_http_request(struct event_base *base, struct RequestContext *ctx)
{
    if(!ctx)
        return NULL ;
    struct NotifyMessage *msg = ctx ->msg ; 
    struct evhttp_uri *evuri = ctx ->evuri ; //evhttp_uri_parse(msg ->notify_url) ;
    if(!evuri)
    {
        evuri = evhttp_uri_parse(msg ->notify_url) ;
        ctx ->evuri = evuri ;
    }
    if(!evuri)
    {
        WARN("invalid notify_url:%s", msg ->notify_url) ;
        free_message(msg) ;
        evhttp_uri_free(evuri) ;
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
            ERROR("not supported protocol:%s notify_url:[%s]notify_data:[%s]", schema, msg ->notify_url, msg ->notify_data) ;
            free_message(msg) ;
            return NULL ;
        }
    }
    struct evhttp_request *req = new_request(ctx) ;
    struct evhttp_connection *conn = evhttp_connection_base_new(base, NULL, host, port) ; 
    if(ctx ->conn)
        evhttp_connection_free(ctx ->conn) ;
    ctx ->conn = conn ;
    struct evbuffer *buffer = evhttp_request_get_output_buffer(req) ;
    evbuffer_add(buffer, msg ->notify_data, msg ->notify_data_len) ;
    struct evkeyvalq *header = evhttp_request_get_output_headers(req) ;
    evhttp_add_header(header, "Content-Type", "application/json; charset=UTF-8") ;
    char uri[2048] = {0} ;
    const char *path = evhttp_uri_get_path(evuri) ;
    const char *query = evhttp_uri_get_query(evuri) ;
    snprintf(uri, sizeof(uri)-1, "%s?%s", path ? path : "/", query ? query : "") ;
    evhttp_make_request(conn, req, EVHTTP_REQ_POST, uri) ;
    return msg ->notify_id ;
}

const char *make_http_request(struct event_base *base, const char *json_string)
{
    struct RequestContext *ctx = (struct RequestContext *)calloc(1, sizeof(struct RequestContext)) ;
    if(!ctx)
        return NULL ;
    ctx ->msg = make_message(json_string) ; 
    ctx ->conn = NULL ;
    ctx ->evuri = NULL ;
    ctx ->e = NULL ;
    return send_http_request(base, ctx) ;
}

#ifdef HTTP_REQUEST
int main(int argc, char *argv[])
{
    run();
    return 0 ;
}

#endif

