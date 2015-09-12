#coding: utf-8
from db import install_database, WithDatabase
import datetime
import time
import threading

class Test(object):

    @WithDatabase('test')
    def test_create_table(self):
        sql = 'create table if not exists `test` (id INT primary key auto_increment,\
                                 join_date date default NULL,\
                                 create_time datetime default NULL,\
                                 goods_name varchar(124) not NULL,\
                                 goods_info varchar(2048) default NULL,\
                                 total_amt int default NULL,\
                                 fee_ratio float default NULL) default charset=utf8'
        self.db.execute(sql)
        self.db.execute(sql)

    @WithDatabase('test')
    def test_drop_table(self):
        self.db.execute('drop table test') 
        self.db.execute('drop table test')

    @WithDatabase(('test', 'test1'))
    def test_select_one(self):
        where = None
        ret = self.db['test'].select_one(table='`test`', where=where, fields='count(*) c')
        ret1 = self.db['test1'].select_one(table='`test`', where=where, fields='count(*) c')
        print ret, ret == ret1

        where = {'goods_name': '测试', 'create_time': datetime.datetime.now()}
        ret = self.db['test'].select_one(table='`test`', where=where, fields='goods_name, create_time')
        ret1 = self.db['test1'].select_one(table='`test`', where=where, fields='goods_name, create_time')
        print ret, ret == ret1

        where = {'goods_name': '测试', 
                 'create_time': ('between', (datetime.datetime.now()+datetime.timedelta(seconds=-10), datetime.datetime.now())), 
                 'total_amt': 1, 'fee': 1}
        ret = self.db['test'].select_one(table='`test`', where=where, fields='goods_name, create_time')
        ret1 = self.db['test1'].select_one(table='`test`', where=where, fields='goods_name, create_time')
        print ret, ret == ret1

        where = {'goods_name': '测试', 'total_amt': 1, 'OR': {'fee': 1, 'create_time': datetime.datetime.now()}}
        ret = self.db['test'].select_one(table='`test`', where=where, fields='goods_name, create_time')
        ret1 = self.db['test1'].select_one(table='`test`', where=where, fields='goods_name, create_time')
        print ret, ret == ret1

        where = {'goods_name': '测试', 
                 'join_date': datetime.date.today(), 'fee': 1, 
                 'total_amt': ('in', (1,2,3,4,5,6,7,10))}
        ret = self.db['test'].select_one(table='`test`', where=where, fields='goods_name, join_date')
        ret1 = self.db['test1'].select_one(table='`test`', where=where, fields='goods_name, join_date')
        print ret, ret == ret1

        where = {'goods_name': '测试', 'join_date': datetime.date.today(), 'fee': ('not in', (1, 2, 3)), 'total_amt': 10}
        ret = self.db['test'].select_one(table='`test`', where=where, fields='goods_name, join_date')
        ret1 = self.db['test1'].select_one(table='`test`', where=where, fields='goods_name, join_date')
        print ret, ret == ret1

        where = {'goods_name': ('!=', '测试'), 'join_date': datetime.date.today(), 'fee': ('not in', (1, 2, 3)), 'total_amt': 10}
        ret = self.db['test'].select_one(table='`test`', where=where, fields='goods_name, join_date')
        ret1 = self.db['test1'].select_one(table='`test`', where=where, fields='goods_name, join_date')
        print ret, ret == ret1

    @WithDatabase('test')
    def test_delete(self):
        print self.db.delete(table='test', where={'create_time': ('>=', datetime.datetime.now()),
                                                  'or': {'goods_name': '测试'}})
        print self.db.delete(table='test', where={'id': ('between', (1,4))})
        print self.db.delete(table='test', where={'id': 5})
        print self.db.delete(table='test', where=None)
        print self.db.delete(table='test1', where=None)

    @WithDatabase(('test', 'test1'))
    def test_select(self):
        where = None
        ret = self.db['test'].select(table='`test`', where=where, fields='count(*) c')
        ret1 = self.db['test1'].select(table='`test`', where=where, fields='count(*) c')
        print ret, ret == ret1

        where = {'goods_name': '测试', 'create_time': datetime.datetime.now()}
        ret = self.db['test'].select(table='`test`', where=where, fields='goods_name, create_time')
        ret1 = self.db['test1'].select(table='`test`', where=where, fields='goods_name, create_time')
        print ret, ret == ret1

        where = {'goods_name': '测试', 
                 'create_time': ('between', (datetime.datetime.now()+datetime.timedelta(seconds=-10), datetime.datetime.now())), 
                 'total_amt': 1, 'fee': 1}
        ret = self.db['test'].select(table='`test`', where=where, fields='goods_name, create_time')
        ret1 = self.db['test1'].select(table='`test`', where=where, fields='goods_name, create_time')
        print ret, ret == ret1

        where = {'goods_name': '测试', 'total_amt': 1, 'OR': {'fee': 1, 'create_time': datetime.datetime.now()}}
        ret = self.db['test'].select(table='`test`', where=where, fields='goods_name, create_time')
        ret1 = self.db['test1'].select(table='`test`', where=where, fields='goods_name, create_time')
        print ret, ret == ret1

        where = {'goods_name': '测试', 
                 'join_date': datetime.date.today(), 'fee': 1, 
                 'total_amt': ('in', (1,2,3,4,5,6,7,10))}
        ret = self.db['test'].select(table='`test`', where=where, fields='goods_name, join_date')
        ret1 = self.db['test1'].select(table='`test`', where=where, fields='goods_name, join_date')
        print ret, ret == ret1

    @WithDatabase('test')
    def test_update(self):
        print self.db.update(table='test', where={'id': 1}, values={'goods_name': 'update 测试'})
        print self.db.update(table='test', 
                             where={'goods_name': ('in', ('insert 测试', 'update 测试'))}, 
                             values={'goods_name': 'update 测试'})
        print self.db.update(table='test', where={'fee': 0.75, 'or': {'total_amt': 1}}, 
                                           values={'total_amt': 100, 'goods_name': 'total_amt 测试'})
        print self.db.update(table='test', where={'goods_name': ('!=', '')}, values={'goods_name': '测试'})

    @WithDatabase('test')
    def test_insert(self):
        self.db.insert(table='test', values={'total_amt': 10, 'goods_name': 'test', 'goods_info': 'test', 'fee_ratio': 1.0, 'join_date': datetime.date.today(), 'create_time': datetime.datetime.now()}) 
        self.db.insert(table='test', values={'goods_name': 'test', 'goods_info': 'test', 'fee_ratio': 1.0, 'join_date': datetime.date.today(), 'create_time': datetime.datetime.now()}) 

        self.db.insert(table='test', values={'goods_name': '', 'goods_info': 'test'})

    @WithDatabase('test')
    def test_get(self):
        sql = 'select * from test where id=1' 
        print self.db.get(sql)
        sql = 'select * from test where id in(%s, %s, %s)'
        print self.db.get(sql, (1,2,3))
        sql = 'select * from test where create_time=%s'
        print self.db.get(sql, datetime.datetime.now())

    @WithDatabase('test')
    def thread_start(self):
        print threading._get_ident()
        time.sleep(1)

    def test_multi_thread(self):
        for i in range(0, 10):
            t = threading.Thread(target=self.thread_start)
            t.start()
        time.sleep(10)
            
    @WithDatabase('test')
    def test_query(self):
        sql = 'select * from test where id in (1,8,9)'
        print self.db.query(sql)
        sql = 'select * from test where id in (%s, %s, %s)'
        print self.db.query(sql, args=(1, 8, 9))

    @WithDatabase('test')
    def test_select_join(self):
        wheres = ({'goods_name': '测试'}, {'goods_name': '测试', 'total_amt': 1}) 
        relation = {'goods_name': 'goods_name', 'OR': {'fee': 'fee'}}
        print self.db.select_left_join(tables=('test', 'test1'), wheres=wheres, relation=relation)
        print self.db.select_left_join(tables=('test1', 'test'), wheres=wheres, relation=relation)
        print self.db.select_right_join(tables=('test1', 'test'), wheres=wheres, relation=relation)

dbconfig = {'test': 
                      {'master': 
                                {'db': 'test',
                                'host': '192.168.1.115',
                                'port': 3306,
                                'user': 'zs',
                                'passwd': '123456',
                                'charset': 'utf8',
                                'timeout': 10}, 
                        'conn': 1,
                        'slave': 
                                {'db': 'test',
                                'host': '192.168.1.115',
                                'port': 3306,
                                'user': 'zs',
                                'passwd': '123456',
                                'charset': 'utf8',
                                'timeout': 10}},
            'test1': 
                      {'master': 
                                {'db': 'test1',
                                'host': '192.168.1.115',
                                'port': 3306,
                                'user': 'zs',
                                'passwd': '123456',
                                'charset': 'utf8',
                                'timeout': 10}, 
                        'conn': 1,
                        'slave': 
                                {'db': 'test1',
                                'host': '192.168.1.115',
                                'port': 3306,
                                'user': 'zs',
                                'passwd': '123456',
                                'charset': 'utf8',
                                'timeout': 10}}

            }

dbconfig = {'test': {'db': 'test1',
                   'host': '192.168.1.115',
                   'port': 3306,
                   'user': 'zs',
                   'passwd': '123456',
                   'charset': 'utf8',
                   'timeout': 10,
                   'conn': 1},
                   }

install_database(dbconfig)

if __name__ == '__main__':
    t = Test()
    t.test_create_table()
    try:
        #t.test_insert()
        t.test_multi_thread()
    finally:
        t.test_drop_table()
