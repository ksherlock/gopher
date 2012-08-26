#ifndef __prototypes_h__
#define __prototypes_h__

#include <stdio.h>


#define IncBusy() asm { jsl 0xE10064 }
#define DecBusy() asm { jsl 0xE10068 }
#define Resched() asm { cop 0x7f }

#define BusyFlag ((byte *)0xE100FFl)

#define SEI() asm { sei }
#define CLI() asm { cli }


int read_binary(unsigned ipid, FILE *file);

int setfiletype(const char *filename);


#ifdef __CONNECTION_H__
int ConnectLoop(char *host, Word port, Connection *connection);
int CloseLoop(Connection *connection);
#endif

#ifdef __url_h__
int do_gopher(const char *url, URLComponents *components);
int do_http(const char *url, URLComponents *components);
#endif

#endif
