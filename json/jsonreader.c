#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "jsonstring.h"
#include "jsondict.h"
#include "jsonobject.h"
#include "jsonarray.h"
#include "jsonreader.h"

extern JsonObjectFunc json_object_func ;
extern JsonStringFunc json_string_func ;
extern JsonDictFunc   json_dict_func ;
extern JsonArrayFunc  json_array_func ;

static JsonString *copy_string_value(JsonReader *json_reader) ;
static JsonString *copy_key(JsonReader *json_reader) ;
static JsonObject *parse_value(JsonReader *json_reader) ;
static JsonObject *parse_dict_object(JsonReader *json_reader) ;
static JsonObject *parse_boolean_object(JsonReader *json_reader) ;
static JsonObject *parse_string_object(JsonReader *json_reader) ;
static JsonObject *parse_null_object(JsonReader *json_reader) ;
static JsonObject *parse_number_object(JsonReader *json_reader) ;
static JsonObject *parse_array_object(JsonReader *json_reader) ;

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
        return nullptr ;
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


static JsonString *copy_key(JsonReader *json_reader)
{
    JsonString *json_string = json_string_func.malloc(0);
    if(!json_string)
        return nullptr ;

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
                    goto __fails ;
                if(!json_string_func.append_string(json_string, tmp, strlen(tmp)))
                    goto __fails ;
                continue ;
            }
            else
            {
                ch = escape(json_reader) ;
                if(ch == -1)
                    goto __fails ;
            }
        }
        if(!json_string_func.append_char(json_string, ch))
            goto __fails ;
    }

    if(!is_valid(json_reader)) 
        goto __fails ;
    //skip "
    json_reader ->curr_pos++ ;
    //skip \t \n \r
    skip_spaces(json_reader) ;
    return json_string;

__fails:
    json_string_func.free(json_string) ;
    return nullptr ;
}

static JsonString *copy_string_value(JsonReader *json_reader)
{
   return copy_key(json_reader) ; 
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

static JsonObject *parse_value(JsonReader *json_reader)
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
    return nullptr ;
}

static JsonObject *parse_number_object(JsonReader *json_reader)
{
    //is_number_object保证最起码有一个数字存在
    if(!is_number_object(json_reader))
        return nullptr ;
    char number[1024] = {0} ;
    number[0] = json_reader ->buff[json_reader ->curr_pos++] ;
    int i = 1 ;
    while(is_valid(json_reader))
    {
        char ch = json_reader ->buff[json_reader ->curr_pos++] ;
        if(in(ch, "eE0123456789.-"))
        {   
            number[i++] = ch ;
        }
        else
        {    
            json_reader ->curr_pos-- ;
            break ;
        }
    }
    JsonObject *json_object = json_object_func.malloc(NUMBER_OBJECT_TYPE) ;
    if(!json_object)
        return nullptr ;
    number[i] = '\0' ;
    if(in('e', number) || in('E', number) || in('.', number))
    {
        char *endptr = nullptr ;
        double v = strtod(number, &endptr) ;
        if(endptr != number + i)
        {
            json_object_func.free(json_object) ;
            return nullptr ;
        }
        json_object ->type = DOUBLE_OBJECT_TYPE ;
        json_object ->object.d = v ;
        return json_object ;
    }
    else
    {
        char *endptr = nullptr ;
        int64_t v = (int64_t)strtoll(number, &endptr, 10) ;
        if(endptr != number + i)
        {
            json_object_func.free(json_object) ;
            return nullptr ;
        }
        json_object ->type = INTEGER_OBJECT_TYPE ;
        json_object ->object.i = v;
        return json_object ;
    }
    return nullptr ;
}

static JsonObject *parse_array_object(JsonReader *json_reader)
{
    skip_spaces(json_reader) ;
    if(!is_array_object_start(json_reader))
        return nullptr ; 
    //skip [
    json_reader ->curr_pos++ ;
    //skip \t \r ...
    skip_spaces(json_reader) ;
    JsonObject *json_object = json_object_func.malloc(ARRAY_OBJECT_TYPE) ;
    if(!json_object)
        return nullptr ;
    JsonArray *json_ay = json_array_func.malloc(0) ;
    JsonObject *v = nullptr ; 
    if(!json_ay)
        goto __fails ;
    int has_comma = -1 ;
    //[null, "test1", {"xxx": "xxxxxx"} ]
    while(is_valid(json_reader) && !is_array_object_end(json_reader))
    {
        if(has_comma == 0)
            goto __fails ;
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
        v = nullptr ;
    }

    if(!is_array_object_end(json_reader) || has_comma == 1)
        goto __fails ;
    json_object ->object.object = json_ay ;
    //skip ]
    json_reader ->curr_pos++ ;
    return json_object ; 

__fails:
    if(json_object)
        json_object_func.free(json_object) ;
    if(v)
        json_object_func.free(v) ;
    if(json_ay)
        json_array_func.free(json_ay) ;
    return nullptr ;
}

static JsonObject *parse_boolean_object(JsonReader *json_reader)
{
    if(!is_boolean_object(json_reader))
        return nullptr ;
    JsonObject *json_object = json_object_func.malloc(BOOLEAN_OBJECT_TYPE) ;
    if(!json_object)
        return nullptr ;
    int is_true = 1 ;
    if(start_with(json_reader ->buff + json_reader ->curr_pos, "false"))
        is_true = 0 ; 
    json_object ->object.b = is_true;
    //len("true") == 4 len("false") == 5
    json_reader ->curr_pos += is_true ? 4 : 5 ;
    return json_object ;
}
static JsonObject *parse_null_object(JsonReader *json_reader)
{
    if(!is_null_object(json_reader))
        return nullptr ;
    //len(null) == 4
    json_reader ->curr_pos += 4 ; 
    return json_object_func.malloc(NULL_OBJECT_TYPE) ;
}

static JsonObject *parse_string_object(JsonReader *json_reader)
{
    if(!is_string_object_start(json_reader)) 
        return nullptr ;
    JsonObject *json_object = json_object_func.malloc(STRING_OBJECT_TYPE) ;
    if(!json_object)
        return nullptr ;
    json_object ->object.object = nullptr ;
    //跳过开始的 "
    json_reader ->curr_pos++ ;
    JsonString *value = copy_string_value(json_reader);
    if(!value)
    {
        json_object_func.free(json_object); 
        return nullptr ;
    }
    json_object ->object.object = value ;
    return json_object ;
}

/*{"key": "value"}
1首先删除{前面的空格
2判断是不是{
3跳过{字符
4删除{后面的空格
5判断是不是一个"
6跳过"
7拷贝key,直到"
8跳过"
9跳过空格
10拷贝value
11跳过空格
12判断是不是,
13如果是,则跳过,和,后面跟的空格
*/

static JsonObject *parse_dict_object(JsonReader *json_reader)
{
    skip_spaces(json_reader) ;
    if(!is_valid(json_reader)) 
        return nullptr ;
    //find dict start token '{'
    if(!is_dict_object_start(json_reader))
        return nullptr ;
    //跳过 '{'
    json_reader ->curr_pos++ ; 
    //跳过空格
    skip_spaces(json_reader)  ;  
    JsonDict *json_dict = json_dict_func.malloc(0);
    if(!json_dict)
        return nullptr ;
    JsonObject *json_object = json_object_func.malloc(DICT_OBJECT_TYPE) ;
    if(!json_object)
        return nullptr ;
    JsonString *key = nullptr ;
    JsonObject *value = nullptr ;
    int has_comma = -1 ;
    while(is_valid(json_reader) && !is_dict_object_end(json_reader))
    {
        //find the key start token '"'
        if(!is_key_start(json_reader))
            goto __fails;
        //skip '"'
        json_reader ->curr_pos++ ;
        if(!has_comma)
            goto __fails ;
        //拷贝key
        key = copy_key(json_reader) ;
        if(!key)
            goto __fails ;
        if(!is_valid(json_reader))
            goto __fails ;
        //找到 ':'
        if(json_reader ->buff[json_reader ->curr_pos] != ':')
            goto __fails ;

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
        key = nullptr ;
        value = nullptr ;
    }

    //最后一对key-value不能再跟着一个 ','
    if(!is_dict_object_end(json_reader) || has_comma == 1)
    {
        json_dict_func.free(json_dict) ;
        return nullptr ;
    }
    //跳过结尾的 '}'
    json_reader ->curr_pos++;
    json_object ->object.object = json_dict ;
    return json_object;

__fails:
    if(key)
        json_string_func.free(key) ;
    if(value)
        json_object_func.free(value) ;
    if(json_object)
        json_object_func.free(json_object) ;
    if(json_dict)
        json_dict_func.free(json_dict) ;
    return nullptr ;
}

JsonObject *parse(char *buff, size_t sz)
{
    if(!buff || !sz)
        return nullptr ;
    JsonReader json_reader ;
    json_reader.buff = buff ;
    json_reader.curr_pos = 0 ;
    json_reader.total_sz = sz ;
    skip_spaces(&json_reader) ;
    JsonObject *json_object = nullptr ;
    json_object = parse_value(&json_reader) ;
    skip_spaces(&json_reader) ;
    if(is_valid(&json_reader))
    {
        json_object_func.free(json_object) ;
        return nullptr ;
    }
    return json_object ;
}

int main(int argc, char *argv[])
{
    if(argc != 2 )
    {
        printf("请输入测试参数\n") ;
        return -1 ;
    }
    char *dict_str = "{\"test\": 1, \"test1\": \"2\\t\", \"test\": {\"test1\": 1, \"test2\": true}, \"test4\": [null, true, false, 1, \"xxx\", {\"test5:\": 1}]}" ;
    dict_str = argv[1] ;
    printf("your input is:[%s]\n", argv[1]) ;
    JsonObject *json_object = parse(dict_str, strlen(dict_str)) ;
    if(!json_object)
    {
       printf("--%s-- is invalid json string\n", dict_str) ;  
    }
    else
    {
        printf("--%s-- is a valid json string\n", dict_str) ;    
        print_json_object(json_object, 0) ;
    }
    json_object_func.free(json_object) ;
    return 0 ;
}
