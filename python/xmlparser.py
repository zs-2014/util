#coding: utf-8

import os, sys
import xmltodict
import time 
'''
<xml><![CDATA[xxxx]<xml>
'''

class XMLFormatException(Exception):
    def __init__(self, msg):
        self.msg = msg 
        Exception.__init__(self, msg)

    def __str__(self):
        return 'XMLFormat Error:%s' % str(self.msg)

    def __repr__(self):
        return 'XMLFormat Error:%s' % str(self.msg)

class XmlDict(object):
    escape_map = {'&lt;': '<',
                  '&gt;': '>',
                  '&quot;': '"',
                  '&apos': "'"}

    def __init__(self, xml_str, encode=None):
        self.encode = encode
        self.xml_str = xml_str.strip()
        self.len = len(self.xml_str)
        self.pos = 0

    #忽略属性的解析
    def parse_key(self):
        #查找到结束的lab
        idx = self.xml_str.find('>', self.pos)
        if idx == -1:
            raise XMLFormatException('expected > at %d, but not found' % self.len)
        start_lab = None
        #提取key
        key_str = self.xml_str[self.pos:idx]
        self.pos = idx 

        key_str = key_str.strip()
        idx = key_str.find(' ')
        attr_str = ''
        if idx != -1:
            attr_str = key_str[idx:]
            key_str = key_str[:idx]
        start_lab = key_str
        nm_beg_idx = attr_str.find('name="')
        if nm_beg_idx != -1:
            nm_end_idx = attr_str.find('"', nm_beg_idx+6)
            if nm_end_idx == -1:
                raise XMLFormatException('expected " at %d, but not found' % (self.pos+len(key_str),))
            key_str = attr_str[nm_beg_idx+6:nm_end_idx]

        idx = key_str.find(',')
        if idx != -1:
            raise XMLFormatException('unexpected , at %s' % key_str)
        idx = key_str.find('=')
        if idx != -1:
            raise XMLFormatException('unexpected = at %s' % key_str)
        idx = key_str.find('<')
        if idx != -1:
            raise XMLFormatException('unexpected < at %s' % key_str)
        return self.escape(start_lab), self.escape(key_str)

    #1<key>value</key>
    #2<key><![CDATA[value]]></key>
    #3<key><sub_key>sub_value</sub_key></key>
    def parse_value(self):
        #3
        if self.xml_str.startswith('<', self.pos) and not self.xml_str.startswith('<![CDATA[', self.pos):
            if self.xml_str.startswith('</', self.pos):
                return ''
            return self.parse_xml()
        #1
        #不是以<![CDATA[ 开头
        if not self.xml_str.startswith('<![CDATA[', self.pos):
            idx = self.xml_str.find('</', self.pos) 
            if idx == -1:
                raise XMLFormatException('expected < at %d' % self.len)
            value_str = self.xml_str[self.pos:idx].strip()
            self.pos = idx
            return self.escape(value_str)
        #2
        self.pos += 9 #len(<![CDATA[) == 9
        #查找结尾
        end_idx = self.xml_str.find(']]>', self.pos)
        if end_idx == -1:
            raise XMLFormatException('unclosed CDATA at %d'% (self.pos))
        value_str = self.xml_str[self.pos:end_idx]
        self.pos = end_idx+3
        return value_str
            
    def parse_xml(self):
        self.skip(" \n\t")
        ret_dict = {} 
        while self.xml_str.startswith('<', self.pos) and not self.xml_str.startswith('</', self.pos):
            if self.xml_str.startswith('<!--', self.pos):
                idx = self.xml_str.find('-->', self.pos)
                if idx == -1:
                    raise XMLFormatException('expected the comment end lable -->')
                self.pos = idx + 3
                self.skip(" \n\t")
                continue
            #跳过开始的'<'
            self.pos += 1
            self.skip(" \n\t")
            #<xxx>
            start_lab, key = self.parse_key()
            #跳过'>'
            self.pos += 1
            self.skip(" \n\t")
            value = self.parse_value()
            self.skip(" \n\t")
            #</xxx>
            if not self.xml_str.startswith('</', self.pos):
                raise XMLFormatException('expected </ at %d' % self.pos) 
            #跳过</
            self.pos += 2
            end_idx = self.xml_str.find('>', self.pos)
            if end_idx == -1:
                raise XMLFormatException('expected > at %d' % self.pos)
            end_lab = self.xml_str[self.pos:end_idx].strip()
            if start_lab != end_lab:
                raise XMLFormatException('not found end lable for <%s>' % key)
            self.pos = end_idx+1
            self.skip(" \n\t")
            if key not in ret_dict:
                ret_dict[key] = value
                continue
            #相同的key的值放在一个list中
            else:
                if not isinstance(ret_dict[key], list):
                    v = ret_dict[key]
                    ret_dict[key] = [v]
                ret_dict[key].append(value)
        return ret_dict 

    def escape(self, text):
        for k, v in self.escape_map.iteritems():
            if k in text:
                text = text.replace(k, v)
        return text

    def skip(self, skip_str):
        while self.pos < self.len and self.xml_str[self.pos] in skip_str:
            self.pos += 1

    def to_dict(self):
        if not self.xml_str:
            raise XMLFormatException('invalid xml format')
        self.skip(" \n\t")
        #查找头，如果有的话，直接去掉
        if self.xml_str.startswith('<?', self.pos):
            self.pos = self.xml_str.find('?>', self.pos) 
            if self.pos == -1:
                raise XMLFormatException('expected ?> at xml header')
            self.pos += 2
        ret = self.parse_xml()
        if self.pos != self.len:
            raise XMLFormatException('invalid xml format')
        if len(ret) != 1:
            raise XMLFormatException('more than one root node')
        for k in ret:
            if isinstance(ret[k], list):
                raise XMLFormatException('more than one root node')
        return ret

def timeit(func):
    def __(*args, **kwargs):
        t1 = time.time()
        for x in xrange(1, 100000):
            func(*args, **kwargs)
        t2 = time.time()
        print int(t2*1000-t1*1000)
    return __

@timeit
def test_xmltodict(text):
    xmltodict.parse(text)   

@timeit
def test_xmlparser(text):
    XmlDict(text).to_dict()
    
if __name__ == '__main__':
    a = '<?xml version="1.0" ?><xml><!-- this is  a test --><amt type="dict"><!-- this is another test --><total_amt> <![CDATA[10 &gt; 9]]> </total_amt><point_amt>1 </point_amt><balance_amt></balance_amt><pay_amt>9&lt;30</pay_amt></amt><pay_seq><item> 1 </item><item>2</item><item>3</item><item1>5</item1></pay_seq><pay_type>1</pay_type><pay_source>1</pay_source></xml>'
    a = '<xml><appid name=""><![CDATA[wx2421b1c4370ec43b]]></appid><mch_id><![CDATA[10000100]]></mch_id><nonce_str><![CDATA[TeqClE3i0mvn3DrK]]></nonce_str><out_refund_no_0><![CDATA[1415701182]]></out_refund_no_0><out_trade_no><![CDATA[1415757673]]></out_trade_no><refund_count>1</refund_count><refund_fee_0>1</refund_fee_0><refund_id_0><![CDATA[2008450740201411110000174436]]></refund_id_0><refund_status_0><![CDATA[PROCESSING]]></refund_status_0><result_code><![CDATA[SUCCESS]]></result_code><return_code><![CDATA[SUCCESS]]></return_code><return_msg><![CDATA[OK]]></return_msg><sign><![CDATA[1F2841558E233C33ABA71A961D27561C]]></sign><transaction_id><![CDATA[1008450740201411110005820873]]></transaction_id></xml>'
    a = '''<?xml version="1.0" encoding="utf-8"?>
<alipay>
    <is_success>T</is_success>
    <request>
        <param name="sign">11ca4b84fa00d22235e16db56f97e0df</param>
        <param name="_input_charset">utf-8</param>
        <param name="sign_type">MD5</param>
        <param name="service">alipay.acquire.refund</param>
        <param name="out_request_no">6034517927594664774</param>
        <param name="partner">2088711407753777</param>
        <param name="refund_amount">0.01</param>
        <param name="out_trade_no">6034514044151505730</param>
        <param name="alipay_ca_request">2</param>
    </request>
    <response>
        <alipay>
        <buyer_logon_id>158****6591</buyer_logon_id>
        <buyer_user_id>2088612245064480</buyer_user_id>
        <fund_change>Y</fund_change>
        <gmt_refund_pay>2015-08-05 10:19:50</gmt_refund_pay>
        <out_trade_no>6034514044151505730</out_trade_no>
        <refund_detail_item_list>
            <TradeFundBill>
                <amount>0.01</amount>
                <fund_channel>ALIPAYACCOUNT</fund_channel>
            </TradeFundBill>
        </refund_detail_item_list>
        <refund_fee>0.01</refund_fee>
        <result_code>SUCCESS</result_code>
        <trade_no>2015080521001004480010854509</trade_no>
        </alipay>
    </response>
    <sign>51ac015aee25df861df07f59c0fbff82</sign>
    <sign_type>MD5</sign_type>
</alipay>'''
    #print XmlDict(a).to_dict()
    test_xmltodict(a)
    test_xmlparser(a)

