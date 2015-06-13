#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jsonstring.h"
#include "jsondict.h"
#include "jsonvalue.h"
#include "jsonarray.h"
#include "jsonreader.h"


extern JsonValueFunc json_value_func ;
extern JsonStringFunc json_string_func ;
extern JsonDictFunc   json_dict_func ;
extern JsonArrayFunc  json_array_func ;
extern JsonReaderFunc json_reader_func ;

static JsonValue *copy_string_value(JsonReader *json_reader) ;
static JsonString *copy_key(JsonReader *json_reader, JsonValue *json_value) ;
static JsonValue *parse_value(JsonReader *json_reader) ;
static JsonValue *parse_dict_object(JsonReader *json_reader) ;
static JsonValue *parse_boolean_object(JsonReader *json_reader) ;
static JsonValue *parse_string_object(JsonReader *json_reader) ;
static JsonValue *parse_null_object(JsonReader *json_reader) ;
static JsonValue *parse_number_object(JsonReader *json_reader) ;
static JsonValue *parse_array_object(JsonReader *json_reader) ;
static JsonValue * get_json_value(JsonReader *json_reader) ;
static int parse(JsonReader *json_reader) ;
static int is_occur_error(JsonReader *json_reader) ;

static int init_json_reader(JsonReader *json_reader, const char *buff, size_t sz) ;
static JsonReader *malloc_json_reader(const char *buff, size_t sz) ;
static void free_json_reader(JsonReader *json_reader) ;
static void deconstruct_json_reader(JsonReader *json_reader) ;

JsonReaderFunc json_reader_func = {.init = init_json_reader,
                                   .malloc = malloc_json_reader,
                                   .deconstruct = deconstruct_json_reader,
                                   .parse = parse,
                                   .is_error = is_occur_error,
                                   .get_json_value = get_json_value,
                                   .free = free_json_reader} ;


inline static int is_key_end(JsonReader *json_reader) ;

inline static int is_valid(JsonReader *json_reader)
{
    return json_reader && json_reader ->buff && json_reader ->total_sz > json_reader ->curr_pos ;
}

static int in(char ch, char *str)
{
    if(!str)
        return 0 ;
    while(*str != '\0')
        if(*str++ == ch)
            return 1 ;
    return 0 ;
}

static void skip_spaces(JsonReader *json_reader)
{
    while(is_valid(json_reader) && in(json_reader ->buff[json_reader ->curr_pos], "\t\n \r"))
        json_reader ->curr_pos++;
}

/*
 * 0000 - 007F 0xxxxxxx 
 * 0080 - 07FF 110xxxxx 10xxxxxx 
 * 0800 - FFFF 1110xxxx 10xxxxxx 10xxxxxx 
 */
static char *unicode_point_to_utf8(int cp, char *result)
{
    if(!result)
        return null ;
    if (cp <= 0x7f) 
    {
        result[0] = cp;
    } 
    else if (cp <= 0x7FF) 
    {
        result[1] = 0x80 | (0x3f & cp);
        result[0] = 0xC0 | (0x1f & (cp >> 6));
    } 
    else if (cp <= 0xFFFF) 
    {
        result[2] = 0x80 | (0x3f & cp);
        result[1] = 0x80 | (0x3f & (cp >> 6));
        result[0] = 0xE0 | (0xf & (cp >> 12));
    }
    else if (cp <= 0x10FFFF) 
    {
        result[3] = 0x80 | (0x3f & cp);
        result[2] = 0x80 | (0x3f & (cp >> 6));
        result[1] = 0x80 | (0x3f & (cp >> 12));
        result[0] = 0xF0 | (0x7 & (cp >> 18));
    }
    return result ;
}


#define hex(ch) (ch >= '0' && ch <= '9' ? ch - '0':\
                (ch >= 'a' && ch <= 'f' ? ch - 'a'+ 10:\
                (ch >= 'A' && ch <= 'F' ? ch - 'A' + 10: -1)))

static int to_unicode_value(JsonReader *json_reader)
{
    if(!is_valid(json_reader))
        return -1 ;
    //len("\\u0134") = 6
    if(json_reader ->total_sz < json_reader ->curr_pos + 6)
        return 0 ;
    //"\u"
    if(json_reader ->buff[json_reader ->curr_pos] != '\\' || json_reader ->buff[json_reader ->curr_pos+1] != 'u')
        return 0;
    int i = 2 ; 
    int num = 0 ;
    while(i < 6)
    {
        char ch = json_reader ->buff[json_reader ->curr_pos+i]  ; 
        char h = hex(ch) ;  
        if(h == -1)
            return -1 ;
        i++;
        num = (num << 4) + h ;
    }
    json_reader ->curr_pos += 6 ;
    return num ;
}

static int is_unicode_start(JsonReader *json_reader)
{
    if(!is_valid(json_reader)) 
        return 0 ;
    if(json_reader ->total_sz <= json_reader ->curr_pos + 1)
        return 0 ;
    return json_reader ->buff[json_reader ->curr_pos] == '\\' && json_reader ->buff[json_reader ->curr_pos+1] == 'u' ;
}

static int convert_unicode_str_to_utf8(JsonReader *json_reader, char *dst)
{
    if(!is_unicode_start(json_reader)) 
        return 0 ;
    int v = to_unicode_value(json_reader) ;
    if(v == -1)
        return 0 ;
    if(v >= 0xD800 && v <= 0xDBFF)
    {
        if(!is_unicode_start(json_reader))
            return 0;
        int surrogate_pair = to_unicode_value(json_reader) ;
        if(surrogate_pair == -1)
            return 0;
        v = 0x10000 + ((v & 0x3FF) << 10) + (surrogate_pair & 0x3FF); 
    }
    unicode_point_to_utf8(v, dst) ;
    return 1;
}

char escape(JsonReader *json_reader)
{
    if(!is_valid(json_reader))
        return 0 ;
    if(json_reader ->buff[json_reader ->curr_pos] != '\\')  
        return 0 ;
    json_reader ->curr_pos++ ;
    if(json_reader ->curr_pos > json_reader ->total_sz)
        return 0 ;
    char ch = json_reader ->buff[json_reader ->curr_pos++] ;
    switch(ch)
    {
        case '\\': return '\\' ;
        case '/' : return '/'  ;
        case '"':  return '"'  ; 
        case 't':  return '\t' ;
        case 'n':  return '\n' ;
        case 'r':  return '\r' ;
        case 'f':  return '\f' ;
        case 'b':  return '\b' ;
        default :  json_reader ->curr_pos -= 2; return -1;
    }
    return 0 ;
}


//拷贝key  
static JsonString *copy_key(JsonReader *json_reader, JsonValue *json_value)
{
    JsonString *json_string = null ;

    if(json_value)
        json_string = &json_value ->object.json_string ;
    else
        json_string = json_string_func.malloc(0);

    if(!json_string)
        return null ;
    while(is_valid(json_reader) && !is_key_end(json_reader))
    { 
        char ch = json_reader ->buff[json_reader ->curr_pos++] ;
        //转义字符
        if(ch == '\\')
        {
            json_reader ->curr_pos-- ;
            if(is_unicode_start(json_reader))
            {
                char tmp[32] = {0} ;
                if(!convert_unicode_str_to_utf8(json_reader, tmp))
                {
                    snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                             "illegal unicode string at position %d", json_reader ->curr_pos) ;
                    goto __fails ;
                }
                if(!json_string_func.append_string(json_string, tmp, strlen(tmp)))
                    goto __fails ;
                continue ;
            }
            else
            {
                ch = escape(json_reader) ;
                if(ch == -1)
                {
                    snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                             "unknown escape char at position %d", json_reader ->curr_pos) ; 
                    goto __fails ;
                }
            }
        }
        if(!json_string_func.append_char(json_string, ch))
            goto __fails ;
    }

    if(!is_valid(json_reader)) 
    {
        snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                 "expected \" at position %d", json_reader ->curr_pos) ;
        goto __fails ;
    }
    //skip "
    json_reader ->curr_pos++ ;
    //skip \t \n \r
    skip_spaces(json_reader) ;
    return json_string;

__fails:
    if(!json_value)
        json_string_func.free(json_string) ;
    else
        json_string_func.deconstruct(json_string) ;

    return null ;
}

//拷贝string value和拷贝string key是一样的
static JsonValue *copy_string_value(JsonReader *json_reader)
{
    JsonValue *json_value = json_value_func.malloc(STRING_VALUE_TYPE) ;
    if(!json_value)
        return null ;
   return copy_key(json_reader, json_value) ? json_value : null ; 
}

static int start_with(const char *s1, const char *s2)
{
    if(!s1 || !s2)
        return 0 ;
    while(*s1 != '\0' && *s2 != '\0')
     {
        if(*s1 != *s2) 
            return 0;
        s1++ ;
        s2++ ;
    }
    return (*s1 == '\0' && *s2 == '\0') || *s1 != '\0' ;
}

inline static int is_string_object_start(JsonReader *json_reader)
{
   return is_valid(json_reader) && json_reader ->buff[json_reader ->curr_pos] == '"' ;
}
inline static int is_string_object_end(JsonReader *json_reader)
{
    return is_valid(json_reader) && json_reader ->buff[json_reader ->curr_pos] == '"' ;
}

inline static int is_dict_object_start(JsonReader *json_reader)
{
    return is_valid(json_reader) && json_reader ->buff[json_reader ->curr_pos] == '{' ;
}

inline static int is_dict_object_end(JsonReader *json_reader)
{
   return is_valid(json_reader) && json_reader ->buff[json_reader ->curr_pos]  == '}' ;
}

static int is_null_object(JsonReader *json_reader)
{
    if(!is_valid(json_reader)) 
        return 0 ;
    return start_with(json_reader ->buff+json_reader ->curr_pos, "null") ;
}

static int is_boolean_object(JsonReader *json_reader)
{
    if(!is_valid(json_reader))
        return 0 ;
    char *buff = json_reader ->buff + json_reader ->curr_pos ;
    return start_with(buff, "true") || start_with(buff, "false") ;
}

static int is_number_object(JsonReader *json_reader)
{
    if(!is_valid(json_reader)) 
        return 0 ;
    char ch = json_reader ->buff[json_reader ->curr_pos] ;
    if(ch >= '0' && ch <= '9')
        return 1 ;
    if(ch == '-' || ch == '+')
    {
        //json_reader ->buff[json_reader ->total_sz] == '\0'
        ch = json_reader ->buff[json_reader ->curr_pos+1] ;
        return ch >= '0' && ch <= '9' ;
    }
    return 0 ;
}

inline static int is_array_object_start(JsonReader *json_reader)
{
    return is_valid(json_reader) && json_reader ->buff[json_reader ->curr_pos] == '[' ;
}

inline static int is_array_object_end(JsonReader *json_reader)
{
    return is_valid(json_reader) && json_reader ->buff[json_reader ->curr_pos] == ']' ;
}

inline static int is_key_start(JsonReader *json_reader)
{
    return is_valid(json_reader) && json_reader ->buff[json_reader ->curr_pos] == '"' ;
}

inline static int is_key_end(JsonReader *json_reader)
{
    return is_valid(json_reader) && json_reader ->buff[json_reader ->curr_pos] == '"' ;
}

static JsonValue *parse_value(JsonReader *json_reader)
{
    if(is_string_object_start(json_reader)) 
    {
        return parse_string_object(json_reader) ;
    }
    else if (is_dict_object_start(json_reader))
    {
        return parse_dict_object(json_reader) ;
    }
    else if(is_boolean_object(json_reader))
    {
        return parse_boolean_object(json_reader) ;
    }
    else if(is_null_object(json_reader))
    {
        return parse_null_object(json_reader) ; 
    }
    else if(is_number_object(json_reader))
    {
        return parse_number_object(json_reader) ; 
    }
    else if(is_array_object_start(json_reader))
    {
        return parse_array_object(json_reader) ;
    }
    if(is_valid(json_reader))
    {
        snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff), 
                "unknown value type at position %d", json_reader ->curr_pos) ;
    }
    return null ;
}

static int is_int64_over_flow(char *number)
{
    //9223372036854775807 == INT64_MAX
    //9223372036854775808 == INT64_MIN
    char *str[] = {"9223372036854775807", "9223372036854775808"} ;
    char *tmp = str[0] ;

    if(number[0] == '-')  
    {   
        number++;
        tmp = str[1] ;
    }
    else if(number[0] == '+')
        number++ ;

    while(*number == '0')
        number++ ;

    int len1 = strlen(tmp) ;
    int len2 = strlen(number) ;

    if(len1 < len2)
        return 1 ;
    else if(len1 > len2)
        return 0 ;

    while(*tmp != '\0' && *number != '\0')
    {
        if(tmp[0] < number[0])
            return 1 ;
        tmp++ ;
        number++ ;
    }
    return 0 ;
}


//解析数字对象
static JsonValue *parse_number_object(JsonReader *json_reader)
{
    //is_number_object保证最起码有一个数字存在
    if(!is_number_object(json_reader))
        return null ;
    char number[1024] = {0} ;
    number[0] = json_reader ->buff[json_reader ->curr_pos++] ;
    int i = 1 ;
    while(is_valid(json_reader))
    {
        char ch = json_reader ->buff[json_reader ->curr_pos++] ;
        if(in(ch, "eE0123456789.-+"))
        {   
            number[i++] = ch ;
        }
        else
        {    
            json_reader ->curr_pos-- ;
            break ;
        }
    }
    JsonValue *json_value = json_value_func.malloc(NUMBER_VALUE_TYPE) ;
    if(!json_value)
        return null ;
    number[i] = '\0' ;
    if(in('e', number) || in('E', number) || in('.', number))
    {
        char *endptr = null ;
        double v = strtod(number, &endptr) ;
        if(endptr != number + i)
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff), 
                    "illegal number at position %d", json_reader ->curr_pos + (endptr-number)) ; 
            json_value_func.free(json_value) ;
            return null ;
        }
        json_value ->type = DOUBLE_VALUE_TYPE ;
        json_value ->object.d = v ;
        return json_value ;
    }
    else
    {
        char *endptr = null ;
        int64_t v = (int64_t)strtoll(number, &endptr, 10) ;
        if(endptr != number + i)
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff), 
                    "illegal number at position %d", json_reader ->curr_pos + (endptr-number)) ; 
            json_value_func.free(json_value) ;
            return null ;
        }
        //判断数据是否溢出
        if(is_int64_over_flow(number))
        {
            if(number[0] != '-') 
                v = strtoll("9223372036854775807", null, 10) ; 
            else
                v = strtoll("-9223372036854775808", null, 10) ;
        }
        json_value ->type = INTEGER_VALUE_TYPE ;
        json_value ->object.i = v;
        return json_value ;
    }
    return null ;
}

//解析数组对象
//[null, "test1", {"xxx": "xxxxxx"}, true, false ]
static JsonValue *parse_array_object(JsonReader *json_reader)
{
    skip_spaces(json_reader) ;
    if(!is_array_object_start(json_reader))
        return null ; 
    //skip [
    json_reader ->curr_pos++ ;
    //skip \t \r ...
    skip_spaces(json_reader) ;
    JsonValue *json_value = json_value_func.malloc(ARRAY_VALUE_TYPE) ;
    if(!json_value)
        return null ;
    JsonArray *json_ay = &json_value ->object.json_array ;
    JsonValue *v = null ; 
    if(!json_ay)
        goto __fails ;
    int has_comma = -1 ;
    while(is_valid(json_reader) && !is_array_object_end(json_reader))
    {
        if(has_comma == 0)
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                     "expected ',' at position %d", json_reader ->curr_pos) ;
            goto __fails ;
        }
        //拷贝每一个值
        v = parse_value(json_reader) ;
        if(!v)
            goto __fails;
        skip_spaces(json_reader) ;
        if(json_reader ->buff[json_reader ->curr_pos] == ',')
        {
            json_reader ->curr_pos++ ;
            skip_spaces(json_reader) ;
            has_comma = 1 ;
        }
        else
        {
            has_comma = 0 ;
        }
        if(!json_array_func.append(json_ay, v))
            goto __fails ;
        v = null ;
    }
    
    if(!is_array_object_end(json_reader) || has_comma == 1)
    {
        if(has_comma == 1)
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                    "unexpected ',' near position %d", json_reader ->curr_pos) ;
        }
        else
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                    "expected ']' at positin:%d", json_reader ->curr_pos) ;
        }
        goto __fails ;
    }
    //skip ]
    json_reader ->curr_pos++ ;
    return json_value ; 

__fails:
    if(json_value)
        json_value_func.free(json_value) ;
    if(v)
        json_value_func.free(v) ;
    return null ;
}

static JsonValue *parse_boolean_object(JsonReader *json_reader)
{
    if(!is_boolean_object(json_reader))
        return null ;
    //len("true") == 4 len("false") == 5
    if(start_with(json_reader ->buff + json_reader ->curr_pos, "false"))
    {
        json_reader ->curr_pos += 5 ;
        return get_false_json_value() ;
    }
    else
    {
        json_reader ->curr_pos += 4 ;
        return get_true_json_value() ;
    }
}
static JsonValue *parse_null_object(JsonReader *json_reader)
{
    if(!is_null_object(json_reader))
        return null ;
    //len(null) == 4
    json_reader ->curr_pos += 4 ; 
    return get_null_json_value() ;
    //return json_value_func.malloc(NULL_VALUE_TYPE) ;
}

static JsonValue *parse_string_object(JsonReader *json_reader)
{
    if(!is_string_object_start(json_reader)) 
        return null ;
    //跳过开始的 "
    json_reader ->curr_pos++ ;
    JsonValue *json_value = copy_string_value(json_reader);
    if(!json_value)
        return null ;

    return json_value ;
}

static JsonValue *parse_dict_object(JsonReader *json_reader)
{
    skip_spaces(json_reader) ;
    if(!is_valid(json_reader)) 
        return null ;
    //find dict start token '{'
    if(!is_dict_object_start(json_reader))
        return null ;
    //skip '{'
    json_reader ->curr_pos++ ; 
    //skip space \t
    skip_spaces(json_reader)  ;  
    JsonValue *json_value = json_value_func.malloc(DICT_VALUE_TYPE) ;
    if(!json_value)
        return null ;
    JsonDict *json_dict = &json_value ->object.json_dict ;
    JsonString *key = null ;
    JsonValue *value = null ;
    int has_comma = -1 ;
    while(is_valid(json_reader) && !is_dict_object_end(json_reader))
    {
        //find the key start token '"'
        if(!is_key_start(json_reader))
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                     "expected a \" at position %d", json_reader ->curr_pos) ;
            goto __fails;
        }
        //skip '"'
        json_reader ->curr_pos++ ;
        if(!has_comma)
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                     "expected a ',' at position %d", json_reader ->curr_pos) ;
            goto __fails ;
        }
        //拷贝key
        key = copy_key(json_reader, null) ;
        if(!key)
            goto __fails ;
        if(!is_valid(json_reader))
            goto __fails ;
        //找到 ':'
        if(json_reader ->buff[json_reader ->curr_pos] != ':')
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                     "expected a ':' at position %d", json_reader ->curr_pos) ;
            goto __fails ;
        }

        //跳过 ':'
        json_reader ->curr_pos++ ;
        //除去跟在 ':'之后的空格
        skip_spaces(json_reader) ;
        value = parse_value(json_reader) ;
        if(!value)
            goto __fails ;

        json_dict_func.set(json_dict, key, value) ;
        skip_spaces(json_reader) ;
        //每对key_value后都跟着个 ','
        if(json_reader ->buff[json_reader ->curr_pos] == ',')
        {    
            json_reader ->curr_pos++ ;
            skip_spaces(json_reader) ;
            has_comma = 1; 
        }
        else
        {
            has_comma = 0;
        }
        key = null ;
        value = null ;
    }

    //最后一对key-value不能再跟着一个 ','
    if(!is_valid(json_reader) || has_comma == 1)
    {
        if(has_comma == 1)
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                    "unexpected ',' near position %d", json_reader ->curr_pos) ; 
        }
        else
        {
            snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                     "expected '}' at position %d", json_reader ->curr_pos) ; 
        }
        goto __fails ;
    }

    //跳过结尾的 '}'
    json_reader ->curr_pos++;
    return json_value;

__fails:
    if(key)
        json_string_func.free(key) ;
    if(value)
        json_value_func.free(value) ;
    if(json_value)
        json_value_func.free(json_value) ;
    return null ;
}

static int init_json_reader(JsonReader *json_reader, const char *buff, size_t sz)
{
    if(!buff || !sz) 
        return 0 ;

    json_reader ->buff = (char *)calloc(1, sizeof(char)*sz) ;
    if(!json_reader ->buff)
        return 0;
    json_reader ->json_value = null ;
    json_reader ->total_sz = sz ; 
    json_reader ->curr_pos = 0 ;
    memcpy(json_reader ->buff, buff, sz) ;
    memset(json_reader ->errbuff, 0, sizeof(json_reader ->errbuff)) ;
    return 1;
}

static JsonValue * get_json_value(JsonReader *json_reader)
{
    if(!json_reader_func.is_error(json_reader))
        return json_reader ->json_value ;
    return null ;
}

static int is_occur_error(JsonReader *json_reader)
{
    return json_reader == null || (json_reader ->json_value == null && json_reader ->errbuff[0] != '\0') ;
}

static void deconstruct_json_reader(JsonReader *json_reader)
{
    if(!json_reader)
        return ;
    if(json_reader ->buff)
        free(json_reader ->buff) ; 
    if(json_reader ->json_value)
        json_value_func.deconstruct(json_reader ->json_value) ;
    json_reader ->buff = null ;
    json_reader ->total_sz = 0 ;
    json_reader ->curr_pos = 0 ;
    memset(json_reader ->errbuff, 0, sizeof(json_reader ->errbuff)) ;
}

static void free_json_reader(JsonReader *json_reader)
{
    if(!json_reader)
        return ;
    json_reader_func.deconstruct(json_reader) ; 

    if(json_reader ->json_value)
        free(json_reader ->json_value) ;
    free(json_reader) ;
}

static JsonReader *malloc_json_reader(const char *buff, size_t sz)
{
    if(!buff || !sz)
        return null ;

    JsonReader *json_reader = (JsonReader *)calloc(1, sizeof(JsonReader)) ;
    if(!json_reader)
        return null ;
    if(!json_reader_func.init(json_reader, buff, sz))
    {
        json_reader_func.free(json_reader) ;
        return null ;
    }
    return json_reader ;
}

static int parse(JsonReader *json_reader)
{
    if(!json_reader)
        return 0;
    json_reader ->json_value = parse_value(json_reader) ;
    skip_spaces(json_reader) ;
    if(is_valid(json_reader))
    {
        snprintf(json_reader ->errbuff, sizeof(json_reader ->errbuff),
                "unexpected charactor at %d", json_reader ->curr_pos) ;
        return 0 ;
    }
    return 1 ;
}
void print_json_object(JsonValue *json_value, int nspaces) ;

int main(int argc, char *argv[])
{
    if(argc < 2 )
    {
        printf("请输入测试参数\n") ;
        return -1 ;
    }
    char *dict_str = "{\"test\": 1, \"test1\": \"2\\t\", \"test\": {\"test1\": 1, \"test2\": true}, \"test4\": [null, true, false, 1, \"xxx\", {\"test5:\": 1}]}" ;
    dict_str = argv[1] ;
    int count = atoi(argv[2]) ;
    int i = 0 ;
    struct timeval t1 ;
    struct timeval t2 ;
    gettimeofday(&t1) ;
    printf("your input is:[%s]\ncount = %d\n", argv[1], count) ;
    for(i=0; i < count; i++)
    {
        JsonReader *json_reader = json_reader_func.malloc(dict_str, strlen(dict_str)) ;
        json_reader_func.parse(json_reader) ;
        if(json_reader_func.is_error(json_reader))
            print_json_object(json_reader ->json_value, 0) ;
        else
            printf("errormsg: [%s]\n", json_reader ->errbuff) ;

        json_reader_func.free(json_reader) ;
    }
    gettimeofday(&t2, NULL) ;
    int64_t diff = (t2.tv_sec*1000000 + t2.tv_usec - t1.tv_sec*1000000 - t1.tv_usec)/1000;
    printf("timediff = %ld ms\n", diff) ;
    return 0 ;
}
