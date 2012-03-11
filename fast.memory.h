#ifndef _fast_memory_h_
#define _fast_memory_h_

struct _Handle
{
    void *pointer;
    Word attr;
    Word owner;
    LongWord size;
};

#define _HLock(h) ((struct _Handle *)h)->attr |= 0x8000
#define _HUnlock(h) ((struct _Handle *)h)->attr &= 0x7fff
#define _GetHandleSize(h) ((struct _Handle *)h)->size

#endif
