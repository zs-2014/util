
/*
 * 1.unicode不是一种编码格式，而是一个编码规范。
 * 2.unicode提供了在[0x0000, 0xFFFF]范围内，每个数值所对应的字符。
 * 3.unicode能提供0xFFFF+1个字符，刚好是2个字节所能表示的范围，
 *   所以我们常说unicode占两个字节。其实这是不对的，不存在unicode
 *   这种编码格式，这种格式都不存在，谈何占空间啊。
 */

/*
 * 多字节编码实际的范围为[0x000000, 0x10FFFF]这个范围又分成了17个平面，
 * [0x0000, 0xFFFF]、[0x010000, 0x01FFFF]....[0x100000, 0x10FFFF]。
 * 也就是说每个平面的编码空间的大小为0xFFFF+1，刚好是两个字节所能表示的
 * 个数。第一个平面也就是[0x0000, 0xFFFF]这个编码空间表示的是基本面，对
 * 应的就是我们常说的unicode，其他的16个都有各自的意思。这里主要看unicode
 * 对应的基本面。
 */

/*
 * utf-16编码:
 * 1.对应[0x0000, 0xFFFF]范围的字符utf-16直接使用两个字节的codepoint表示。
 * 2.对应[0x010000, 0x10FFFF]范围的，utf-16用四个字节表示，也就是surrogate pair表示。
 *   具体的计算方式如下，例如有一个数字为0x10086
 *   (1)首先 0x10086 - 0x10000 ==>0x00086
 *   (2)将步骤(1)计算得到的数值分成两部分,每部分10bits 0x00086 == 0000 0000 00|00 1000 0110
 *      ===>(h=0000 0000 00, l=00 1000 0110)
 *   (3)将步骤(2)得到的h(高10bits)和l(低10bits)分别和0xD800、0xDC00 |(或) 操作即: 
 *      h|0xD800 ==>0xD800, l|0xDC00 ===> 0xDC86
 *  (4)则0x10086的utf-16编码为0xD800 0XDC86或者\uD800\uDC86,这是小端法表示的
 *  至于大小端的问题，可以参见wiki(https://en.wikipedia.org/wiki/UTF-16#Examples)
 */


#define be_to_le(n) (((n) >> 8)&0xFF)|(((n)&0xFF)<<8)
#define le_to_be(n) be_to_le(n)

int codep_to_utf8(unsigned int code_point, unsigned char *utf8_str)
{
    if(code_point < 0x80)
    {
        utf8_str[0] = code_point & 0x7F;
        return 1;
    }
    else if(code_point < 0x800)
    {
        utf8_str[0] = ((code_point >> 6) & 0x1F) | 0xC0;
        utf8_str[1] = (code_point & 0x3F) | 0x80;
        return 2;
    }
    else if(code_point < 0x10000)
    {
        utf8_str[0] = ((code_point >> 12)&0xF)|0xE0;
        utf8_str[1] = ((code_point >> 6)&0x3F)|0x80;
        utf8_str[2] = (code_point & 0x3F) | 0x80;
        return 3;
    }
    else if(code_point < 0x10FFFF)
    {
        utf8_str[0] = ((code_point >> 18) & 0x07) | 0xF0;
        utf8_str[1] = ((code_point >> 12)&0x3F)|0x80;
        utf8_str[2] = ((code_point >> 6)&0x3F)|0x80;
        utf8_str[3] = (code_point & 0x3F) | 0x80;
        return 4;
    }
    return -1;
}

/*
    utf-8转换对照关系。
    其中的 1，2，3，4表示的是在utf-8中的第几个字节。
    x是从codepoint的高位开始取，而存储在utf-8字节时
    是从低到高位字节存的。
                                1        2       3       4
    U-00000000 - U-0000007F: 0xxxxxxx
    U-00000080 - U-000007FF: 110yyyyy 10xxxxxx
    U-00000800 - U-0000FFFF: 1110yyyy 10xxxxxx 10zzzzzz
    U-00010000 - U-001FFFFF: 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
*/

/*
 *@u_str 待转换unicode buffer
 *@total_len u_str的长度
 *@from_len 被转换的unicode的长度
 *@utf_str 存放utf8的buffer
 *@to_len 转换后utf8的长度
 *@is_be 待转换的unicode是否是大端法存储的
 */

void *utf16_to_utf8(const void *src, 
                    unsigned int total_len, //u_str 的长度
                    unsigned int *from_len, //返回被转化的unicode字符的个数理论上 convert_chars == total_len/2
                    void *dst, //转化后的utf-8字符串存放的地方
                    unsigned int *to_len,
                    int is_be)
{
    if(!src|| !dst)
        return dst;
    unsigned char *utf8_str = (unsigned char *)dst;
    unsigned int _from_len = 0;
    if(!from_len)
        from_len = &_from_len;
    *from_len = 0;

    unsigned int _to_len = 0 ;
    if(!to_len)
        to_len = &_to_len;
    *to_len = 0;
    unsigned short *beg = (unsigned short *)src; 
    while(*from_len + 2 <= total_len)
    { 
        int codep1 = is_be ? be_to_le(*beg) : *beg ;
        //四字节表示方式
        if(codep1 >= 0xD800)
        {
            if(codep1 > 0xDBFF)
                break;
            //第二个code point必须存在
            if(*from_len + 4 > total_len)
                break;
            beg++;
            int codep2 = is_be ? be_to_le(*beg) : *beg ;
            //第二个code point必须大于0xDC00
            if(codep2 < 0xDC00 || codep2 > 0xDFFF)
                break;
            *from_len += 2;
            *to_len += codep_to_utf8(((codep2&0x3FF)|((codep1&0x3FF)<<10))+0x10000, utf8_str+*to_len);
        }
        else
        {
            //将此code point 转换成utf8
            int ret = codep_to_utf8(codep1, utf8_str+*to_len);    
            if(ret > 0)
                *to_len += ret;
            else
                break;
        }
        *from_len += 2;
        beg++;
    }
    return dst;
}

void *utf8_to_utf16(const void *src,
                    unsigned int total_len,
                    unsigned int *from_len,
                    void *dst,
                    unsigned int *to_len,
                    int is_to_be)
{
    if(!src || !dst)
        return dst;
    unsigned int _from_len = 0;
    if(!from_len)
        from_len = &_from_len;
    *from_len = 0;

    unsigned int _to_len = 0;
    if(!to_len)
        to_len = &_to_len;
    *to_len = 0;

    unsigned char *utf8_str = (unsigned char *)src;
    unsigned short *utf16_str = (unsigned short*)dst;
    unsigned char masks[] = {0x7F, 0x1F, 0x0F, 0x0E};
    while(*from_len < total_len)
    {
        int cnt = 0;
        unsigned char n = *utf8_str;
        while(n&0x80)
        {
            n = n << 1;
            cnt++;
        }
        if(cnt == 0)
            cnt = 1;
        if(*from_len + cnt > total_len)
            break;
        if(cnt > sizeof(masks)/sizeof(masks[0]))
            break;
        unsigned char mask = masks[cnt-1];
        unsigned codep = utf8_str[0]&mask;
        int i=1;
        for(i=1; i<cnt; i++)
            codep = (codep<<6)|(utf8_str[i]&0x3F);
        //保留字符区间
        if(codep >= 0xD800 && codep <= 0xFFFF)
            break;
        //[0x0000, 0xD800)区间
        if(codep < 0x10000)
        {
            *utf16_str++ = is_to_be?(((codep>>8)&0xFF)|(codep&0xFF)<<8):codep;
            *to_len +=2;
        }
        //[0x10000, 0x10FFFF)区间
        else
        {
            codep = codep - 0x10000; 
            unsigned h = 0xD800|((codep>>10)&0x3F);
            unsigned l = 0xDC00|(codep&0x3F);
            *utf16_str++ = is_to_be?le_to_be(h):h;
            *utf16_str++ = is_to_be?le_to_be(l):l;
            *to_len +=4;
        }
        *from_len += cnt;
        utf8_str += cnt ;
    }
    return dst;
}

#ifdef TEST
#include <stdio.h>
#include <string.h>
int get_utf16_length(const void *src)
{
    const unsigned short *beg = (const unsigned short*)src;
    int l = 0;
    while(*beg++ != 0)
        l+=2;
    return l;
}

void print_utf16_string(const char *src, int len)
{
   int i = 0;
   const char *hex_str = "0123456789ABCDEF";
   for(i=0; i < len; i+=2)
   {
        printf("\\u%c%c%c%c", hex_str[(src[i+1]>>4)&0x0F], hex_str[src[i+1]&0x0F], 
                               hex_str[(src[i]>>4)&0x0F], hex_str[src[i]&0x0F]);
   }
   printf("\n");
}

void test_utf8_to_utf16()
{
    char dst_buff[1024] = {0}; 
    char *src = "这是一个合法的utf8字符串";
    unsigned int to_len = 0;
    unsigned from_len = 0;
    unsigned total_len = strlen(src);
    //小端法
    utf8_to_utf16(src, total_len, &from_len, dst_buff, &to_len, 0);
    print_utf16_string(dst_buff, to_len);
    printf("total_len=%d from_len=%d to_len=%d\n", total_len, from_len, to_len);
    //大端法
    memset(dst_buff, 0, sizeof(dst_buff));
    utf8_to_utf16(src, total_len, &from_len, dst_buff, &to_len, 1);
    print_utf16_string(dst_buff, to_len);
    printf("total_len=%d from_len=%d to_len=%d\n", total_len, from_len, to_len);

    src = "这是一个包含了笑脸的utf-8字符串😂l";
    total_len = strlen(src);
    memset(dst_buff, 0, sizeof(dst_buff));
    utf8_to_utf16(src, total_len, &from_len, dst_buff, &to_len, 0);
    print_utf16_string(dst_buff, to_len);
    printf("total_len=%d from_len=%d to_len=%d\n", total_len, from_len, to_len);
    
    //错误的utf-8串
    src = "\xff\xe8\xbf\x99\xe6\x98\xaf\xe4\xb8\xaa\xe9\x94\x99\xe8\xaf\xaf\xe7\x9a\x84\xe5\xad\x97\xe7\xac\xa6";
    total_len = strlen(src);
    memset(dst_buff, 0, sizeof(dst_buff));
    utf8_to_utf16(src, total_len, &from_len, dst_buff, &to_len, 0);
    print_utf16_string(dst_buff, to_len);
    printf("total_len=%d from_len=%d to_len=%d\n", total_len, from_len, to_len);

    //空utf-8串
    src = "";
    total_len = strlen(src);
    memset(dst_buff, 0, sizeof(dst_buff));
    utf8_to_utf16(src, total_len, &from_len, dst_buff, &to_len, 0);
    print_utf16_string(dst_buff, to_len);
    printf("total_len=%d from_len=%d to_len=%d\n", total_len, from_len, to_len);
}


void test_utf16_to_utf8()
{
    char dst_buff[1024] = {0};   
    char *src = "\xd9\x8f\x2f\x66\x00\x4e\x2a\x4e\x4b\x6d\xd5\x8b\x22\x25\x26\x25\x3d\xd8\x02\xde";
    char *src1 = "\x8f\xd9\x66\x2f\x4e\x00\x4e\x2a\x6d\x4b\x8b\xd5\xd8\x3d\xde\x02\x00\x00";
    unsigned int to_len = 0;
    unsigned int from_len = 0;
    utf16_to_utf8(src1, get_utf16_length(src1), &from_len, dst_buff, &to_len, 1);
    printf("to_len = %d\nfrom_len=%d\ndst_buff=%s\n", to_len, from_len, dst_buff);
}

int main(int argc, char *argv[])
{
    test_utf8_to_utf16();
    test_utf16_to_utf8();
    return 0;
}
#endif
