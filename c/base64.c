#include <stdio.h>
#include <string.h>
//base64编码对照表
/*
base64原理:
base64中将24bits拆分成4组，每组6bits.
对于最后的一个6bits小分组，如果不满6bits
那么在其后补0来达到6bits长度.
对于每24bits作为分组的大分组，如果最后凑不够
3个分组，则在其后添加1或者2个'='来凑够3个分组
例如:
ab  ---->0x61,0x62 ---->0110 0001  0110 0010
每6个一个分组，则被分成 0110 00/1 010110/2 0010 (00)/3
因为最后一个小分组只有4bits,所以要凑够6个bits,在其后面补足00
这样三个数值分别为: 011000 = 24  010110 = 22, 001000 = 8
查找对应表, 24对应的是Y, 22对应的是W, 8对应的是I
所以ab的base64编码时YWI,因为YWI不够一个大分组(4),
所以需要在其后添加一个 '='来凑够到4,所以最终的
base64编码是YWI=
*/
/*
0   A   16  Q   32  g   48  w
1   B   17  R   33  h   49  x
2   C   18  S   34  i   50  y
3   D   19  T   35  j   51  z
4   E   20  U   36  k   52  0
5   F   21  V   37  l   53  1
6   G   22  W   38  m   54  2
7   H   23  X   39  n   55  3
8   I   24  Y   40  o   56  4
9   J   25  Z   41  p   57  5
10  K   26  a   42  q   58  6
11  L   27  b   43  r   59  7
12  M   28  c   44  s   60  8
13  N   29  d   45  t   61  9
14  O   30  e   46  u   62  +
15  P   31  f   47  v   63  /
*/

#define MIN(a, b) (a) > (b) ? (b) : (a)

static char value_to_char_map[64] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"} ;
typedef unsigned char uchar ;

inline static uchar top_bits(uchar n, int bits)
{ 
    uchar bits_mask[] = {0x0, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
    return bits_mask[bits] & n ;
}

inline static int char_to_value(char c)
{
   return (c >= 'A' && c <= 'Z') ? (c - 'A') :( 
          (c >= 'a' && c <= 'z') ? (c - 'a' + 26):( 
          (c >= '0' && c <= '9') ? (c - '0' + 52):(
          (c == '+') ? 62 : (
          (c == '/') ? 63 : -1))));
}

inline static uchar bottom_bits(uchar n, int bits)
{
    uchar bits_mask[] = {0x0, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff};
    return bits_mask[bits] & n ;
}


int base64decode(const char *src, void *dst)
{
    if(src == NULL)
        return -1 ;
    int len = strlen(src) ;
    if(len == 0)
        return 0 ;
    memset(dst, 0, (len>>2)*3) ;
    uchar *dst_uchar = (uchar *)dst ;
    int left_bits = 8 ;
    while(*src != '\0')
    {
        //遇到了=则表示结束了
        if(*src == '=')
            break ;
        int v = char_to_value(*src) ;
        //base64编码不合法
        if(v == -1)
            return -1 ;
        v &= 0x3f ;
        int need_bits = MIN(left_bits, 6) ;
        int top = top_bits(v<<2, need_bits) ; 
        //移动到最后几位
        top = top >> (8-need_bits) ;
        //填充上一个字节的位数
        *dst_uchar = (*dst_uchar << need_bits)|top ;  
        left_bits -= need_bits ;
        if(left_bits == 0)
        {
            dst_uchar++ ;
            int bottom  = bottom_bits(v, 6-need_bits) ;
            left_bits = (8 - (6-need_bits)) ;
            *dst_uchar = bottom ; 
        }
        src++ ;
    }
    return dst_uchar - (uchar *)dst ;
}
int base64encode(const uchar *src, size_t srclen, void *dst)
{
    if(srclen == 0)
        return 0 ;

    size_t i = 0 ;
    char *dst_char = (char *)dst ;
    int pre_bits = 0 ;
    for(i=0; i< srclen; i++)
    {
        int p = 0 ;
        //上一个字节中未使用的bits
        if(pre_bits > 0)
        {
            //取出上一个字节剩下的bits 
            p = bottom_bits(src[i-1], pre_bits) ;
            //转化到6bits中的最高位
            p = p << (6 - pre_bits) ;
        }
        //从当前数据中凑出6bits剩下的bits
        int c = top_bits(src[i], 6-pre_bits); 
        //将取出的数据，移动到以最低位开始
        c = c >> (8-(6-pre_bits)) ; 
        //从最低位取出数据
        c = bottom_bits(c, 6-pre_bits);
        //转换成字符串
        *dst_char++ = value_to_char_map[p|c] ;
        //设置当前字节未使用的bits
        pre_bits = (8-(6-pre_bits));
        //如果有8个字节未使用，那么下次循环可以继续以使用这个
        if(pre_bits == 8)
        {
            i--;
            pre_bits = 0 ;
        }
    }
    //最后需要补0
    if(pre_bits != 0)
    {
       int v = bottom_bits(src[i-1], pre_bits) ; 
       v = v<<(6-pre_bits) ;
       *dst_char++ = value_to_char_map[v] ;
    }
    int mod = srclen % 3 ;
    if(3 - mod == 2)
    {
        *dst_char++ = '=' ;
        *dst_char++ = '=' ;
    }
    else if(3 - mod == 1)
    {
        *dst_char++ = '=' ;
    }
    return dst_char - (char *)dst ;
}

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int main(int argc, char *argv[])
{ 
    if(argc != 2)
    {
        printf("请输入要编码的字符串\n") ;
        return -1 ;
    }
    char dst[1024] = {0} ;
    int n = base64encode((uchar*)argv[1], strlen(argv[1]), dst) ;
    printf("encode:%s\n", dst) ;
    char src1[1024] = {1, 2, 3, 4} ;
    int n1 = base64decode(dst, src1) ;
    int i = 0 ; 
    printf("decode:\n");
    for(i=0; i < n1; i++)
    {
        printf("%c", src1[i]) ;
    }
    printf("\n") ;
    return 0 ;
}
