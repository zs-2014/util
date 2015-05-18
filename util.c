#include "string.h"

//判断一个字符串转化成字符int64有没有溢出
int is_int64_over_flow(char *number)
{
    if(!number)
        return 0;
    //9223372036854775807 == INT64_MAX
    //9223372036854775808 == INT64_MIN
    char *max_min_str[] = {"9223372036854775807", "9223372036854775808"} ;
    char *tmp = max_min_str[0] ;
    if(number[0] == '-')  
    {   
        number++;
        tmp = max_min_str[1] ;
    }
 
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
