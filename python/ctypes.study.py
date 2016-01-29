#coding: utf-8

#https://docs.python.org/2/library/ctypes.html#calling-functions-continued
#为了方便，导入所有
from ctypes import *

#ctypes 类型对照表
'''
---------------------------------------------------------------------------------------------|
|ctypes type     |     C type                                 |   Python type
|----------------|--------------------------------------------|------------------------------|
|c_bool          |     _Bool                                  |   bool (1)
|----------------|--------------------------------------------|------------------------------|
|c_char          |     char                                   |   1-character string
|----------------|--------------------------------------------|------------------------------|
|c_wchar         |     wchar_t                                |   1-character unicode string
|----------------|--------------------------------------------|------------------------------|
|c_byte          |     char                                   |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_ubyte         |     unsigned char                          |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_short         |     short                                  |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_ushort        |     unsigned short                         |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_int           |     int                                    |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_uint          |     unsigned int                           |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_long          |     long                                   |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_ulong         |     unsigned long                          |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_longlong      |     __int64 or long long                   |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_ulonglong     |     unsigned __int64 or unsigned long long |   int/long
|----------------|--------------------------------------------|------------------------------|
|c_float         |     float                                  |   float
|----------------|--------------------------------------------|------------------------------|
|c_double        |     double                                 |   float
|----------------|--------------------------------------------|------------------------------|
|c_longdouble    |     long double                            |   float
|----------------|--------------------------------------------|------------------------------|
|c_char_p        |     char * (NUL terminated)                |   string or None
|----------------|--------------------------------------------|------------------------------|
|c_wchar_p       |     wchar_t * (NUL terminated)             |   unicode or None
|----------------|--------------------------------------------|------------------------------|
|c_void_p        |     void *                                 |   int/long or None
|----------------|--------------------------------------------|------------------------------|
'''
##python 中的N    one对应于c/c++中的NULL

def load_library():
    libc1 = CDLL('libc.so.6')
    libc2 = cdll.LoadLibrary('libc.so.6')

    #调用库中的函数可以通过属性的方式，也可以通过字典索引的方式
    print "通过CDLL类加载C库 CDLL('libc.so.6'): ret=%s" % libc1
    print "通过cdll.LoadLibrary来加载C库 cdll.LoadLibrary('libc.so.6') ret=%s" % libc2

def call_function_from_loaded_library():
    libc = CDLL('libc.so.6')
    #调用库中的函数可以通过属性的方式，也可以通过字典索引的方式
    libc.printf("libc.printf() 通过属性得方式访问库中的函数 library.funcName\n")
    libc['printf']("libc['print']通过字典索引的方式访问库中的函数 library[funcName]\n")

def call_function_with_argument():
    #当向c function传入参数时，除了integer，string，unicode strings不需要使用ctypes中的类型构造外
    #其余的类型都需要使用ctypes中的类来构造
    #注意: float和double不属于integer的范畴
    libc = CDLL('libc.so.6')
    libc.argtypes = (c_char_p, c_double, c_char_p)
    libc.printf("调用libc.so.6中的printf函数参数为:c_int(%d) float(%f) c_char_p(%s)\n", c_int(1), c_double(3.14), c_char_p("test"))
    try:
        libc.printf("%f", 3.14)
    except ArgumentError, arger:
        print "float和double不属于python的integer的范畴"

    #对于自定义的类型，其类型以及其表大的具体C类型值都是通过_as_parameter_这个属性来表示的
    class CustomTypeWith_as_parameter(object):
        def __init__(self, s):
            print "CustomeType.__init__"
            self._as_parameter_ = s
    libc.printf("自定义类型作为C函数的参数: %s\n", CustomTypeWith_as_parameter("custom type with _as_parameter"))
     
    class CustomTypeWithNo_as_parameter(object):
        def __init__(self, s):
            self.s = s
    try:
        libc.printf("自定义类型作为C函数的参数: %s\n", CustomTypeWithNo_as_parameter("custom type with no _as_parameter"))
    except Exception, e:
        print str(e)

    #对于传入c参数的类型，我们可以使用argtypes属性来规定
    printf = libc.printf
    printf.argtypes = (c_char_p, c_int, c_double)
    printf("argstypes(c_char_p, c_int, c_double) Int:%d Double:%f\n", 1, 3.14)

    #对应自定义的类型来说,其类必须实现from_param函数
    class ArgumentChecker1(object):
        valid_types=(int, str, c_int, c_char_p, c_double)
        @staticmethod
        def from_param(obj):
            print "ArgumentChecker1.from_param"
            if type(obj) not in ArgumentChecker1.valid_types:
                raise TypeError("not support type:%s" % type(obj))
            return obj
    class ArgumentChecker2(object):
        valid_types=(int, str, c_int, c_char_p, c_double)
        @staticmethod
        def from_param(obj):
            print "ArgumentChecker2.from_param"
            if type(obj) not in ArgumentChecker2.valid_types:
                raise TypeError("not support type:%s" % type(obj))
            return obj
    #对于传入的前三个参数,每个参数都会调用相应类的ArgumentChecker.from_param,
    #from_param的参数就是c函数相应位置的参数值，
    #注意，这里只能限制类型，对应超过argtypes个数的参数，无法检测
    #对应from_param的返回值，可以返回拥有_as_parameter_属性的对象
    #或者python内置的integer,string, unicode string
    #亦或是ctypes中的类型
    #当传入函数中的参数个数少于argtypes的长度时，会抛出异常
    printf.argtypes = (ArgumentChecker1, ArgumentChecker2, ArgumentChecker1)
    printf("argtype(ArgumentChecker, ArgumentChecker, ArgumentChecker) Int:%d, c_int:%d string:%s c_char_p:%s\n", 10, c_int(10), "test", c_char_p("test"))


    #对应调用c api的返回值，则统一是int类型。
    #我们通过设置c函数的restype类改变这一点，restype可以是一个任何一个callable对象
    printf.argtypes = ( )
    strchr = libc.strchr
    printf("没有设置restype的时候返回的是一个值：%p\n", strchr("abc", c_char('c')))
    strchr.restype = c_char_p
    printf("设置了restype类型为c_char_p类型，返回的值为:%s\n", strchr('abc', c_char('c')))
    def check_return_value(v):
        if v == 0 or v is None:
            #raise ValueError("not found")
            return "not found"
        return v
    strchr.restype = check_return_value
    printf("设置了restype为一个callable对象，返回的值为:%d\n", strchr('abc', c_char('c')))
    printf("设置了restype为一个callable对象，返回的值为:%s\n", strchr('abc', c_char('d')))

    #byref()函数传递参数给c api函数时，是通过传引用的，也就是说
    #c api函数对参数的值是可以修改的 byref的参数必须是ctypes类型的
    i = c_int()
    d = c_double()
    s = create_string_buffer(128)
    libc.sscanf("1 3.14 hello,create_string_buff", "%d %f %s", byref(i), byref(d), s)
    #从输出结果来看，python对double和float不能很好的处理！
    print "i:%d f:%f s:%s\n" % (i.value, d.value, s.value)


    #传递structure必须继承自Structure
    #这个structure可以是其他的struct组合来的
    class TimeVal(Structure):
        _fields_ = [("tv_sec", c_uint),
                    ("tv_usec", c_ulong)]
    c = TimeVal()
    #pointer就是构建一个 c 指针
    libc.gettimeofday(pointer(c), None)
    print c.tv_sec, c.tv_usec
            
if __name__ == '__main__':
    load_library()
    call_function_from_loaded_library() 
    call_function_with_argument()
