#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "url.h"

void test(const char *url)
{
    URLComponents data;
    int ok;
    
    char *buffer;
    buffer = strdup(url); // enough space.
    
    if (!url || !*url) return;
    
    ok = ParseURL(url, strlen(url), &data);
    
    printf("%s (%s)\n", url, ok ? "ok" : "error");
    
    printf(" schemeType: %d\n", data.schemeType);

    URLComponentGetC(url, &data, URLComponentScheme, buffer);
    printf("      scheme: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentUser, buffer);    
    printf("    username: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentPassword, buffer);    
    printf("    password: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentHost, buffer);        
    printf("        host: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentPort, buffer);    
    printf("        port: %s [%d]\n", buffer, data.portNumber);
    
    URLComponentGetC(url, &data, URLComponentPath, buffer);    
    printf("        path: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentParams, buffer);        
    printf("      params: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentQuery, buffer);        
    printf("       query: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentFragment, buffer);        
    printf("    fragment: %s\n", buffer);

    URLComponentGetC(url, &data, URLComponentPathAndQuery, buffer);        
    printf("  path+query: %s\n", buffer);
        
    free(buffer);
        
}

int main(int argc, char **argv)
{
    int i;
    
    for (i = 1; i < argc; ++i)
    {
        test(argv[i]);
    }

    return 0;
} 

