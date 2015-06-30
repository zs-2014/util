#coding: utf-8
import types


class XmlDict(object):
    def __init__(self, encode='utf-8'):
        self.encode = encode

    def __list__value__(self, v):
        xml_value_str_list = [] 
        for e in v:
            xml_value_str_list.append('<item>') 
            xml_value_str_list.append(self.__parse_value__(e)) 
            xml_value_str_list.append('</item>')
        return ''.join(xml_value_str_list)
            
    def __parse_value__(self, v):
        if isinstance(v, types.UnicodeType): 
            return '<![CDATA[%s]]>' % v.encode('utf-8')
        elif isinstance(v, dict):
            return self.__convert_dict_to_xml__(v)
        elif isinstance(v, (list, tuple)):
            return self.__list__value__(v) 
        elif v is None:
            return '<![CDATA[]]>'
        else:
            return '<![CDATA[%s]]>' % str(v)

    def __parse_key__(self, key):
        if isinstance(key, types.UnicodeType):
            return key.encode('utf-8')
        else:
            return str(key)
        
    def __convert_dict_to_xml__(self, params):
        xml_str_list = [] 
        for k, v in params.iteritems():
            k = self.__parse_key__(k)
            xml_str_list.append('<%s>' % k)
            xml_str_list.append(self.__parse_value__(v))
            xml_str_list.append('</%s>' % k)

        return ''.join(xml_str_list)

    #将一个字典转化成xml
    def convert_dict_to_xml(self, params):
        xml_str_list = []
        xml_str_list.append(self.__convert_dict_to_xml__(params)) 
        if self.encode is None:
            return ''.join(xml_str_list).decode('utf-8')
        else:
            return ''.join(xml_str_list).decode('utf-8').encode(self.encode)

if __name__ == '__main__':
    params = {'test': 'a', 'test1': 1, "test234": [1,2,3,4,5], 'None': None} 
    params['dict'] = {'xxxxxxxxxxx': 'bbbbbbbbbbbbb', u'马达': '测试'}
    xml_str = XmlDict().convert_dict_to_xml({'xml': params})
    print xml_str
    import xmltodict
    rsp_dict = xmltodict.parse(xml_str)
    for k, v in rsp_dict['xml'].iteritems():
        print k, ':', v
