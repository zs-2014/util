#coding: utf-8

from thrift.transport import TTransport
from thrift.transport import TSocket
from thrift.protocol import TBinaryProtocol

from notify import Notifier
from notify.ttypes import *

tsock = TSocket.TSocket('172.100.111.133', 9090)
trans = TTransport.TBufferedTransport(tsock)
protocol = TBinaryProtocol.TBinaryProtocol(trans)
client = Notifier.Client(protocol)
trans.open()

import msgpack
import json

notify_dict = {'url': 'http://172.100.101.106:8084/notify', 
               'data': json.dumps({'test': 'test'})}

print client.send_notify(msgpack.dumps(notify_dict))
#print client.ping()

trans.close()
