#ifdef __ORCAC__
#pragma optimize 79
#pragma noroot
#endif

#include <ctype.h>
#include "url.h"


int parseHeaderLine(const char *cp, unsigned length, URLRange *key, URLRange *value)
{
    unsigned i, l;
    
    key->location = 0;
    key->length = 0;
    value->location = 0;
    value->length = 0;
    
    // trim any trailing whitespace.
    
    while (length)
    {
        if (isspace(cp[length - 1])) 
            --length;
        else break;
    }
    if (!length) return 0;
    
    /* format:
     * key: value
     * /^([^:]+):\s+(.*)$/
     * -or-
     *  value [continuation of previous line]
     * /^\s+(.*)$/
     */
     
    for (i = 0; i < length; ++i)
    {
      if (cp[i] == ':') break;
    }
    
    if (i == length)
    {
      // try as value only
      i = 0;
    }
    else
    {
      key->length = i;
      i = i + 1;
    }
    
    // now gobble up all the whitespace...
    
    for ( ; i < length; ++i)
    {
      if (!isspace(cp[i])) break;
    }
    
    // no value? no problem!
    if (i == length) return 1;
    
    value->location = i;
    value->length = length - i;
    
    return 1;
}


int parseStatusLine(const char *cp, unsigned length, int *version, int *status)
{
    /*
     * HTTP/1.1 200 OK etc.
     *
     */

    unsigned short *wp;
    int i;
    char c;
    int x;
    
    *version = 0;
    *status = 0;
    
    wp = (unsigned short *)cp;

    
    // HTTP/
    if (length <= 5) return 0;
    if ((wp[0] | 0x2020) != 0x7468) return 0; // 'ht'
    if ((wp[1] | 0x2020) != 0x7074) return 0; // 'tp'
    if (cp[4] != '/') return 0;

    // version string. 
    // \d+ . \d+   
    i = 5;
    
    c = cp[i];
    if (!isdigit(c)) return 0;
    x = c - '0';

    for (i = i + 1; i < length; ++i)
    {
        c = cp[i];
        if (!isdigit(c)) break;
        x = (x << 1) + (x << 3) + (c - '0');
    }
    
    *version = x << 8;
    
    if (i == length) return 0;
    if (cp[i++] != '.') return 0;
    
    c = cp[i];
    if (!isdigit(c)) return 0;
    x = c - '0';

    for (i = i + 1; i < length; ++i)
    {
        c = cp[i];
        if (!isdigit(c)) break;
        x = (x << 1) + (x << 3) + (c - '0');
    }
    
    *version |= x;

    // 1+ space
    if (i == length) return 0;
    c = cp[i];
    if (!isspace(c)) return 0;

    for (i = i + 1; i < length; ++i)
    {
        c = cp[i];
        if (!isspace(c)) break;
    }
    
    if (i == length) return 0;

    c = cp[i];
    if (!isdigit(c)) return 0;
    x = c - '0';

    for (i = i + 1; i < length; ++i)
    {
        c = cp[i];
        if (!isdigit(c)) break;
        x = (x << 1) + (x << 3) + (c - '0');
    }
    
    *status = x;
    
    // rest of status line unimportant.
    
    return 1;
}
