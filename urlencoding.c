#include "urlencoding.h"


static char hexchars[] = {
    '0', '1', '2', '3' ,'4' ,'5', '6', '7',
    '8', '9', 'A', 'B', 'C' ,'D' ,'E', 'F' };


unsigned urlencoded_length(const char *str)
{
    unsigned result = 0;
    for( ; *str; ++str)
    {
        if( ( *str >= '0' && *str <= '9' ) || ( *str >= 'a' && *str <= 'z') ||
            ( *str >= 'A' && *str <= 'Z' ) || *str == '$' || *str == '-' ||
            *str == '_' || *str == '.' || *str == '+' || *str == '!' ||
            *str == '*' || *str == '\'' || *str == '(' || *str == ')' )
        {
            result += 1;
        }
        else
        {
            result += 3;
        }
    }
    return result;
}


unsigned urldecoded_length(const char *str)
{
    unsigned result = 0;
    for( ; *str; ++str)
    {
        if( str[0] == '%' &&
            ( ( str[1] >= '0' && str[1] <= '9' ) ||
              ( str[1] >= 'a' && str[1] <= 'f' ) ||
              ( str[1] >= 'A' && str[1] <= 'F' ) ) &&
            ( ( str[2] >= '0' && str[2] <= '9' ) ||
              ( str[2] >= 'a' && str[2] <= 'f' ) ||
              ( str[2] >= 'A' && str[2] <= 'F' ) ) )
        {
            str += 2;
        }
        ++result;
    }
    return result;
}


void urlencode(char *dst, const char *src)
{
    while(*src)
    {
        if( ( *src >= '0' && *src <= '9' ) || ( *src >= 'a' && *src <= 'z' ) ||
            ( *src >= 'A' && *src <= 'Z' ) || *src == '$' || *src == '-' ||
            *src == '_' || *src == '.' || *src == '+' || *src == '!' ||
            *src == '*' || *src == '\'' || *src == '(' || *src == ')' )
        {
            *dst++ = *src++;
        }
        else
        {
            *dst++ = '%';
            *dst++ = hexchars[((unsigned char)*src) >> 4];
            *dst++ = hexchars[*src & 15];
            ++src;
        }
    }
    *dst = '\0';
}

void urldecode(char *dst, const char *src)
{
    for( ; *src; ++src)
    {
        if( src[0] == '%' &&
            ( ( src[1] >= '0' && src[1] <= '9' ) ||
              ( src[1] >= 'a' && src[1] <= 'f' ) ||
              ( src[1] >= 'A' && src[1] <= 'F' ) ) &&
            ( ( src[2] >= '0' && src[2] <= '9' ) ||
              ( src[2] >= 'a' && src[2] <= 'f' ) ||
              ( src[2] >= 'A' && src[2] <= 'F' ) ) )
        {
            if( src[1] >= '0' && src[1] <= '9' )
                *dst = (src[1] - '0') << 4;
            else
            if( src[1] >= 'a' && src[1] <= 'f' )
                *dst = (10 + src[1] - 'a') << 4;
            else
                *dst = (10 + src[1] - 'A') << 4;

            if( src[2] >= '0' && src[2] <= '9' )
                *dst |= (src[2] - '0');
            else
            if( src[2] >= 'a' && src[2] <= 'f' )
                *dst |= (10 + src[2] - 'a');
            else
                *dst |= (10 + src[2] - 'A');
            
             ++dst, src += 3;
        }
        else
        {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}
