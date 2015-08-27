#coding: utf-8
import types

#输入的dict必须都是unicode编码

class XmlDict(object):
    def __init__(self, root=True, custom_root=None, encode='utf-8'):
        self.encode = encode 
        self.root = root
        self.custom_root = u'root' if custom_root is None else custom_root

    def unparse_list(self, v):
        xml_list_value = [] 
        for e in v:
            xml_list_value.append(u'<item>') 
            xml_list_value.append(self.unparse_value(e)) 
            xml_list_value.append(u'</item>')
        return u''.join(xml_list_value)
            
    def unparse_value(self, v):
        if isinstance(v, types.UnicodeType): 
            return u'<![CDATA[%s]]>' % v
        elif isinstance(v, dict):
            return self.unparse_dict(v)
        elif isinstance(v, (list, tuple)):
            return self.unparse_list(v) 
        elif v is None:
            return u'<![CDATA[]]>'
        else:
            return u'<![CDATA[%s]]>' % v

    def unparse_dict(self, d):
        xml_str_list = [] 
        for k, v in d.iteritems():
            xml_str_list.append(u'<%s>' % k)
            xml_str_list.append(self.unparse_value(v))
            xml_str_list.append(u'</%s>' % k)
        return u''.join(xml_str_list)

    #将一个字典转化成xml
    def convert_dict_to_xml(self, d):
        xml_str_list = []
        xml_str_list.append(u'<?xml version="1.0" encoding="%s" ?>' % self.encode)
        if self.root:
            xml_str_list.append(u'<%s>' % self.custom_root)
        xml_str_list.append(self.unparse_dict(d)) 
        if self.root:
            xml_str_list.append(u'</%s>' % self.custom_root)
        s = u''.join(xml_str_list)
        if self.encode is not None:
            return s.encode(self.encode) 
        else:
            return s.encode('utf-8')

def test1():
    params = {'test': 'a', 'test1': 1, "test234": [1,2,3,4,5], 'None': None} 
    params['dict'] = {'xxxxxxxxxxx': 'bbbbbbbbbbbbb', u'马达': u'测试'}
    xml_str = XmlDict(root=True, custom_root='xml').convert_dict_to_xml(params)
    print xml_str
    import xmltodict
    rsp_dict = xmltodict.parse(xml_str)
    for k, v in rsp_dict['xml'].iteritems():
        print k, ':', v
def test2():
    params = {'test': 'a', 'test1': 1, "test234": [1,2,3,4,5], 'None': None} 
    params['dict'] = {'xxxxxxxxxxx': 'bbbbbbbbbbbbb', 'testxxxxx': u'测试'}
    xml_str = XmlDict(root=True, custom_root='xml', encode='gb2312').convert_dict_to_xml(params)
    print xml_str
    import xmltodict
    #import xmlparser
    #rsp_dict = xmlparser.XmlDict(xml_str).to_dict()
    rsp_dict = xmltodict.parse(xml_str.decode('gb2312'))
    for k, v in rsp_dict['xml'].iteritems():
        print k, ':', v
if __name__ == '__main__':
    test1()
    test2()
