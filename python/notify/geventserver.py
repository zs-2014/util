#coding: utf-8
from gevent.monkey import patch_all 
patch_all()
from gevent.server import StreamServer
import os, sys

from thrift.transport.TSocket import TSocket
from thrift.transport.TTransport import TTransportException
import multiprocessing
import logging
import util
import traceback

log = logging.getLogger()

class GeventServer(StreamServer):
    def __init__(self, processor, itrans, iprot, otrans=None, oprot=None, listener="0.0.0.0:9090", handle=None, backlog=None, spawn='default', **ssl_args):
        super(GeventServer, self).__init__(listener, handle if handle is not None else self.handle_stream, backlog, spawn, **ssl_args)
        self.itrans = itrans
        self.otrans = otrans if otrans is not None else self.itrans
        self.iprot = iprot
        self.oprot = oprot if oprot is not None else self.iprot
        self.processor = processor
        self.child_pids = []
        self.init_socket()

    @util.timeit
    def handle_stream(self, sock, address):
        tsock = TSocket()
        tsock.setHandle(sock)
        itrans = self.itrans.getTransport(tsock) 
        otrans = self.otrans.getTransport(tsock)
        iprot = self.iprot.getProtocol(itrans)
        oprot = self.oprot.getProtocol(otrans)
        try:
            while True:
                self.processor.process(iprot, oprot)
        except TTransportException, ex:
            pass
        except Exception, e:
            log.warn(traceback.format_exc()) 
        finally:
            itrans.close()
            otrans.close()
            itrans = None
            otrans = None
            iprot = None
            oprot = None

    def run(self, works=None, threads=1):
        if threads <= 0:
            raise ValueError('works (%s) threads (%s) is not valid' % (works, threads))
        self.set_spawn(threads) 
        #self.init_socket()
        #self.spawn_works(works)
        #self.install_child_process_signal_handler()
        self.serve_forever() 

if __name__ == '__main__':
    gserver = GeventServer(None, None, None)
    gserver.run(None, 1)
