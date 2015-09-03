#coding: utf-8

import traceback
import types
import time
from MySQLdb import connect as mysql_connect
from MySQLdb import OperationalError
from MySQLdb.cursors import SSDictCursor 
from threading import Lock as ThreadLock
from threading import Condition as ThreadCondition
from contextlib import contextmanager
import logging

unicode_to_utf8 = lambda t: t.encode('utf-8') if isinstance(t, types.UnicodeType) else t

def reconnect_mysql(func):
    def _(self, *args, **kwargs):
        try:
            return func(self, *args, **kwargs)
        except OperationalError, e:
            #self.log.warn(traceback.format_exc())
            #reconnect it
            if e.args[0] == 2006:
                self.reconnect()
                return func(self, *args, **kwargs)
            raise
    return _


class Connection(object):
    def __init__(self, config, autocommit=True):
        self.log = logging.getLogger()
        if not self.log.handlers:
            logging.basicConfig()
            self.log = logging.getLogger()

        self.config = config
        self.auto_commit = autocommit
        self.conn = None #mysql_connect(host=config['host'], port=config['port'], user=config['user'], 
                         #          passwd=config['db'], db=config['db'], charset=config.get('charset', 'utf-8'),
                         #          connect_timeout=config.get('timeout'))
        self.last_use_tm = int(time.time())
    def reconnect(self):
        if self.conn is not None:
            try:
                self.conn.ping(True)
                return True
            except OperationalError, e:
                if e.args[0] != 2006:
                    raise 
            try:
                self.conn.ping()
                return True
            except OperationalError, e:
                if e.args[0] != 2006:
                    raise
        self.conn = mysql_connect(host=self.config['host'], port=self.config['port'], user=self.config['user'], 
                                  passwd=self.config['passwd'], db=self.config['db'], charset=self.config.get('charset', 'utf-8'), 
                                  connect_timeout=self.config.get('timeout'))
        self.conn.autocommit(self.auto_commit)
        return True

    connect = reconnect 
                        
    def is_connected(self):
        return self.conn is not None

    def close(self):
        self.conn.close()
        self.conn = None

    def begin_transaction(self):
        if not self.auto_commit:
            return True
        self.conn.autocommit(False)

    def end_transaction(self):
        try:
            self.conn.commit()
        except Exception, e:
            self.log.warn(traceback.format_exc())
            self.conn.rollback()
            raise
        finally:
            self.conn.autocommit(self.auto_commit)

    def escape(self, o):
        return self.conn.escape(unicode_to_utf8(o)) 

    def escape_string(self, s):
        return self.conn.escape_string(unicode_to_utf8(s))

    def insert_id(self):
        return self.conn.insert_id()

    @reconnect_mysql
    def execute(self, sql, args=None, is_dict=True):
        print sql
        if is_dict:
            cur =  self.conn.cursor(SSDictCursor)
        else:
            cur = self.conn.cursor()
        ret = cur.execute(sql, args)
        result = cur.fetchall()
        cur.close()
        return ret, result

    def query(self, sql, args=None, is_dict=True):
        _, result = self.execute(sql, args, is_dict) 
        return result
    
    def get(self, sql, args=None, is_dict=True):
        if isinstance(args, types.DictType):
            args = self.escape(args)
        elif isinstance(args, (types.ListType, types.TupleType)):
            args = [self.escape(s) if isinstance(s, basestring) else s for s in args]
        elif isinstance(args, basestring):
            args = self.escape(args)
        if args is not None:
            sql = '%s limit 1' % (sql % args)
        _, result = self.execute(sql, is_dict=is_dict)
        if result:
            return result[0]
        else:
            return None

    def dict2sql(self, d, sp):
        sql = [] 
        list_type = (types.ListType, types.TupleType)
        for k, v in d.iteritems():
            if not k.startswith('`'):
                k = '`%s`' % k
            if isinstance(v, list_type):
                op, value = v[0].upper(), v[1]
                if op == 'BETWEEN':
                    sql.append('%s BETWEEN %s AND %s' % (self.escape_string(k), 
                                                         self.escape(value[0]), 
                                                         self.escape(value[1])))
                elif isinstance(value, list_type):
                    #k in (1,2,3,4)
                    sql.append(' %s %s (%s) ' % (self.escape_string(k), 
                                                 self.escape_string(op), 
                                                 ','.join([self.escape(vv) for vv in value])))
                else:
                    sql.append(' %s %s %s ' % (self.escape_string(k),
                                              self.escape_string(op),
                                              self.escape(value)))
            elif k == '`OR`' and isinstance(v, types.DictType):
                sql.append(' OR (%s) ' % self.dict2sql(v))
            else:
                sql.append(' %s=%s ' % (self.escape_string(k), self.escape(v)))
        return sp.join(sql)

    def where2sql(self, where):
        if not where:
            return '1'
        return self.dict2sql(where, ' AND ')

    def update(self, table, where, values):
        sql = 'UPDATE %s SET %s WHERE %s' % (self.escape_string(table),
                                             self.dict2sql(values, ' , '),
                                             self.where2sql(where))
        ret, _ = self.execute(sql, is_dict=False)
        return ret

    def insert(self, table, values):
        if isinstance(values, types.DictType):
            values_list = [values]
        keys = values_list[0].keys()
        vals = []
        fields = ','.join(['`%s`'% k for k in keys])
        for value_dict in values_list:
            val_str = []
            for k in keys:
                val_str.append(self.escape(value_dict[k]))
            vals.append('(%s)' % ','.join(val_str))
        vals_str = ','.join(vals)
        sql = 'INSERT INTO %s(%s) VALUES %s ' % (self.escape_string(tablel), fields, vals_str)
        ret, _ = self.execute(sql, is_dict=False)
        return ret
                                                  
    def delete(self, table, where):
        sql = 'DELETE FROM %s WHERE %s' % (self.escape_string(table),
                                           self.where2sql(where)) 
        ret, _ = self.execute(sql, is_dict=False)
        return ret

    def select(self, table, where, fields='*', id_dict=True):
        sql = 'SELECT %s FROM %s WHERE %s' % (self.escape_string(fields),
                                              self.escape_string(table),
                                              self.where2sql(where))
        return self.query(sql, is_dict=is_dict)
                                              
    def select_one(self, table, where, fields, is_dict=True):
        sql = 'SELECT %s FROM %s WHERE %s limit 1' % (self.escape_string(fields),
                                                      self.escape_string(table),
                                                      self.where2sql(where))
        return self.get(sql, is_dict=is_dict)

class ConnectionProxy(object):
    def __init__(self, master_conn, slave_conn):
        self.master_method = ('execute', 'update', 'insert', 'delete', 'begin_transaction', 'end_transaction')
        self.master_conn = master_conn
        self.slave_conn = slave_conn
    def __getattr__(self, method):
        if not self.slave_conn or method in self.master_method:
            if not self.master_conn.is_connected():
                self.master_conn.connect()
            self.master_conn.last_use_tm = int(time.time())
            return getattr(self.master_conn, method)
        if not self.slave_conn.is_connected():
            self.slave_conn.connect()
        self.slave_conn.last_use_tm = int(time.time())
        return getattr(self.slave_conn, method)
        
class DBPool(object):
    def __init__(self, dbconfig, maxconn):
        self.lock = ThreadLock() 
        self.cond = ThreadCondition(self.lock)
        self.dbs = []
        for _ in range(0, maxconn):
            master_conn = None
            slave_conn = None
            if 'master' in dbconfig:
                master_conn  = Connection(dbconfig['master'])

            if 'slave' in dbconfig:
                slave_conn = Connection(dbconfig['slave']) 

            if master_conn is None and slave_conn is None:
                master_conn = Connection(dbconfig)
            self.dbs.append(ConnectionProxy(master_conn, slave_conn))

    def close_idle(self):
        pass

    def acquire(self):
        with self.lock:
            if len(self.dbs) == 0:
                self.cond.wait()
            return self.dbs.pop(0)
        
    def release(self, conn):
        with self.lock:
            old_cnt = self.dbs
            self.dbs.append(conn)
            if not old_cnt:
                self.cond.notify()

class WithDatabase(object):
    def __init__(self, dbnames):
        self.dbnames = dbnames

    def __call__(self, func):
        def _(ins, *args, **kwargs):
            dbs = acquire(self.dbnames) 
            try:
                ins.db = dbs
                ret = func(ins, *args, **kwargs)
                return ret
            finally:
                ins.db = None
                release(dbs, self.dbnames)
        return _

@contextmanager
def get_connection(dbnames):
    try:
        dbs = acquire(dbnames)
        yield dbs 
    finally:
        release(dbs, dbnames)

dbpool = {}

def install_database(dbconf):
    global dbpool
    for k, v  in dbconf.iteritems():
        dbpool[k] = DBPool(v, v.get('conn', 16))
    return dbpool

def acquire(dbnames):
    dbs = None
    if isinstance(dbnames, (types.ListType, types.TupleType)):
        dbs = {}
        for nm in dbnames:
            dbs[names] = dbpool[nm].acquire()  
        return dbs
    else:
        return dbpool[dbnames].acquire()

def release(dbs, dbnames):
    if isinstance(dbnames, types.DictType):
        for nm in dbnames:
            dbpool[nm].realse(dbs[nm])
    else:
        dbpool[dbnames].release(dbs)

class Test(object):
    @WithDatabase('test')
    def test_select_one(self):
        self.db.select_one(table='`order`', where={'order_id': '12345', 'id': 12345}, fields='*')
        self.db.select_one(table='`order`', where={'order_id': ('in', (1,2,'3')), 'id': ('between', (3,4))})

dbconfig = {'test': 
                      {'master': 
                                {'db': 'test',
                                'host': '172.100.101.106',
                                'port': 3306,
                                'user': 'test',
                                'passwd': '123456',
                                'charset': 'utf8',
                                'timeout': 10}, 
                        'conn': 1,
                        'slave': 
                                {'db': 'test',
                                'host': '172.100.101.106',
                                'port': 3306,
                                'user': 'test',
                                'passwd': '123456',
                                'charset': 'utf8',
                                'timeout': 10}}
            }

install_database(dbconfig)

if __name__ == '__main__':
    t = Test() 
    t.test_select_one()
