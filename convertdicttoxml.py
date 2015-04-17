#coding: utf-8
import types

class XML(object):
    start_root = '<?xml version="1.0" encoding="%s" ?><root>'
    end_root = '</root>'
    def __init__(self, root=False, attr_type=False, encode='utf-8'):
        self.root = root
        self.attr_type = attr_type
        self.encode = encode

    def _get_type(self, v):
        if isinstance(v, int):
            return 'int'
        return v.__class__.__name__

    def get_type(self, v):
        if self.attr_type:
            return 'type="%s"' % self._get_type(v)
        else:
            return self._get_type(v)

    def get_start_element(self, k, v):
        if self.attr_type:
            return '<%s %s>' % (str(k), self.get_type(v)) 
        else:
            return '<%s>' % str(k)
    def get_start_item(self, e):
        if self.attr_type:
            return '<item %s>' % self.get_type(e)
        else:
            return '<item>'

    def get_end_item(self):
        return '</item>'

    def get_end_element(self, k):
        return '</%s>' % str(k)

    def __list__value__(self, v):
        xml_value_str_list = [] 
        for e in v:
            xml_value_str_list.append(self.get_start_item(e)) 
            xml_value_str_list.append(self.__parse_value__(e)) 
            xml_value_str_list.append(self.get_end_item())
        return ''.join(xml_value_str_list)
            
    def __parse_value__(self, v):
        xml_val_str_list = [] 
        if isinstance(v, types.UnicodeType): 
            return v.encode('utf-8')
        elif isinstance(v, dict):
            return self.__convert_dict_to_xml__(v)
        elif isinstance(v, list):
            return self.__list__value__(v) 
        else:
            return str(v)

    def __parse_key__(self, key):
        if isinstance(key, types.UnicodeType):
            return key.encode('utf-8')
        else:
            return str(key)
        
    def __convert_dict_to_xml__(self, params):
        xml_str_list = [] 
        for k, v in params.iteritems():
            k = self.__parse_key__(k)
            xml_str_list.append(self.get_start_element(str(k), v))
            xml_str_list.append(self.__parse_value__(v))
            xml_str_list.append(self.get_end_element(str(k)))

        return ''.join(xml_str_list)

    #将一个字典转化成xml
    def convert_dict_to_xml(self, params):
        xml_str_list = [] 

        if self.root:
            xml_str_list.append(XML.start_root % ('UNICODE' if self.encode is None else self.encode.upper(), ))

        xml_str_list.append(self.__convert_dict_to_xml__(params)) 

        if self.root:
            xml_str_list.append(XML.end_root)
        if self.encode is None:
            return ''.join(xml_str_list).decode('utf-8')
        else:
            return ''.join(xml_str_list).decode('utf-8').encode(self.encode)

if __name__ == '__main__':
    params = {'test': 'a', 'test1': 1, 'zs': [1, 2, 3, 4]} 
    params['dict'] = {'xxxxxxxxxxx': 'bbbbbbbbbbbbb', u'马达': u'测试'}
    print  XML(root=True, attr_type=False, encode='utf-8').convert_dict_to_xml({'xml': params})
    #xml_str = XML(root=True, attr_type=True).convert_dict_to_xml(params)
    #print xml_str
    #import xmltodict
    #print xmltodict.parse(xml_str)
