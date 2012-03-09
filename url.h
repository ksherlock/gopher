#ifndef __url_h__
#define __url_h__

enum {
    SCHEME_UNKNOWN = -1,
    SCHEME_NONE = 0,
    SCHEME_FILE,
    SCHEME_FTP,
    SCHEME_GOPHER,
    SCHEME_HTTP,
    SCHEME_HTTPS,
    SCHEME_MAILTO,
    SCHEME_NEWS,
    SCHEME_NNTP,
    SCHEME_TELNET
};


typedef struct URLRange {
    int location;
    int length;
} URLRange;

enum {
    URLComponentScheme,
    URLComponentUser,
    URLComponentPassword,
    URLComponentHost,
    URLComponentPort,
    URLComponentPath,
    URLComponentParams,
    URLComponentQuery,
    URLComponentFragment,
    URLComponentPathAndQuery
};

typedef struct URLComponents {

    int schemeType;
    int portNumber;

    URLRange scheme;
    URLRange user;
    URLRange password;
    URLRange host;
    URLRange port;
    URLRange path;
    URLRange params;
    URLRange query;
    URLRange fragment;
    
    URLRange pathAndQuery;
     
} URLComponents;

int ParseURL(const char *url, int length, struct URLComponents *components);

int URLGetComponentCString(const char *url, struct URLComponents *, int, char *);
int URLGetComponentPString(const char *url, struct URLComponents *, int, char *);
//int URLGetComponentGSString(const char *url, struct URLComponents *, int, GSString255Ptr);


#endif
