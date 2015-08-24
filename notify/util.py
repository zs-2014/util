#coding: utf-8
import os, sys
import uuid
import logging
import logging.handlers
import urllib2
import urllib
import types
import time
import constants
import traceback

log = logging.getLogger()

unicode_to_utf8 = lambda txt: txt.encode('utf-8') if isinstance(txt, unicode) else txt

class UrllibClient(object):
    max_read_size = len(constants.RSP_SUCCESS_CODE)
    def __init__(self, notify_id):
        self.notify_id  = notify_id

    def __post__(self, url, data, content_type=None, read_sz=None):
        if content_type is None:
            content_type = 'application/x-www-form-urlencode'
        try:
            t1 = int(time.time()*1000)
            req = urllib2.Request(url)
            req.add_header('Content-Type', content_type)
            req.add_data(data)
            rsp_obj = urllib2.urlopen(req)
            ret = rsp_obj.read(read_sz or UrllibClient.max_read_size)
            status_code = rsp_obj.getcode()
            return ret
        except Exception, e:
            ret = str(e)
            status_code='unknown'
            log.warn(traceback.format_exc())
            return 'fails' 
        finally:
            t2 = int(time.time()*1000)
            args = [self.notify_id, url, data, content_type, status_code, ret, t2-t1]
            log.info('notify_id=[%s]|url=[%s]|post_data=[%s]|content-type=[%s]|status_code=[%s]|ret=[%s]|difftm=[%d]', *args)

    def post_json(self, url, json_data, read_sz=None):
        if isinstance(json_data, dict):
            json_data = json.dumps(json_data) 
        return self.__post__(url, json_data, 'application/json; charset=UTF-8', read_sz)

def get_errno_from_exception(e):
    if hasattr(e, 'errno'):
        return e.errno
    elif e.args:
        return e.args[0]
    else:
        return None

def generate_notify_id():
    return uuid.uuid1().hex.upper()

def timeit(func):
    def _(*args, **kwargs):
        t1 = int(time.time()*1000)
        ret = None
        try:
            ret = func(*args, **kwargs)
        except Exception, e:
            log.warn(traceback.format_exc())
            ret = e
            raise
        finally:
            t2 = int(time.time()*1000)
            msg = []
            msg.append('func=%s' % func.__name__)
            msg.append('args=%s' % str(args[1:]))
            msg.append('kwargs=%s' % str(kwargs))
            msg.append('ret=%s' % (str(ret)[:2048], ))
            msg.append('%d' % t1)
            msg.append('%d' % t2)
            msg.append('%d' % (t2-t1, ))
            log.info('|'.join(msg))
        return ret
    return _

def install_logger(conf):
    msg_format = "%(asctime)s|%(process)d|%(threadName)s|%(filename)s|%(lineno)d|%(levelname)s|%(message)s" 
    formatter = logging.Formatter(msg_format)
    log = logging.getLogger()
    log.setLevel(logging.DEBUG)
    stm_map = {'STDOUT': sys.stdout,
               'STDERR': sys.stderr}
    if isinstance(conf, basestring):
        if conf.strip().upper() in ['STDOUT', 'STDERR']:
            conf = conf.strip().upper()
            handler = logging.StreamHandler(stm_map[conf]) 
        else:
            handler = logging.handlers.WatchedFileHandler(conf) 
        handler.setFormatter(formatter)
        log.addHandler(handler)
        return log
    elif isinstance(conf, dict):
        for l,f in conf.iteritems():
            level = logging.getLevelName(l.upper())
            if isinstance(level, basestring):
                raise ValueError('unknown logging level (%s)' % l)
            if f.strip().upper() in ['STDOUT', 'STDERR']:
                handler = logging.StreamHandler(stm_map[f.strip().upper()])
            else:
                handler = logging.handlers.WatchedFileHandler(f)
            handler.setFormatter(formatter)
            handler.setLevel(level)
            log.addHandler(handler)
        return log
    else:
        raise ArgumetError('%s not string nor dict' % str(conf))

if __name__ == '__main__':
    def test(conf):
        log = install_logger(conf)
        log.debug('debug')
        log.info('info')
        log.warn('warn')
        log.error('error')
    #test(' stdout ')
    #test('STDOUT')
    #test('stderr')
    #test('test.log')
    test({'INFO': 'test.info.log',
          'WARN': 'test.warn.log'})

