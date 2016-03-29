#coding: utf-8

import os, sys
import logging
import util
import time
import traceback
import constants
import config
import multiprocessing
import heapq

import redis 
from gevent.pool import Pool
from gevent.queue import Empty


from gevent import pool
from msgpack import dumps, loads
from msgpack.exceptions import ExtraData as msgpack_ExtraData_Exception
from util import UrllibClient


log = logging.getLogger()

class NotifyHandler(object):
    def __init__(self, queue, notify_threads):
        #消息通知的队列
        self.queue = queue
        self.pool = Pool(notify_threads)

    def notify_immediately(self, notify_id, notify_data):
        try:
            notify_dict = loads(notify_data)
        except Exception, e:
            log.warn(traceback.format_exc())
            return False

        notify_url = notify_dict.get('url') 
        post_data = notify_dict.get('data')
        if not notify_url or not post_data:
            log.info('notify_id=[%s]|notify_url=[%s]|post_data=[%s]|errmsg=invalid notify|', notify_id, notify_url, post_data)
            return False

        rsp = util.UrllibClient(notify_id).post_json(notify_url, post_data)
        if rsp == constants.RSP_SUCCESS_CODE:
            return True 
        #如果第一次通知不成功的话添加到待通知队列中去
        try:
            msg = dumps({'ct': int(time.time()*1000),
                         'id': notify_id,
                         'data': notify_data})
            self.queue.put(msg)
            return True 
        except Exception, e:
            args = [notify_data, notify_id, self.queue.qsize()]
            log.info('fail to put the notify_data to queue:|notify_data=[%s]|notify_id=[%s]|queue_len=[%d]', *args)
            return False
        
    #发送异步通知
    def send_notify(self, notify_data):
        notify_id = util.generate_notify_id()
        self.pool.spawn(self.notify_immediately, notify_id, notify_data)
        return notify_id
        

    def ping(self):
        return 'OK'

class Msg(object):
    def __init__(self, notify_id, cnt, next_notify_tm):
        self.notify_id = notify_id
        self.cnt = cnt
        self.next_notify_tm = next_notify_tm 
        self.is_finished = False

    def __cmp__(self, msg_obj):
        if self.next_notify_tm == msg_obj.next_notify_tm:
            return 0
        elif self.next_notify_tm < msg_obj.next_notify_tm:
            return -1
        else:
            return 1

    def __str__(self):
        return '[notify_id=%s cnt=%d next_notify_tm=%d]' % (self.notify_id, self.cnt, self.next_notify_tm/1000)

    def __repr__(self):
        return str(self)

class TimeSpan(object):
    def __init__(self, tm_spans):
        self.time_spans = [t*1000 for t in tm_spans]
    def notify_is_over(self, cnt):
        return len(self.time_spans) < cnt

    def next_notify_time(self, cnt):
        return  self.time_spans[min(cnt, len(self.time_spans)-1)] + int(time.time()*1000) 

    def get_next_cnt(self, create_tm):
        tm = int(time.time()*1000) 
        diff_tm = tm - create_tm
        if diff_tm <= 0:
            return 0
        for i in range(len(self.time_spans)):
            diff_tm -= self.time_spans[i]
            if diff_tm <= 0:
                return i 
        return -1

class Store(object):
    __hash_name__ = '__msg_hash__'
    def __init__(self, redis_server):
        self.redis_server = redis_server
        self.redis = redis.Redis(host=redis_server['host'], port=redis_server['port'], password=redis_server['password'])

    @util.timeit
    def save_notify_data(self, notify_id, notify_data):
        try:
            #放到通知队列中去
            self.redis.hset(self.__hash_name__, notify_id, dumps({'ct': int(time.time()*1000), 'data': notify_data}))
        except Exception, e:
            log.warn(traceback.format_exc())

    @util.timeit
    def get_notify_data(self, notify_id):
        try:
            ret = self.redis.hget(self.__hash_name__, notify_id)
            if ret is not None:
                _tmp = loads(ret) 
                log.debug('ct=%d|difftm=%d', _tmp['ct'], int(time.time()*1000)-_tmp['ct'])
                return loads(ret)['data']
        except Exception, e:
            log.warn(traceback.format_exc())

    @util.timeit
    def delete_notify_data(self, notify_id):
        try:
            self.redis.hdel(self.__hash_name__, notify_id) 
        except Exception, e:
            log.warn('fail to delete notify data:notify_id=%s', notify_id)
            log.warn(traceback.format_exc())

    @util.timeit
    def get_all(self):
        try:
            return self.redis.hgetall(self.__hash_name__)
        except Exception, e:
            log.warn(traceback.format_exc())

class Notify(object):
    def __init__(self, queue, threads, is_load_from_store):
        self.queue = queue
        self.pool = pool.Pool(threads)
        self.msg_lst = []
        self.loop = True
        self.time_span = TimeSpan(config.time_span)
        self.store = Store(config.redis_server)
        self.is_load = is_load_from_store

    def stop(self):
        log.debug('start stop the notify .........')
        while True:
            try:
                msg = loads(self.queue.get_nowait())
            except Empty:
                break
            except Exception, e:
                log.warn(traceback.format_exc())
            self.save(msg, False)
        self.loop = False
        self.queue.put_nowait('wake_up')
        log.debug('end stop the notify !!!!')

    def save(self, msg, is_push=True):
        notify_id = msg['id']
        if is_push:
            log.debug('before save_notify_data: self.msg_lst=%s', str(self.msg_lst))
            heapq.heappush(self.msg_lst, Msg(notify_id, 1, self.time_span.next_notify_time(0)))
            log.debug('after save_notify_data: self.msg_lst=%s', str(self.msg_lst))
        self.store.save_notify_data(notify_id, msg['data'])
        return True

    def get_ready_list(self):
        ready_lst = []
        if not self.msg_lst:
            return ready_lst
        tn = int(time.time()*1000)
        #查找到通知时间的
        while self.msg_lst and self.msg_lst[0].next_notify_tm <= tn:
            ready_lst.append(heapq.heappop(self.msg_lst))
        log.debug('ready_lst=%s self.msg_lst=%s tn=%d', ready_lst, self.msg_lst, tn)
        return ready_lst

    def _do_notify(self, notify_id, msg):
        notify_data = self.store.get_notify_data(notify_id) 
        log.debug('notify_data: %s', notify_data)
        #可能被删掉了 也可能是获取的时候抛出异常了
        if notify_data is None:
            return
        notify_dict = loads(notify_data)  
        rsp = util.UrllibClient(notify_id).post_json(notify_dict['url'], notify_dict['data'])
        #通知成功删除掉
        if rsp == constants.RSP_SUCCESS_CODE:
            self.store.delete_notify_data(notify_id)
            return
        #还没结束添加到下次通知的队列中去
        if not self.time_span.notify_is_over(msg.cnt):
            msg.next_notify_tm = self.time_span.next_notify_time(msg.cnt) 
            msg.cnt += 1
            heapq.heappush(self.msg_lst, msg)
            #唤醒queue
            try:
                self.queue.put_nowait('wake_up')
            except Exception, e:
                pass

    def handle_ready_list(self):
        ready_lst = self.get_ready_list()
        if not ready_lst:
            return
        #notify_ids = []
        for msg in ready_lst:
            #通知次数未满且通知一直没有成功
            if not self.time_span.notify_is_over(msg.cnt): 
                self.pool.spawn(self._do_notify, msg.notify_id, msg)
            #通知次数满了
            elif self.time_span.notify_is_over(msg.cnt):
                self.store.delete_notify_data(msg.notify_id)

    def load_from_store(self):
        st_dict = self.store.get_all() 
        if not st_dict:
            return
        for k, v in st_dict.iteritems():
            v_dict = loads(v) 
            cnt = self.time_span.get_next_cnt(v_dict['ct'])
            if cnt < 0:
                self.store.delete_notify_data(k)  
                continue
            heapq.heappush(self.msg_lst, Msg(k, cnt+1, self.time_span.next_notify_time(cnt)))

    def do_notify(self):
        if self.is_load:
            self.load_from_store()
        while self.loop:
            try:
                tout = None
                if self.msg_lst:
                    tout = (self.msg_lst[0].next_notify_tm-int(time.time()*1000))/1000.0
                    log.debug('message %s, now=%d', str(self.msg_lst), int(time.time()))
                try:
                    log.debug('tout: %s', str(tout))
                    msg = loads(self.queue.get(timeout=tout))
                except (Empty, msgpack_ExtraData_Exception):
                    pass
                except Exception, e:
                    log.warn(traceback.format_exc())
                    sys.exit(-1)
                else:
                    self.save(msg)
                self.handle_ready_list()
            except Exception, e:
                log.warn(traceback.format_exc())
        log.debug('the process(%d) is terminated', os.getpid())
        sys.exit(0)
