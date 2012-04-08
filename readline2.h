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
    Handle handle;
    LongWord size;
    Word terminator;
} rlBuffer; 

Word ReadLine2(Word ipid, rlBuffer *buffer);

#endif