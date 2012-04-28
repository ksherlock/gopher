#ifndef __prototypes_h__
#define __prototypes_h__

#include <stdio.h>

int read_binary(unsigned ipid, FILE *file);

int setfiletype(const char *filename);


#ifdef __CONNECTION_H__
int ConnectLoop(char *host, Word port, Connection *connection);
int CloseLoop(Connection *connection);
#endif

#ifdef __url_h__
int do_gopher(const char *url, URLComponents *components, FILE *file);
int do_http_0_9(const char *url, URLComponents *components, FILE *file);
int do_http_1_0(const char *url, URLComponents *components, FILE *file);
int do_http_1_1(const char *url, URLComponents *components, FILE *file);
#endif

#endif
