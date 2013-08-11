#ifndef __prototypes_h__
#define __prototypes_h__

#include <stdio.h>


#define IncBusy() asm { jsl 0xE10064 }
#define DecBusy() asm { jsl 0xE10068 }
#define Resched() asm { cop 0x7f }

#define BusyFlag ((byte *)0xE100FFl)

#define SEI() asm { sei }
#define CLI() asm { cli }


typedef struct ReadBlock
{
	LongWord requestCount;
	LongWord transferCount;
} ReadBlock;

int read_binary(unsigned ipid, FILE *file, ReadBlock *);
int read_binary_size(unsigned ipid, FILE *file, ReadBlock *);

int parse_extension_c(const char *cp, Word *ftype, LongWord *atype);
int parse_extension(const char *cp, Word size, Word *ftype, LongWord *atype);

int parse_mime_c(const char *cp, Word *ftype, LongWord *atype);
int parse_mime(const char *cp, Word size, Word *ftype, LongWord *atype)


#ifdef __GSOS__
enum {
    ATTR_ACCESS = 1,
    ATTR_FILETYPE = 2,
    ATTR_AUXTYPE = 4,
    ATTR_CREATETIME = 8,
    ATTR_MODTIME = 16
};

int setfileattr(const char *filename, FileInfoRecGS *info, unsigned flags)

#endif


#ifdef __CONNECTION_H__
int ConnectLoop(char *host, Word port, Connection *connection);
int CloseLoop(Connection *connection);
#endif

#ifdef __url_h__
int do_gopher(const char *url, URLComponents *components);
int do_http(const char *url, URLComponents *components);
#endif

#ifdef __TYPES__
void tiTimeRec2ISO8601(const TimeRecPtr t, char *str);
void tiTimeRec2GMTString(const TimeRecPtr t, char *str);
#endif


#endif
