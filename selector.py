#coding: utf-8
import operator
import random

class Selector(object):
    def __init__(self, server_list, policy='round_robin'):
        self.curr_pos = 0 
        self.servers = []
        self.policy = policy
        if isinstance(server_list, dict):
            server_list = [server_list]
        for item in server_list:
            newitem = {}
            newitem['valid'] = True
            newitem['server'] = item
            self.servers.append(newitem)
        self._op_map = {'=': 'eq',
                        '!=': 'ne',
                        '<=': 'le',
                        '<' : 'lt',
                        '>=': 'ge',
                        '>': 'gt',
                        'in': 'contains'}

    def filter_by_rule(self, input):
        ava_servers = []
        for item in self.servers:
            if not item['valid']:
                continue
            rule = item['server'].get('rule', '')
            if not rule:
                ava_servers.append(item)
                continue
            if input is not None:
                for r in rule:
                    name, op, v = r
                    input_v = input.get(name) 
                    if not input_v or not getattr(operator, self._op_map[op])(v, input_v):
                        break
                else:
                    ava_servers.append(item)
            else:
                ava_servers.append(item)
        return ava_servers 

    def random(self, input=None):
        ava_servers = self.filter_by_rule(input) 
        if ava_servers:
            return ava_servers[random.randrange(0, len(ava_servers))]
        return None

    def round_robin(self, input=None):
        ava_servers = self.filter_by_rule(input)
        if ava_servers:
            pos = self.curr_pos % len(ava_servers)
            self.curr_pos = (self.curr_pos+1) % len(ava_servers)
            return ava_servers[pos]
        return None
    
    def select_one(self, input=None):
        return getattr(self, self.policy)(input) 
    
    def set_invalid(self, server):
        if server is not None:
            server['valid'] = False

    def restore_invalid(self, func=None):
        if func is None or not hasattr(func, '__call__'):
            return
        for item in self.servers:
            if not item['valid'] and func(item['server']):
                item['valid'] = True 

if __name__ == '__main__':
    serverlist = [{'addr': ('172.100.101.106', '9001'), 'timeout': 2000, 'rule': [('url', 'in', ['/url/v1/select', ]), ]}, 
                  {'addr': ('172.100.101.106', '9002'), 'timeout': 2000, 'rule': [('url', 'in', ['/url/v1/select1', ]), ]},
                  {'addr': ('172.100.101.106', '9003'), 'timeout': 2000 }]
    selector = Selector(serverlist, 'random')
    print selector.select_one()

    server = selector.select_one()
    print server
    selector.set_invalid(server)

    print selector.select_one({'url': '/url/v1/select'})
    print selector.select_one({'url': '/url/v1/select1'})
    print 'after restore...............'
    def check_valid(item):
        return True
    selector.restore_invalid(check_valid)
    print selector.select_one()

    print selector.select_one()

    print selector.select_one({'url': '/url/v1/select'})
    print selector.select_one({'url': '/url/v1/select1'})

    print 'random is end'
    selector = Selector(serverlist)
    print selector.select_one()

    server = selector.select_one()
    print server
    selector.set_invalid(server)

    print selector.select_one({'url': '/url/v1/select'})
    print selector.select_one({'url': '/url/v1/select1'})
    print 'after restore...............'
    def check_valid(item):
        return True
    selector.restore_invalid(check_valid)
    print selector.select_one()

    print selector.select_one()

    print selector.select_one({'url': '/url/v1/select'})
    print selector.select_one({'url': '/url/v1/select1'})

