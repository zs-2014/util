#coding: utf-8
import random
import time
import struct
import traceback
import socket
import sys, os

'''
dns header(12 bytes)
+------------------------------------------------------------------------------------------------------+
|  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10  |  11  |  12  |  13  | 14   | 15   |  16  |
+------------------------------------------------------------------------------------------------------+
|                                    ID                                                                |
+------------------------------------------------------------------------------------------------------+
| QR  |       Opcode          | AA  | TC  |  RD | RA  |         Z          |         RCode             |
+------------------------------------------------------------------------------------------------------+
| RD  | TC | AA   | Opcode                | QR  |    Rcode                 |          Z          |  RA |
+------------------------------------------------------------------------------------------------------+
|                                   QDCount                                                            |
+------------------------------------------------------------------------------------------------------+
|                                   ANSCount                                                           |
+------------------------------------------------------------------------------------------------------+
|                                   QRCount                                                            |
+------------------------------------------------------------------------------------------------------+
ID:长度为16位，是一个用户发送查询的时候定义的随机数，当服务器返回结果的时候，返回包的ID与用户发送的一致.
QR:长度1位，值0是请求，1是应答.
Opcode:长度4位，值0是标准查询，1是反向查询，2死服务器状态查询。
AA:长度1位，授权应答(Authoritative Answer) – 这个比特位在应答的时候才有意义，指出给出应答的服务器是查询域
   名的授权解析服务器
TC:长度1位，截断(TrunCation) – 用来指出报文比允许的长度还要长导致被截断
RD:长度1位，期望递归(Recursion Desired) – 这个比特位被请求设置，应答的时候使用的相同的值返回。如果设置了RD,
   就建议域名服务器进行递归解析，递归查询的支持是可选的。
RA:长度1位，支持递归(Recursion Available) – 这个比特位在应答中设置或取消，用来代表服务器是否支持递归查询.
Z :长度3位，保留值，值为0.
RCode:长度4位，应答码，类似http的stateCode一样，值0没有错误、1格式错误、2服务器错误、3名字错误、4服务器不支
      持、5拒绝
QDCount:长度16位，报文请求段中的问题记录数.
ANCount:长度16位，报文回答段中的回答记录数.
NSCOUNT:长度16位，报文授权段中的授权记录数.
ARCOUNT:长度16位，报文附加段中的附加记录数.

Query
+------------------------------------------------------------------------------------------------------+
|  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10  |  11  |  12  |  13  | 14   | 15   |  16  |
+------------------------------------------------------------------------------------------------------+
|                                   Name: 不定长                                                       |
+------------------------------------------------------------------------------------------------------+
|......................................................................................................|
+------------------------------------------------------------------------------------------------------+
|                                   QType                                                               |
+------------------------------------------------------------------------------------------------------+
|                                   QClass                                                              |
+------------------------------------------------------------------------------------------------------+

QType：长度16位，表示查询类型。取值大概如下：
enum QueryType //查询的资源记录类型。
{
A=0×01, //指定计算机 IP 地址。
NS=0×02, //指定用于命名区域的 DNS 名称服务器。
MD=0×03, //指定邮件接收站（此类型已经过时了，使用MX代替）
MF=0×04, //指定邮件中转站（此类型已经过时了，使用MX代替）
CNAME=0×05, //指定用于别名的规范名称。
SOA=0×06, //指定用于 DNS 区域的“起始授权机构”。
MB=0×07, //指定邮箱域名。
MG=0×08, //指定邮件组成员。
MR=0×09, //指定邮件重命名域名。
NULL=0x0A, //指定空的资源记录
WKS=0x0B, //描述已知服务。
PTR=0x0C, //如果查询是 IP 地址，则指定计算机名；否则指定指向其它信息的指针。
HINFO=0x0D, //指定计算机 CPU 以及操作系统类型。
MINFO=0x0E, //指定邮箱或邮件列表信息。
MX=0x0F, //指定邮件交换器。
TXT=0×10, //指定文本信息。
UINFO=0×64, //指定用户信息。
UID=0×65, //指定用户标识符。
GID=0×66, //指定组名的组标识符。
ANY=0xFF //指定所有数据类型。
};

QClass: 长度为16位，表示分类
enum QueryClass //指定信息的协议组。
{
IN=0×01, //指定 Internet 类别。
CSNET=0×02, //指定 CSNET 类别。（已过时）
CHAOS=0×03, //指定 Chaos 类别。
HESIOD=0×04,//指定 MIT Athena Hesiod 类别。
ANY=0xFF //指定任何以前列出的通配符。
};
Response
+------------------------------------------------------------------------------------------------------+
|  1  |  2  |  3  |  4  |  5  |  6  |  7  |  8  |  9  |  10  |  11  |  12  |  13  | 14   | 15   |  16  |
+------------------------------------------------------------------------------------------------------+
|                                   Name: 不定长                                                       |
+------------------------------------------------------------------------------------------------------+
|......................................................................................................|
+------------------------------------------------------------------------------------------------------+
|                                   Type                                                               |
+------------------------------------------------------------------------------------------------------+
|                                   Class                                                              |
+------------------------------------------------------------------------------------------------------+
|                                   TTL                                                                |
+------------------------------------------------------------------------------------------------------+
|                                   RDLength                                                           |
+------------------------------------------------------------------------------------------------------+
|                                   RDData(不定长,由RDLength指定)                                      |
+------------------------------------------------------------------------------------------------------+
|                                   Name: 不定长                                                       |
+------------------------------------------------------------------------------------------------------+
|......................................................................................................|
+------------------------------------------------------------------------------------------------------+
|                                   Type                                                               |
+------------------------------------------------------------------------------------------------------+
|                                   Class                                                              |
+------------------------------------------------------------------------------------------------------+
|                                   TTL                                                                |
+------------------------------------------------------------------------------------------------------+
|                                   RDLength                                                           |
+------------------------------------------------------------------------------------------------------+
|                                   RDData(不定长,由RDLength指定)                                      |
+------------------------------------------------------------------------------------------------------+
|                                   .................                                                  |
+------------------------------------------------------------------------------------------------------+
'''

unicode_to_utf8 = lambda t: t.encode('utf-8') if isinstance(t, unicode) else t

class RespError(Exception):
    RESP_OK = 0X0
    REQUEST_DATA_FORMAT_ERROR = 0X01
    DNS_SERVER_ERROR = 0X02
    DOMAIN_NAME_ERROR = 0X03
    UNSUPPORT_ERROR = 0X04
    DNS_SERVER_REFUSED = 0X05
    DNS_RESPONSE_FORMAT_ERROR = 0X06
    UNKNOWN_NAME_ERROR = 0X07

    def __init__(self, errcd, msg, *args, **kwargs):
        super(RespError, self).__init__(self, *args, **kwargs)
        self.errcode = errcd
        self.msg = msg

    def __str__(self):
        return 'RespError: errorcode=%d errmsg=%s' % (self.errcode, self.msg)
    def __repr__(self):
        return str(self)

class ResourceRecord(object):
    query_type_map = { 0x01: 'A', #指定计算机 IP 地址。
                       0x02: 'NS', #指定用于命名区域的 DNS 名称服务器。
                       0x03: 'MD', #指定邮件接收站（此类型已经过时了，使用MX代替）
                       0x04: 'MF', #指定邮件中转站（此类型已经过时了，使用MX代替）
                       0x05: 'CNAME', #指定用于别名的规范名称。
                       0x06: 'SOA', #指定用于 DNS 区域的“起始授权机构”。
                       0x07: 'MB', #指定邮箱域名。
                       0x08: 'MG', #指定邮件组成员。
                       0x09: 'MR', #指定邮件重命名域名。
                       0x0A: 'NULL', #指定空的资源记录
                       0x0B: 'WKS', #描述已知服务。
                       0x0C: 'PTR', #如果查询是 IP 地址，则指定计算机名；否则指定指向其它信息的指针。
                       0x0D: 'PTR', #指定计算机 CPU 以及操作系统类型。
                       0x0E: 'MINFO', #指定邮箱或邮件列表信息。
                       0x0F: 'MX', #指定邮件交换器。
                       0x10: 'TXT', #指定文本信息。
                       0x64: 'UINFO', #指定用户信息。
                       0x65: 'UID', #指定用户标识符。
                       0x66: 'GID', #指定组名的组标识符。
                       0xFF: 'ANY' #指定所有数据类型。 
                       }

    def __init__(self, name, qtp, qcls, ttl, data):
        self.name = name 
        self.qtp = qtp
        self.qcls = qcls
        self.ttl = ttl
        self.data = data
        self.create_tm = int(time.time())

    def is_expired(self):
        return int(time.time()) > self.create_tm + self.ttl

    def get_data(self):
        return self.data 
    def is_CNAME_record(self):
        return self.qtp == 0x05
    def is_A_record(self):
        return self.qtp == 0x01

    def get_ip(self):
        if len(self.data) == 4:
            return socket.inet_ntop(socket.AF_INET, self.data)
        #maybe ipv6
        else:
            return socket.inet_ntop(socket.AF_INET6, self.data)
    def __str__(self):
        if self.is_A_record():
            data = self.get_ip() 
        else:
            data = self.data
        return '%s       %s         %s\n' % (self.name, self.query_type_map.get(self.qtp, 'Unknown type'), data)
    def __repr__(self):
        return str(self)

class DNSResp(object):

    def __init__(self, resp):
        self.resp = unicode_to_utf8(resp)
        self.id = -1
        self.flags = 0
        self.qdcount = 0
        self.ancount = 0
        self.nscount = 0
        self.arcount = 0
        self.offset = 0

    def parse_ptr_name(self):
        new_offset, =  struct.unpack('!H', self.resp[self.offset: self.offset+2]) 
        new_offset = new_offset & 0x3fff
        if self.offset <= new_offset:
            raise RespError(RespError.DNS_RESPONSE_FORMAT_ERROR,'unknow domain name ptr at: 0x%x (0x%x)' % (self.offset, new_offset)) 
        old_offset = self.offset
        #这个地方应该判断当前offset和new_offset的值
        self.offset = new_offset
        offset_name = self.parse_name()
        self.offset = old_offset + 2
        return offset_name

    def parse_name_section(self, n):
        name = []
        while n > 0 and len(self.resp) > self.offset:
            c, = struct.unpack('!B', self.resp[self.offset: self.offset+1])
            #通过偏移定位
            if (c & 0xc0) == 0xc0:
                return '%s.%s' (''.join(name), self.parse_ptr_name())
            else:
                self.offset += 1
                name.append(chr(c))
            n = n - 1
        return ''.join(name)

    def parse_name(self):
        name_sections = []
        while len(self.resp) > self.offset:
            n, = struct.unpack('!B', self.resp[self.offset: self.offset+1])
            if n == 0:
                self.offset += 1
                return '.'.join(name_sections)
            if (n & 0xc0) == 0xc0:
                return '%s.%s' % ('.'.join(name_sections), self.parse_ptr_name()) 
            else:
                self.offset += 1
                name_sections.append(self.parse_name_section(n))

    #解析dns响应头部
    def parse_header(self):
        self.id, self.flags, self.qdcount, self.ancount, self.nscount, self.arcount = struct.unpack('!6H', self.resp[:12])
        self.offset += 12

    def get_response_code(self):
        return ((self.flags & 0xf000) >> 8) & 0xf 
    def response_is_ok(self):
        return self.get_response_code() == 0x0
    def raise_response_exception(self):
        rspcd = self.get_response_code() 
        if rspcd == RespError.REQUEST_DATA_FORMAT_ERROR:
            raise RespError(rspcd, '数据格式错误')
        elif rspcd == RespError.DNS_SERVER_ERROR:
            raise RespError(rspcd, 'DNS server 错误')
        elif rspcd == RespError.DOMAIN_NAME_ERROR:
            raise RespError(rspcd, '无法解析的域名')
        elif rspcd == RespError.UNSUPPORT_ERROR:
            raise RespError(rspcd, 'DNS server 不支持的域名解析')
        elif rspcd == RespError.DNS_SERVER_REFUSED:
            raise RespError(rspcd, 'DNS server refused')
        raise RespError(rspcd, 'unknown response code')

    def parse_response(self):
        try:
            self.parse_header() 
            if not self.response_is_ok():
                self.raise_response_exception()
            qdcount = self.qdcount
            #print qdcount
            #如果请求记录存在,则跳过
            host_name = None
            while qdcount > 0:
                host_name = self.parse_name()   
                _, _ = struct.unpack('!HH', self.resp[self.offset: self.offset+4])
                self.offset += 4
                qdcount -= 1
            
            if self.ancount <= 0:
                raise RespError(RespError.UNKNOWN_NAME_ERROR, 'unknown host name[%s]' % host_name)
            ancount = self.ancount+self.nscount+self.arcount
            records = []
            while ancount > 0 and len(self.resp) > self.offset:
                name = self.parse_name().strip('.')
            #    print name
                tp, cls, ttl, rdlen = struct.unpack('!HHiH', self.resp[self.offset: self.offset+10])
            #    print tp, cls, ttl, rdlen
                if rdlen < 0 :
                    raise RespError(RespError.DNS_RESPONSE_FORMAT_ERROR, 'dns 服务器响应数据格式错误(rdlen=%d)', rdlen)
                self.offset += 10 
                if tp in (0x05, 0x02):
                    old_offset = self.offset
                    data = self.parse_name().strip('.')
                    self.offset = old_offset 
                else:
                    fmt = '!%ds' % rdlen
                    data, = struct.unpack(fmt, self.resp[self.offset: self.offset+rdlen])
                self.offset += rdlen
                records.append(ResourceRecord(name, tp, cls, ttl, data))
                ancount -= 1
            return records
        except struct.error, se:
            raise RespError(RespError.DNS_RESPONSE_FORMAT_ERROR, 'dns 服务器响应数据格式错误')

class DNSReq(object):
    def __init__(self):
        pass
    #坑爹的翻译文档
    @classmethod
    def make_query_packet(cls, host_names, tc_flag=0, rd_flag=1):

        if tc_flag not in (0, 1):
            raise ValueError('tc_flag(%s) not in (0, 1)' % tc_flag) 
        if rd_flag not in (0, 1):
            raise ValueError('rd_flag(%s) not in (0, 1)' % rd_flag)
        if isinstance(host_names, basestring):
            host_names = [host_names]

        random_id = random.randint(0, 0xFFFF)
        qr_flag = 0
        opcode = 0
        aa_flag = 0
        ra_flag = 0
        z = 0
        rcode = 0
        qdcount = len(host_names)
        ancount = 0
        nscount = 0
        arcount = 0
        msg = []
        flags = rcode|(z << 4)|(ra_flag << 7)|(rd_flag << 8)|(tc_flag << 9)|(aa_flag << 10)|(opcode << 11)|(qr_flag << 15)
        msg.append(struct.pack('!6H', random_id, flags, qdcount, ancount, nscount, arcount))
        for h in host_names:
            msg.append(cls.make_question_packet(h))
        return ''.join(msg)
        
    @classmethod
    def make_question_packet(cls, host_name, qtype=0x01, qclass=0x01):
        if qtype < 0x01 or qtype > 0x0c:
            raise ValueError('qtype(%s) not in range(%s)' % (qtype, range(0x01, 0x0d))) 
        if qclass not in (0x01, 0x02, 0x03, 0x04, 0xff):
            raise ValueError('qclass(%s) not in range(%s)' % (qclass, str([0x01, 0x02, 0x03, 0x04, 0xff])))
        msg = []
        host_sec = host_name.split('.')
        for h in host_sec: 
            fmt = '!%dB' % (len(h) + 1)
            args = [ord(c) for c in h]
            msg.append(struct.pack(fmt, len(h), *args))
        msg.append(struct.pack('!B', 0x00))
        msg.append(struct.pack('!H', qtype))
        msg.append(struct.pack('!H', qclass))
        return ''.join(msg)


