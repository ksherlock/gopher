#include <stdio.h>
#include <string.h>

#include "http.utils.h"

int main(int argc, char **argv)
{

    int version;
    int status;
    int ok;
    char *cp;
    
    URLRange k, v;
    
    cp = "HTTP/1.0 200 OK";
    ok = parseStatusLine(cp, strlen(cp), &version, &status);
    if (!ok || version != 0x0100 || status != 200)
    {
        fprintf(stderr, "%s: %x %d\n", cp, version, status);
    }

    cp = "HTTP/1.1 404 bleh";
    ok = parseStatusLine(cp, strlen(cp), &version, &status);
    if (!ok || version != 0x0101 || status != 404)
    {
        fprintf(stderr, "%s: %x %d\n", cp, version, status);
    }


    cp = "http/1.10 200 asdasd as dsad asd";
    ok = parseStatusLine(cp, strlen(cp), &version, &status);
    if (!ok || version != 0x010a || status != 200)
    {
        fprintf(stderr, "%s: %x %d\n", cp, version, status);
    }


    cp = "key: value";
    ok = parseHeaderLine(cp, strlen(cp), &k, &v);
    if (!ok 
        || k.location != 0 
        || k.length != 3 
        || v.location != 5 
        || v.length !=  5)
    {
        fprintf(stderr, "%s: %.*s, %.*s\n", cp, 
            k.length, cp + k.location,
            v.length, cp + v.location);
    }


    return 0;
}


