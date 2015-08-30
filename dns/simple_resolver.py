#coding: utf-8

from dns_protocol import DNSResp, DNSReq
import socket

class SimpleDNSResolver(object):
    def __init__(self, dns_server, port=53):
        self.dns_server = dns_server
        self.port = port

    def print_binary(self, content, sep):
        msg = []
        for i in range(2, len(content), 2):
            msg.append('%02x%02x' % (ord(content[i-2]), ord(content[i-1])))
        print sep.join(msg)

    def resolve_addr(self, host_names):
        pckt = DNSReq.make_query_packet(host_names, tc_flag=1, rd_flag=1)
        #self.print_binary(pckt, ' ') 
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        sock.sendto(pckt, (self.dns_server, self.port))
        s,_ = sock.recvfrom(1024)
        #self.print_binary(s, ' ')
        records = DNSResp(s).parse_response()
        #得到cname
        names = [r.get_data() for r in filter(lambda x: x.is_CNAME_record(), records)]
        #print names
        #得到names
        names.extend(host_names if not isinstance(host_names, basestring) else [host_names]) 
        records = filter(lambda r: r.name in names, records)
        ret = [r for r in filter(lambda r: r.is_A_record(), records)]
        print(str(ret))
        return ret 

if __name__ == '__main__':
    SimpleDNSResolver('114.114.114.114', 53).resolve_addr('www.baidu.com')
