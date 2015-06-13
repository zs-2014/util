#include <event2/event.h>
#include <event2/buffer.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "notify_server.h"
#include "string_util.h"
#include "config.h"
#include "json.h"

#define DEFAULT_PORT "6666"

#define DEFAULT_BACKLOG "128"

#define max(a, b) (a)>(b)?(a):(b)
#define min(a, b) (a)>(b)?(b):(a)

struct Server *g_server = NULL ;

struct ReadArgs
{
    struct evbuffer *evbuff ;
    struct event *e ;
}; 

struct WriteArgs
{
};

static struct Server *new_server()
{
    if(!g_server)
    { 
        g_server = (struct Server *)calloc(1, sizeof(struct Server)) ;
    }
    return g_server ;
}

static evutil_socket_t create_server_socket(const char *host, int port)
{
    if(!host)
        return -1 ;
    char buff[1024] = {0} ;
    strcpy(buff, host) ;
    strip(buff, ' ') ;
    size_t l = strlen(buff) ;
    buff[l++] = ':' ;
    //ip:host
    itostr(port, buff+l) ; 
    struct sockaddr_storage sock_addr ;      
    int sock_len = sizeof(sock_addr) ;
    if(evutil_parse_sockaddr_port(buff, (struct sockaddr *)&sock_addr, &sock_len) != 0)
    {
        printf("%s is not a valid socket address\n", buff) ;
        return -1 ;
    }
    evutil_socket_t sock_fd = socket(sock_addr.ss_family, SOCK_STREAM, 0) ;
    if(sock_fd < 0)
    {
        printf("fail to create socket:%s\n", strerror(errno)) ;
        return -1 ;
    }
    if(bind(sock_fd, (struct sockaddr *)&sock_addr, sock_len) < 0)
    {
        printf("fail to bind socket to %s\n errmsg:%s", buff, strerror(errno)) ;
        close(sock_fd) ;
        return -1 ;
    }
    return sock_fd ;
}

//|content-length#content|
void do_read(evutil_socket_t fd, short events, void *args)
{
    struct Server *server = new_server() ;
    struct ReadArgs *read_args = (struct ReadArgs *)args ;
    if(events & EV_TIMEOUT)
    {
        //timeout则关闭
        event_del(read_args ->e) ;
        evbuffer_free(read_args ->evbuff) ;
        return ;
    }
    struct evbuffer *evbuff = read_args ->evbuff ; 
    //read at most
    evbuffer_read(evbuff, fd, -1) ;
    struct evbuffer_ptr evbuff_ptr = evbuffer_search(evbuff, "#", 1, NULL) ;
    if(evbuff_ptr.pos == -1)
        return ;
    char buff[128] = {0} ;
    evbuffer_copyout(evbuff, buff, min(evbuff_ptr.pos, sizeof(buff)-1)) ;
    size_t content_length = atoi(buff) ; 
    //+1 '#'
    evbuffer_drain(evbuff, evbuff_ptr.pos+1) ;
    size_t buff_len = evbuffer_get_length(evbuff) ;
    //数据还没有读完
    if(content_length > buff_len)
    {
        //extra '\0'
        evbuffer_expand(evbuff, content_length+1) ;
        return ;
    }
    evbuffer_add(evbuff, "\0", 1) ;
    const char *notify_id = make_http_request(server ->base, (const char *)evbuffer_pullup(evbuff)) ;
    printf("notify_id:%s\n", notify_id) ;
    //write notify_id
    event_del(read_args ->e) ;
    evbuffer_free(read_args ->evbuff) ;
}

void do_write(evutil_socket_t fd, short events, void *args)
{
}

void do_accept(evutil_socket_t lsnfd, short events, void *args)
{ 
    struct sockaddr_storage cli_addr ;
    socklen_t sock_len = sizeof(cli_addr) ;
    int cli_fd = accept(lsnfd, (struct sockaddr *)& cli_addr, &sock_len) ;
    int err = errno ;
    if(cli_fd < 0)
    {
        printf("DEBUG fail to accept: %s\n", strerror(err)) ;
        return ;
    }
    struct Server *server = new_server() ;
    struct ReadArgs *read_args = (struct ReadArgs *)calloc(1, sizeof(struct ReadArgs)) ;
    struct evbuffer *evbuff = evbuffer_new() ;
    read_args ->evbuff = evbuff ;
    struct event *e = event_new(server ->base, cli_fd, EV_PERSIST|EV_READ, do_read, (void *)read_args) ;
    read_args ->e = e ;
    struct timeval val ;
    const char *timeout = get_value(server ->config, NULL, "timeout") ;
    int tm = 0 ;
    if(timeout)
        tm = atoi(timeout);
    val.tv_sec = tm/1000 ;
    val.tv_usec = (tm-tm/1000)*1000 ;
    event_add(e, timeout ? &val : NULL) ;
}

void run(const char *cfg_file)
{
    if(!cfg_file)
        return ; 
    struct Server *server = new_server() ;
    struct Config *config = read_config(cfg_file) ;
    if(!config)
        return ;
    const char *host = get_value(config, NULL, "host") ;
    const char *port = get_value(config, NULL, "port") ;
    if(!port)
        port = DEFAULT_PORT ;
    evutil_socket_t sock_fd = create_server_socket(host, atoi(port)) ;
    if(sock_fd == -1)
        return ;
    struct event_base *base = event_base_new() ;
    if(!base)
    {
        printf("fail to call event_base_new()\n") ;
        return ;
    }
    evutil_make_socket_nonblocking(sock_fd) ;
    evutil_make_listen_socket_reuseable(sock_fd) ;
    const char *backlog = get_value(config, NULL, "backlog") ;
    if(!backlog)
        backlog = DEFAULT_BACKLOG ;
    if(listen(sock_fd, atoi(backlog)) != 0)
    {
        printf("fail to listen at %s:%s with backlog:%s\n", host, port, backlog) ;
        return ;
    }
    accept(sock_fd, NULL, NULL) ;
    struct event *e = event_new(base, sock_fd, EV_PERSIST|EV_READ, do_accept, (void *)base) ;
    event_add(e, NULL) ;
    event_base_dispatch(base) ;  
    event_base_free(base) ;
}

int main(int argc, char *argv[])
{
    run(argc == 2 ? argv[1]: "config.ini") ;
    return 0 ;
}
