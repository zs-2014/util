#coding: utf-8
import traceback
import types
import datetime
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
            self.log.setLevel(logging.DEBUG)

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
                                  passwd=self.config['passwd'], db=self.config['db'], charset=self.config.get('charset', 'utf8'), 
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
        return self.conn.autocommit(False)

    def end_transaction(self):
        try:
            return self.conn.commit()
        except Exception, e:
            self.log.warn(traceback.format_exc())
            self.conn.rollback()
            raise
        finally:
            self.conn.autocommit(self.auto_commit)

    def escape(self, o):
        if type(o) == datetime.datetime:
            o = o.strftime('%Y-%m-%d %H:%M:%S')
        elif type(o) == datetime.date:
            o = o.strftime('%Y-%m-%d')
        return self.conn.escape(unicode_to_utf8(o)) 

    def escape_string(self, s):
        return self.conn.escape_string(unicode_to_utf8(s))

    def insert_id(self):
        return self.conn.insert_id()

    @reconnect_mysql
    def execute(self, sql, args=None, is_dict=True):
        self.log.debug(sql)
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
        if args is not None:
            sql = '%s limit 1' % (sql % self.escape(args))
        else:
            sql = '%s limit 1' % sql
        _, result = self.execute(sql, is_dict=is_dict)
        if result:
            return result[0]
        else:
            return None

    def dict2sql(self, d, sp, escape_string=None, escape=None, add_escape_ch=True):
        escape_string = escape_string or self.escape_string
        escape = escape or self.escape
        sql = [] 
        or_sql = []
        list_type = (types.ListType, types.TupleType)
        for k, v in d.iteritems():
            if not k.startswith('`') and add_escape_ch:
                k = '`%s`' % k
            if isinstance(v, list_type):
                op, value = v[0].upper(), v[1]
                if op == 'BETWEEN':
                    sql.append('(%s BETWEEN %s AND %s)' % (escape_string(k), 
                                                         escape(value[0]), 
                                                         escape(value[1])))
                elif isinstance(value, list_type):
                    #k in (1,2,3,4)
                    sql.append('%s %s (%s)' % (escape_string(k), 
                                               escape_string(op), 
                                                 ','.join([escape(vv) for vv in value])))
                else:
                    sql.append('%s %s %s' % (escape_string(k),
                                              escape_string(op),
                                              escape(value)))
            elif k.upper() in ('`OR`', 'OR') and isinstance(v, types.DictType):
                or_sql.append('%s' % self.where2sql(v, escape_string, escape, add_escape_ch))
            else:
                sql.append('%s=%s' % (escape_string(k), escape(v)))
        if len(or_sql) == 0:
            return sp.join(sql)
        elif len(sql) == 0:
            return sp.join(or_sql)
        else:
            return '%s OR (%s)' % (sp.join(sql), sp.join(or_sql))

    def where2sql(self, where, escape_string=None, escape=None, add_escape_ch=True):
        if not where:
            return '1'
        return self.dict2sql(where, ' AND ', escape_string, escape, add_escape_ch)

    def format_table_dict(self, key_table, value_table, d):
        if not d:
            return None
        ret_d = {}
        key_table = unicode_to_utf8(key_table)
        value_table = unicode_to_utf8(value_table)
        format_value = lambda v1: v1 if value_table is None else '%s.`%s`' % (value_table, self.escape_string(v1))
        format_key = lambda k1: k1 if key_table is None else '%s.`%s`' % (key_table, self.escape_string(k1))
        list_type = (types.ListType, types.TupleType)
        for k, v in d.iteritems():
            new_k = format_key(k) 
            if k.upper() == 'OR' and isinstance(v, types.DictType):
                ret_d[k] = self.format_table_dict(key_table, value_table, v)
            elif isinstance(v, types.DictType):
                ret_d[new_k] = self.format_table_dict(key_table, value_table, v)
            elif isinstance(v, list_type):
                #op, value
                v = v[1]
                if isinstance(v, list_type):
                    ret_d[new_k] = [format_value(vv) for vv in v] 
                else:
                    ret_d[new_k] = format_value(v)
            else:
                ret_d[new_k] = format_value(v) 
        return ret_d

    def select_join(self, tables, wheres, relation, way, other=None, fields='*'):
        tb_nm = tables[0] 
        where = self.format_table_dict(tb_nm, None, wheres[0])
        where_sql = self.where2sql(where, add_escape_ch=False)
        join_tb_nm = tables[1]
        join_where = self.format_table_dict(join_tb_nm, None, wheres[1])
        join_where_sql = self.where2sql(join_where, add_escape_ch=False)
        escape = unicode_to_utf8
        escape_string = unicode_to_utf8
        relation = self.format_table_dict(tb_nm, join_tb_nm, relation)
        relation_sql = self.where2sql(relation, escape_string, escape, False)
        sql = 'SELECT %s FROM %s %s JOIN %s ON %s WHERE %s AND %s %s' % (self.escape_string(fields),
                                                                            tb_nm, way, join_tb_nm, 
                                                                            relation_sql,
                                                                            where_sql, join_where_sql,
                                                                            '' if other is None else other)
        _, result = self.execute(sql)
        return result 


    def select_left_join(self, tables, wheres, relation, other=None, fields='*'):
        return self.select_join(tables, wheres, relation, 'LEFT', other, fields)

    def select_right_join(self, tables, wheres, relation, other=None, fields='*'):
        return self.select_join(tables, wheres, relation, 'RIGHT', other, fields)

    def select_inner_join(self, tables, wheres, relation, other=None, fields='*'):
        return self.select_join(tables, wheres, relation, 'INNER', other, fields)

    def update(self, table, where, values, other=None):
        sql = 'UPDATE %s SET %s WHERE %s %s' % (self.escape_string(table),
                                             self.dict2sql(values, ','),
                                             self.where2sql(where),
                                             '' if other is None else other)
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
        sql = 'INSERT INTO %s(%s) VALUES %s ' % (self.escape_string(table), fields, vals_str)
        ret, _ = self.execute(sql, is_dict=False)
        return ret
                                                  
    def delete(self, table, where, other=None):
        sql = 'DELETE FROM %s WHERE %s %s' % (self.escape_string(table),
                                           self.where2sql(where), 
                                           '' if other is None else other) 
        ret, _ = self.execute(sql, is_dict=False)
        return ret

    def select(self, table, where, other=None, fields='*', is_dict=True):
        sql = 'SELECT %s FROM %s WHERE %s %s' % (self.escape_string(fields),
                                              self.escape_string(table),
                                              self.where2sql(where), 
                                              '' if other is None else other)
        return self.query(sql, is_dict=is_dict)
                                              
    def select_one(self, table, where, other=None, fields='*', is_dict=True):
        sql = 'SELECT %s FROM %s WHERE %s %s' % (self.escape_string(fields),
                                              self.escape_string(table),
                                              self.where2sql(where),
                                              '' if other is None else other)
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
            return self.dbs.pop()
        
    def release(self, conn):
        with self.lock:
            old_cnt = len(self.dbs)
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
                delattr(ins, 'db')
                release(dbs, self.dbnames)
        return _

@contextmanager
def get_connection(dbnames):
    try:
        dbs = acquire(dbnames)
        yield dbs 
    except Exception, e:
        dbs = None
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
        try:
            for nm in dbnames:
                dbs[nm] = dbpool[nm].acquire()  
            return dbs
        except Exception, e:
            release(dbs, dbs.keys())
            raise
    else:
        return dbpool[dbnames].acquire()

def release(dbs, dbnames):
    if not dbs or not dbnames:
        return
    if isinstance(dbnames, (types.ListType, types.TupleType)):
        for nm in dbnames:
            dbpool[nm].release(dbs[nm])
    else:
        dbpool[dbnames].release(dbs)

