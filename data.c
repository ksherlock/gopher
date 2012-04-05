#include "Data.h"
#include <Memory.h>
#include "fast.memory.h"

typedef union _Data {
    LongWord size;
    char data[4];
} _Data;

#define SIZEOF_DATA_HEADER sizeof(LongWord);

#define H_TO_D(h) *((_Data **)h)

static Boolean reserve(Handle h, LongWord size)
{
    //LongWord currentSize;
    //currentSize = _GetHandleSize(h);
    
    // size includes the data header.
        
    size = (size + 4095) & ~4095;
    
    //if (size <= currentSize) return true;
    _HUnlock(h);
    
    SetHandleSize(size, h);
    if (_toolErr) return 0;
    _HLock(h);
    
    return 1;
}

/*
 * Create a Data Handle.
 *
 */
Handle DataCreate(Word MemID, Word initialSize)
{
    Handle h;
    
    if (initialSize < 1024) initialSize = 1024;
    
    h = NewHandle(initialSize, MemID, attrNoSpec | attrLocked, NULL);
    if (_toolErr) return NULL;

    (H_TO_D(h))->size = SIZEOF_DATA_HEADER;
    return h;
}


/*
 * reset the size to 0.
 *
 */
Word DataReset(Handle h)
{
    if (!h) return 0;
    
    _HLock(h);
    (H_TO_D(h))->size = SIZEOF_DATA_HEADER;
    _HUnlock(h);
    
    return 1;
}


Word DataAppendPString(Handle h, const char *s)
{
    if (!h) return 0;
    if (!s || !*s) return 1;
    
    return DataAppendBytes(h, s + 1, *s);
}

Word DataAppendGSString(Handle h, const GSString255Ptr s)
{
    if (!h) return 0;    
    if (!s || !s->length) return 1;
    
    return DataAppendBytes(h, s->text, s->length);
}

Word DataAppendCString(Handle h, const char *cp)
{
    Word l;

    if (!h) return 0;
    
    if (!cp || !*cp) return 1;

    // strlen (cp)
    /*
     * xba - sets Z if (A & 0x00ff) == 0
     */
     //
    asm {
        ldy #0
    loop:
        lda [<cp],y
        xba // a = byte 0
        beq done
        iny
        xba // a = byte 1
        beq done
        iny
        bne loop    // not bra in case not terminated
    done:
        sty <l
    }

    return DataAppendBytes(h, cp, l);
}


Word DataAppendBytes(Handle h, const char *bytes, LongWord count)
{
    _Data *data;
    LongWord dSize;
    LongWord hSize;
    LongWord newSize; 
    
    if (!h) return 0;
    
    _HLock(h);
    
    data = H_TO_D(h);
    dSize = data->size;
    hSize = _GetHandleSize(h);
    
    newSize = dSize + count;
    if (newSize > hSize)
    {
        if (!reserve(h, newSize)) return 0;
        data = H_TO_D(h);    
    }    
    
    BlockMove(bytes, data->data + dSize, count);
    
    data->size = newSize;
    return 1;
}

Word DataAppendChar(Handle h, char c)
{
    _Data *data;
    LongWord dSize;
    LongWord hSize;
    LongWord newSize;

    if (!h) return 0;

    _HLock(h);

    data = H_TO_D(h);
    dSize = data->size;
    hSize = _GetHandleSize(h);

    newSize = dSize + 1; // dSize includes header.
    
    if (newSize > hSize)
    {
        if (!reserve(h, newSize)) return 0;
        data = H_TO_D(h);    
    }
    
    data->data[dSize] = c;
    data->size = newSize;

    return 1;
}


LongWord DataSize(Handle h)
{
    if (!h) return 0;
    return (H_TO_D(h))->size - SIZEOF_DATA_HEADER;
}

void *DataBytes(Handle h)
{
    _Data *data;
    
    if (!h) return NULL;
    
    _HLock(h);
    
    data = H_TO_D(h);
    return data->data + SIZEOF_DATA_HEADER;
}

/*
 * guarantee at least minSize bytes available and
 * return a raw pointer to the data.
 */
char *DataLock(Handle h, Word newSize)
{
    LongWord hSize;
    _Data *data;
    
    if (!h) return NULL;
    
    _HLock(h);
  
      if (newSize)
      {  
        newSize += SIZEOF_DATA_HEADER;
        
        hSize = _GetHandleSize(h);
        
        if (newSize > hSize)
        {
            if (!reserve(h, newSize)) return NULL;
            
            // should also bzero it?
        }
    }
    
    data = H_TO_D(h);
    return data->data + SIZEOF_DATA_HEADER;
}

void DataUnlock(Handle h, Word newSize)
{
    if (!h) return;
    
    if (newSize == -1) return;
    
    (H_TO_D(h))->size = newSize + SIZEOF_DATA_HEADER;
}
