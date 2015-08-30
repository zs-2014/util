#coding: utf-8
from dns_protocol import DNSResp, DNSReq
from tornado.ioloop import IOLoop
from tornado.netutil import Configurable
from tornado.gen import coroutine, Future, Return
import sys

import functools
import socket

unicode_to_utf8 = lambda t: t.encode('utf-8') if isinstance(t, unicode) else t

class DNSError(Exception):
    def __init__(self, host_name, server, port):
        self.host_name = unicode_to_utf8(host_name)
        self.server = unicode_to_utf8(server)
        self.port = unicode_to_utf8(port)

    def __str__(self):
        return 'unresolved the host[%s] at [%s:%s]' % (self.host_name, self.server, self.port)

    def __repr__(self):
        return str(self)

class TornadoDNSResolver(Configurable):
    
    @classmethod
    def configurable_base(cls):
        return TornadoDNSResolver

    @classmethod
    def configurable_default(cls):
        return TornadoDNSResolver

    def initialize(self, server, port=53, ioloop=None):
        self.server = server
        self.port = port
        self.ioloop = ioloop or IOLoop.current()

    def read(self, future, tmout_handler, host_name, sock, events):
        try:
            if tmout_handler is not None:
                self.ioloop.remove_timeout(tmout_handler)
            self.ioloop.remove_handler(sock)  
            records = DNSResp(sock.recvfrom(1024)[0]).parse_response()
            names = [r.get_data() for r in filter(lambda x: x.is_CNAME_record(), records)]
            names.append(host_name) 
            records = filter(lambda r: r.name in names, records)
            future.set_result([r for r in filter(lambda r: r.is_A_record(), records)])
        except Exception, e:
            future.set_exc_info(sys.exc_info())
        finally:
            sock.close()

    def timeout_callback(self, future, sock, timeout):
        try:
            if not future.done():
                return
            self.ioloop.remove_handler(sock) 
            sock.close()
            raise socket.timeout(timeout)
        except Exception, e:
            future.set_exc_info(sys.exc_info())

    def getaddrinfo(self, host_name, timeout=None) :
        result = Future() 
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        packet = DNSReq.make_query_packet(host_name, tc_flag=1, rd_flag=1)
        sock.sendto(packet, (self.server, self.port))
        timeout_handler = None
        if timeout is not None:
            timeout_handler = self.ioloop.add_timeout(self.ioloop.time()+timeout, self.timeout_callback, result, sock, timeout)
        self.ioloop.add_handler(sock, functools.partial(self.read, result, timeout_handler, host_name), IOLoop.READ)
        return result
    
    @coroutine
    def resolve(self, host, port, family=socket.AF_UNSPEC, callback=None):
        records = yield self.getaddrinfo(host) 
        addrs = [r.get_ip() for r in records]
        raise Return([(socket.AF_INET, (addr, port)) for addr in addrs])

class TornadoDNSCacheResolver(TornadoDNSResolver):

    def initialize(self, server, port, tmout_per_domain=1.5, max_try_cnt=2, ioloop=None):
        self.dns_cache_dict = {}
        self.max_try_cnt = max_try_cnt
        self.tmout_per_domain = tmout_per_domain
        super(TornadoDNSCacheResolver, self).initialize(server, port, ioloop)

    @classmethod
    def configure_default(cls):
        return TornadoDNSCacheResolver

    @coroutine
    def get(self, host_name):
        records = self.dns_cache_dict.get(host_name, []) 
        ava_records = []
        for r in records:
            if not r.is_expired():
                ava_records.append(r)
        if not ava_records:
            for _ in range(0, self.max_try_cnt):
                try:
                    ava_records = yield  self.getaddrinfo(host_name, self.tmout_per_domain)
                except socket.timeout, te:
                    pass
                else:
                    break
            if not ava_records:
                raise DNSError(host_name, self.server, self.port) 
            self.dns_cache_dict[host_name] = ava_records
        raise Return(ava_records)

    @coroutine
    def resolve(self, host, port, family=socket.AF_UNSPEC, callback=None):
        if family == socket.AF_UNSPEC:
            family = socket.AF_INET
        records = yield self.get(host)
        addrs = [(family, (r.get_ip(), port)) for r in records]
        raise Return(addrs)

def test():
    from tornado.web import RequestHandler, Application
    from tornado.httpclient import AsyncHTTPClient 
    import logging
    log = logging.getLogger()
    log.setLevel(logging.DEBUG)
    log.addHandler(logging.StreamHandler(sys.stdout))
    class Ping(RequestHandler):
        @coroutine
        def get(self):
            cli = AsyncHTTPClient(resolver=TornadoDNSCacheResolver(server='114.114.114.114', port=53))
            rsp = yield cli.fetch('http://www.baidu.com/')
            self.write(rsp.body)
    app = Application([('/ping', Ping)])
    app.listen(12345)
    IOLoop.current().start()

if __name__  == '__main__':
    test()
