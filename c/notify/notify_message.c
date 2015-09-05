#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include "notify_message.h"
#include "json.h"
#include "log.h"


struct NotifyMessage *new_message(const char *notify_url, const char *notify_data)
{
    size_t notify_url_len = strlen(notify_url) ;
    size_t notify_data_len = strlen(notify_data) ;
    struct NotifyMessage *msg = (struct NotifyMessage *)calloc(1, sizeof(struct NotifyMessage)+notify_url_len+notify_data_len+2) ;
    if(!msg)
        return NULL ;
    msg ->create_tm = time(NULL) ;
    msg ->update_tm = time(NULL) ;
    
    struct timeval val ;
    gettimeofday(&val, NULL) ;
    snprintf(msg ->notify_id, sizeof(msg ->notify_id)-1, "%020lu%010u%02u", val.tv_sec*1000000+val.tv_usec, getpid()&0xffffffff, rand()%100) ;
    msg ->notify_url = ((char *)msg) + sizeof(struct NotifyMessage) ;
    strcpy(msg ->notify_url, notify_url) ;
    msg ->notify_url[notify_url_len] = '\0' ;
    msg ->notify_data = msg ->notify_url + notify_url_len + 1;
    msg ->notify_data_len = notify_data_len ;
    strcpy(msg ->notify_data, notify_data) ;
    return msg ;
}

/*
 * {
 *     "notify_url": "http://www.baidu.com",
 *     "notify_data": "xxxxxxx"
 * }
 */

struct NotifyMessage *make_message(const char *json_string)
{
    struct NotifyMessage *msg = NULL ;
     //json loads
    json_object *object = json_tokener_parse(json_string) ;
    if(!object)
        return NULL ;
    json_object *notify_data_obj = NULL ;
    json_object *notify_url_obj = json_object_object_get(object, "notify_url") ;
    if(json_object_get_type(notify_url_obj) != json_type_string)
    {
        WARN("notify_url is invalid string:%s", json_string) ;
        goto __end ;
    }
    const char *notify_url = json_object_get_string(notify_url_obj) ;

    notify_data_obj = json_object_object_get(object, "notify_data") ;
    if(json_object_get_type(notify_data_obj) != json_type_string)
    {
        WARN(" notify_data is invalid string:[%s]", json_string) ;
        goto __end ;
    }
    const char *notify_data = json_object_get_string(notify_data_obj) ; 
    msg = new_message(notify_url, notify_data) ; 

__end:
    json_object_put(notify_data_obj) ;
    json_object_put(notify_url_obj) ;
    json_object_put(object) ;
    return msg;
}

void free_message(struct NotifyMessage *msg)
{
   free(msg) ; 
}

