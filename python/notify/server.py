#coding: utf-8
from gevent import monkey
monkey.patch_all()
from geventserver import GeventServer

import config
from util import install_logger
install_logger(config.logfile)

from notify_handler import NotifyHandler 
from notify.Notifier import Processor
from thrift.transport.TTransport import TBufferedTransportFactory 
from thrift.protocol.TBinaryProtocol import TBinaryProtocolFactory 
from worker import Workers
from gevent.queue import Queue
#from multiprocessing import Queue


def start_server():
    queue = Queue(config.msg_queue_max_len)
    processor = Processor(NotifyHandler(queue, config.server_threads)) 
    trans = TBufferedTransportFactory()
    prot = TBinaryProtocolFactory()
    gserver = GeventServer(processor, trans, prot)
    #gserver.run(config.server_works, config.server_threads)
    worker = Workers(gserver, queue)
    worker.run()

if __name__ == '__main__':
    start_server()

