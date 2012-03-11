#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>



#include "url.h"

enum {
    kScheme,
    kUser,
    kPassword,
    kHost,
    kPort,
    kPath,
    kParams,
    kQuery,
    kFragment
};


/*
 * scheme://username:password@domain:port/path;parms?query_string#fragment_id
 * 
 * 
 */

int URLComponentGetC(const char *url, URLComponents *components, int type, char *dest)
{
    URLRange *rangePtr;
    URLRange r;
    
    if (!url || !components) return -1;
    
    
    if (type < URLComponentScheme || type > URLComponentPathAndQuery) return -1;
    
    rangePtr = &components->scheme;
    
    r = rangePtr[type];

    if (!dest) return r.length;
    else
    {
        if (r.length)
        {
            memcpy(dest, url + r.location, r.length);
        }
        dest[r.length] = 0;
        
        return r.length;
    }
}

// pstring.
int URLComponentGet(const char *url, URLComponents *components, int type, char *dest)
{
    URLRange *rangePtr;
    URLRange r;
    
    if (!url || !components) return -1;
    
    
    if (type < URLComponentScheme || type > URLComponentPathAndQuery) return -1;
    
    rangePtr = &components->scheme;
    
    r = rangePtr[type];

    if (!dest) return r.length;
    else if (r.length > 255) return -1;
    else
    {
        dest[0] = r.length;
        if (r.length)
        {
            memcpy(dest + 1, url + r.location, r.length);
        }
        
        return r.length;
    }
}


#if 0
int schemeType(const char *string)
{
  if (!string || !*string) return SCHEME_NONE;
 
  static struct {
    int length;
    const char *data
  } table[] = {
    { 4, "file:" },
    { 3, "ftp:" },
    { 6, "gopher:" },
    { 4, "http:" },
    { 5, "https:" },
    { 6, "mailto:" },
    { 4, "news:" },
    { 4, "nntp:" },
    { 6, "telnet:" }
  };
  
  switch(*string)
  {
  case 'f':
    // ftp, file
    if (!strcmp(string, "file")) return SCHEME_FILE;
    if (!strcmp(string, "ftp")) return SCHEME_FTP;    
    break;
    
  case 'g':
    // gopher
    if (!strcmp(string, "gopher")) return SCHEME_GOPHER;
    break;
    
  case 'h':
    // http, https
    if (!strcmp(string, "https")) return SCHEME_HTTPS;
    if (!strcmp(string, "http")) return SCHEME_HTTP;    
    break;
    
  case 'm':
    // mailto
    if (!strcmp(string, "mailto")) return SCHEME_MAILTO;
    break;
    
  case 'n':
   // news, nntp
    if (!strcmp(string, "news")) return SCHEME_NEWS;
    if (!strcmp(string, "nntp")) return SCHEME_NNTP;
   break;
   
  case 't':
    // telnet
    if (!strcmp(string, "telnet")) return SCHEME_TELNET;
    break;
  
  }

  return SCHEME_UNKNOWN;

}

#endif

int ParseURL(const char *url, int length, struct URLComponents *components)
{
    char c;
    int state;
    int hasScheme;
    int hasAt;
    
    int i, l;
    
    URLRange range = { 0, 0 };
    URLRange *rangePtr;
    
    
    if (!components || !url) return 0;

    rangePtr = &components->scheme;
    memset(components, 0, sizeof(URLComponents));   

    state = kScheme;
    
    // 1 check for a scheme.
    // [A-Za-z0-9.+-]+ ':'
    
    
    hasScheme = 0; 
    hasAt = 0;   
    range.location = 0;
    
    for(i = 0; i < length; ++i)
    {
        c = url[i];
        
        if (c == ':')
        {
            hasScheme = 1;
            break;
        }
        
        if (c >= 'a' && c <= 'z') continue;
        if (c >= 'A' && c <= 'Z') continue;
        if (c >= '0' && c <= '9') continue;
        
        //if (isalnum(c)) continue;
        if (c == '.') continue;
        if (c == '+') continue;
        if (c == '-') continue;
        
        break;
    }
    
    if (hasScheme)
    {
        if (i == 0) return 0;
        range.length = i;

        components->scheme = range;
        ++i; // skip the ':'
    }
    else
    {
        state = kPath;
        i = 0;
    }

    // 2. check for //
    // //user:password@domain:port/
    // otherwise, it's a path
        
    if (strncmp(url + i, "//", 2) == 0)
    {
        state = kUser;
        i += 2;
    }
    else
    {
        state = kPath;
    }
    
    //range = (URLRange){ i, 0 };
    range.location = i; range.length = 0;
      
    for( ; i < length; ++i)
    {
        c = url[i];
            
        if ((c < 32) || (c & 0x80))
        {
            // goto invalid_char;
            return 0;
        }
    
        switch(c)
        {
        // illegal chars.
        // % # not allowed but allowed.
        
        case ' ':
        case '"':
        case '<':
        case '>':
        case '[':
        case '\\':
        case ']':
        case '^':
        case '`':
        case '{':
        case '|':
        case '}':
        case 0x7f:
            // goto invalid_char;
            return 0;
            break;
        
        
        case ':':
            // username -> password
            // domain -> port
            // password -> error
            // port -> error
            // all others, no transition.
            switch(state)
            {
            case kUser:
            case kHost:
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }
                //range = (URLRange){ i + 1 , 0 };
                range.location = i + 1; range.length = 0;
                
                state++;
                break;
            
            
            case kPassword:
            case kPort:
                return 0;
            
            }
            break;


        case '@':
            // username -> domain
            // password -> domain
            // all others no transition.
            // path -> error?
            
            switch(state)
            {
            case kUser:
            case kPassword:
                hasAt = 1;
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }
                //range = (URLRange){ i + 1 , 0 };
                range.location = i + 1; range.length = 0;
                
                state = kHost;            
                break;
            }
            break;
            
        case '/':
            // domain -> path
            // port -> path
            
            // (must be switched to domain:port)
            // username -> path
            // password -> path
            // all others, no transition.
            
            // '/' is part of the filename.
            switch (state)
            {
            case kUser:
                hasAt = 1;        // close enough
                state = kHost;    // username was actually the domain. 
                                  // pass through.     
            case kPassword:       // need to switch to domain:port later.
            case kHost:
            case kPort:
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }          
                
                //range = (URLRange){ i, 0 }; // '/' is part of path.
                range.location = i; range.length = 0;
                
                state = kPath; 
                break;            
            }
            break;
        
        case ';':
            // path -> param
            // all others, no transition.
            
            if (state == kPath)
            {
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }
                //range = (URLRange){ i + 1 , 0 };
                range.location = i + 1; range.length = 0;
                state = kParams; 
            }
            break;  
            
        case '?':
            // path -> query
            // param -> query
            // all others, no transition.
                  
            if (state == kPath || state == kParams)
            {
                l = i - range.location;
                if (l > 0)
                {
                    range.length = l;
                    rangePtr[state] = range; 
                }
                //range = (URLRange){ i + 1 , 0 };
                range.location = i + 1; range.length = 0;
                
                state = kQuery;
            }
            break;
            
            
        case '#':
            // fragment -> no transition
            // everything else transitions to a fragment.
            // can immediately end parsing since we know the length...
            
            l = i - range.location;
            if (l > 0)
            {
                range.length = l;
                rangePtr[state] = range; 
            }
            
            state = kFragment;
            range.location = i + 1; range.length = 0;

            i = length; // break from loop.
            break;              
        }

    }
    // finish up the last portion.
    if (state)
    {
        l = i - range.location;
        if (l > 0)
        {
            range.length = l;
            rangePtr[state] = range; 
        }        
    }
    
    
    if (!hasAt)
    {
        // user:name was actually domain:port
        components->host = components->user;
        components->port = components->password;
        
        components->user.location = 0;
        components->user.length = 0;
        components->password.location = 0;
        components->password.length = 0;
    }
    
    // port number conversion.
    if (components->port.location)
    {
        const char *tmp = url + components->port.location;
        int p = 0;

        l = components->port.length;
        
        
        for (i = 0; i < l; ++i)
        {
            c = tmp[i];
            
            if ((c < '0') || (c > '9'))
            {
                p = 0;
                break;
            }
            // convert to number.
            c &= 0x0f;
            
            // p *= 10;
            // 10x = 2x + 8x = (x << 1) + (x << 3)
            if (p)
            {
                int p2;
                
                p2 = p << 1;
                p = p2 << 2;
                p = p + p2;
            }
            p += c; 
        }
        
        components->portNumber = p;
    }
    
    #if 0
    // path and query.
    // path;params?query
    range = components->path;
    if (range.length)
    {
        if (components->params.length)
            range.length += components->params.length + 1;
        if (components->query.length)
            range.length += components->query.length + 1;
        
        components->pathAndQuery = range;
    }
    #endif
    
    return 1;

}

#ifdef TEST

void test(const char *url)
{
    URLComponents data;
    int ok;
    
    char *buffer;
    buffer = strdup(url); // enough space.
    
    if (!url || !*url) return;
    
    ok = ParseURL(url, strlen(url), &data);
    
    printf("%s (%s)\n", url, ok ? "ok" : "error");
    

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
    
    URLGetComponentCString(url, &data, URLComponentPath, buffer);    
    printf("        path: %s\n", buffer);
    
    URLGetComponentCString(url, &data, URLComponentParams, buffer);        
    printf("      params: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentQuery, buffer);        
    printf("       query: %s\n", buffer);
    
    URLComponentGetC(url, &data, URLComponentFragment, buffer);        
    printf("    fragment: %s\n", buffer);
        
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

#endif
