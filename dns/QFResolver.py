#coding: utf-8

import traceback
import socket
import time
import collections
import struct
import os
import functools
import tornado
import logging
from tornado.netutil import Resolver
import config

log = logging.getLogger()

QTYPE_ANY = 255
QTYPE_A = 1
QTYPE_AAAA = 28
QTYPE_CNAME = 5
QTYPE_NS = 2
QCLASS_IN = 1

class DNSResponse(object):
    def __init__(self):
        self.hostname = None
        self.questions = []  # each: (addr, type, class)
        self.answers = []  # each: (addr, type, class)

    def __str__(self):
        return '%s: %s' % (self.hostname, str(self.answers))

def _parse_resolv():
    _servers = []
    try:
        with open('/etc/resolv.conf', 'rb') as f:
            content = f.readlines()
            for line in content:
                line = line.strip()
                if line:
                    if line.startswith(b'nameserver'):
                        parts = line.split()
                        if len(parts) >= 2:
                            server = parts[1]
                            _servers.append(server)
    except IOError:
        pass
    if not _servers:
        _servers = ['8.8.4.4', '8.8.8.8']
    return _servers

def build_address(address):
    log.debug('build address')
    address = address.strip(b'.')
    labels = address.split(b'.')
    results = []
    for label in labels:
        l = len(label)
        if l > 63:
            return None
        results.append(chr(l))
        results.append(label)
    results.append(b'\0')
    return b''.join(results)

def build_request(address, qtype):
    header = struct.pack('!BBHHHH', 1, 0, 1, 0, 0, 0)
    addr = build_address(address)
    qtype_qclass = struct.pack('!HH', qtype, QCLASS_IN)
    return str(header) + str(addr) + str(qtype_qclass)


def parse_ip(addrtype, data, length, offset):
    if addrtype == QTYPE_A:
        return socket.inet_ntop(socket.AF_INET, data[offset:offset + length])
    elif addrtype == QTYPE_AAAA:
        return socket.inet_ntop(socket.AF_INET6, data[offset:offset + length])
    elif addrtype in [QTYPE_CNAME, QTYPE_NS]:
        return parse_name(data, offset)[1]
    else:
        return data[offset:offset + length]

def parse_name(data, offset):
    p = offset
    labels = []
    l = ord(data[p])
    while l > 0:
        if (l & (128 + 64)) == (128 + 64):
            # pointer
            pointer = struct.unpack('!H', data[p:p + 2])[0]
            pointer &= 0x3FFF
            r = parse_name(data, pointer)
            labels.append(r[1])
            p += 2
            # pointer is the end
            return p - offset, b'.'.join(labels)
        else:
            labels.append(data[p + 1:p + 1 + l])
            p += 1 + l
        l = ord(data[p])
    return p - offset + 1, b'.'.join(labels)

def parse_record(data, offset, question=False):
    nlen, name = parse_name(data, offset)
    if not question:
        record_type, record_class, record_ttl, record_rdlength = struct.unpack(
            '!HHiH', data[offset + nlen:offset + nlen + 10]
        )
        ip = parse_ip(record_type, data, record_rdlength, offset + nlen + 10)
        return nlen + 10 + record_rdlength, \
            (name, ip, record_type, record_class, record_ttl)
    else:
        record_type, record_class = struct.unpack(
            '!HH', data[offset + nlen:offset + nlen + 4]
        )
        return nlen + 4, (name, None, record_type, record_class, None, None)

def parse_header(data):
    if len(data) >= 12:
        header = struct.unpack('!HBBHHHH', data[:12])
        res_id = header[0]
        res_qr = header[1] & 128
        res_tc = header[1] & 2
        res_ra = header[2] & 128
        res_rcode = header[2] & 15
        res_qdcount = header[3]
        res_ancount = header[4]
        res_nscount = header[5]
        res_arcount = header[6]
        return (res_id, res_qr, res_tc, res_ra, res_rcode, res_qdcount,
                res_ancount, res_nscount, res_arcount)
    return None

def parse_response(data):
    try:
        if len(data) >= 12:
            header = parse_header(data)
            if not header:
                return None
            res_id, res_qr, res_tc, res_ra, res_rcode, res_qdcount, \
                res_ancount, res_nscount, res_arcount = header

            qds = []
            ans = []
            offset = 12
            for i in range(0, res_qdcount):
                l, r = parse_record(data, offset, True)
                offset += l
                if r:
                    qds.append(r)
            for i in range(0, res_ancount):
                l, r = parse_record(data, offset)
                offset += l
                if r:
                    ans.append(r)
            for i in range(0, res_nscount):
                l, r = parse_record(data, offset)
                offset += l
            for i in range(0, res_arcount):
                l, r = parse_record(data, offset)
                offset += l
            response = DNSResponse()
            if qds:
                response.hostname = qds[0][0]
            for an in qds:
                response.questions.append((an[1], an[2], an[3]))
            for an in ans:
                response.answers.append((an[1], an[2], an[3]))
            return response
    except:
        print traceback.format_exc()
        return None

class QFResolver(Resolver):
    ''' cache : host+port : ([(host, port)], add_time) '''
    CACHE = {} # key: 'host:port' value: [(family, addr)...]
    SOCK = None
    CALLBACK = {} # key: requestid value: callback
    MAPPING = {} # key: requestid value: 'host:port'
    
    def _get_resolve(self):
        _servers = []
        if getattr(config, 'resolver_server', '') and config.resolver_server:
            self._servers = config.resolver_server
            return
        try:
            with open('/etc/resolv.conf', 'rb') as f:
                content = f.readlines()
                for line in content:
                    line = line.strip()
                    if line:
                        if line.startswith(b'nameserver'):
                            parts = line.split()
                            if len(parts) >= 2:
                                server = parts[1]
                                _servers.append(server)
        except IOError:
            pass
        if not _servers:
            _servers = ['114.114.114.114']
        self._servers = _servers
        return _servers

    def _init_sock(self):
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        for svr in self._servers:
            try:
                sock.connect((svr, 53))
            except:
                continue 
            else:
                QFResolver.SOCK = sock
                break
        else:
            QFResolver.sock = None
            log.warn("all resolver host unreachable")
            
            
    def initialize(self, io_loop=None):
        self._get_resolve()
        self._init_sock()
        if io_loop:
            self.io_loop = io_loop
        else:
            self.io_loop = tornado.ioloop.IOloop.instance()
        # add sock's ioloop.READ events to io_loop
        callback = functools.partial(self._read_resolve, QFResolver.SOCK)
        io_loop.add_handler(QFResolver.SOCK, callback, io_loop.READ)  
    
    def resolve(self, host, port, family=socket.AF_UNSPEC, callback=None):
        # may using regex to check if host is a IP
        # if host is a IP, this may not be called
        try:
            if host.count('.') == 3 and all(0<=int(num)<256 for num in host.strip().split('.')):
                return callback(socket.AF_INET, (host, port))
        except:
            pass 
            
        log.debug("callback:%s", callback)
        log.debug("resolve: %s:%s", host, port)
        try:
            key = "%s:%s" % (host, port)
            # try find in CACHE
            if QFResolver.CACHE.get(key, ''):
                add_time = QFResolver.CACHE[key][1]
                if time.time() - add_time < config.resolve_max_life:
                    if callback:
                        callback(QFResolver.CACHE[key][0])
                        return
                    else:
                        return QFResolver.CAHCE[key][0]
                else:
                    QFREsolver.CACHE.pop(key)
                    
            request_id = os.urandom(2)
            request_str = build_request(host, QTYPE_A)
            log.debug("host:%s", host)
            QFResolver.SOCK.send(str(request_id) + str(request_str))
            QFResolver.CALLBACK[host] = callback
            QFResolver.MAPPING[host] = (host, port)
            log.debug("mapping:%s", QFResolver.MAPPING)
        except:
            log.debug(traceback.format_exc())
    
    def _read_resolve(self, sock, fd, event):
        ''' read at mostest 1024 bytes and parse it '''
        data = QFResolver.SOCK.recv(1024)
        self._parse_resolve(data) 

    def _parse_resolve(self, data):
        result = parse_response(data)        
        addrs = []
        for r in result.answers:
            if r[1] == QTYPE_A:
                addrs.append((socket.AF_INET, (r[0], QFResolver.MAPPING[result.hostname][1])))
        if addrs:
            # add to CACHE
            QFResolver.CACHE[result.hostname] = (addrs, time.time())
            del QFResolver.MAPPING[result.hostname]
            # call the callback
            QFResolver.CALLBACK[result.hostname](addrs)
            del QFResolver.CALLBACK[result.hostname]
