#ifndef __readline2_h__
#define __readline2_h__

#ifndef __TCPIP__
#include <tcpip.h>
#endif

enum {
    TERMINATOR_CR = 0x0d,
    TERMINATOR_LF = 0x0a,
    TERMINATOR_CR_LF = 0x0a0d // byte-swapped.
};

typedef struct rlBuffer {
    Handle bufferHandle;
    LongWord bufferSize;
    Word terminator;
    Word moreFlag;
} rlBuffer; 

int ReadLine2(Word ipid, rlBuffer *buffer);

#endif
