#include "readline2.h"
#include <tcpipx.h>


static void HandleAppend(Handle h1, Handle h2)
{
    LongWord size1, size2;
    HUnlock(h1);
    
    size1 = GetHandleSize(h1);
    size2 = GetHandleSize(h2);
    
    SetHandleSize(size1 + size2, h1);
    if (_toolErr) return;
    
    HandToPtr(h2, *h1 + size1, size2);     
}
/*
 * read a line terminated by \r\n, \r, or \n
 *
 *
 */
Word ReadLine2(Word ipid, rlBuffer *buffer)
{
    userRecordHandle urh;
    userRecordPtr ur;

    rrBuff rb;


    const char *cp;
    Handle h;
    LongWord hsize;
    LongWord size;
    Word term;
    Word tlen;
    Word rv;
    
    if (!buffer) return -1;
    
    buffer->handle = NULL;
    buffer->size = 0;
    buffer->terminator = 0;

    urh = TCPIPGetUserRecord(ipid);
    if (_toolErr || !urh) return -1;
    
    ur = *urh;
    
    h = (Handle *)ur->uwTCPDataIn;

    if (!h) return 0;
    cp = *(const char **)h;
    
    hsize = GetHandleSize(h);
    size = find_crlf(cp, hsize);
    
    if (size & 0x8000000)
    {
        // not found.
        // if closing, return data as-is w/o terminator?
        
        return 0;
    }
    
    
    hsize -= size;
    cp += size;
    
    // check for \r\n
    term = *(Word *)cp;
    if (term == 0x0a0d && hsize >= 2)
    {
        tlen = 2;
    }
    else
    {
        term &= 0x00ff;
        tlen = 1;
    }
    
    buffer->size = size;
    buffer->term = term;
    
    // read the data.
    // read will stop reading if there was a push.
    // if using \r\n, there could be something stupid like a push in the middle,
    // so read it in and then delete it afterwards.
    //
    size += tlen;
    h = NULL;

    while (size)
    {
        Handle h2;
        rv = TCPIPReadTCP(ipid, 2, 0, size, &rb);
        
        h2 = rb.rrBuffHandle;
        size -= rb.rrBuffCount;
     
        if (h)
        {
            // append.
            HandleAppend(h, h2);
            DisposeHandle(h2);
        }
        else
        {
            buffer->handle = h = h2;
        }
    }
        
    if (tlen)
    {
        // remove the delimiter
        h = buffer->handle;
        size = buffer->size;
        
        // if line length is 0, dispose the handle entirely.
        if (!size)
        {
            DisposeHandle(h);
            buffer->handle = 0;
        }
        else
        {
            SetHandleSize(size, h);
        }
    }
    
    return 1;
}