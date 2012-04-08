#include "readline2.h"
#include <tcpipx.h>


static LongWord find_crlf(const char *cp, LongWord length)
{
    LongWord rv;
    
    rv = 0;

    
    asm {
        
        // scan all the full blocks.
        ldy #0
        ldx length+2
        beq remainder
        
    loop1:
        lda [<cp],y
        and #0xff
        cmp #'\r'
        beq match
        cmp #'\n'
        beq match
        iny
        bne loop1
        // scanned a full bank, increment cp && rv
        // decrement x
        inc <cp+2
        inc <rv+2
        dex
        bne loop1
        // if x = 0, drop to remainder bytes.
        
    remainder:
        // scan non-full bank
        // y = 0
        // x is the counter var.
        ldx <length
        beq done
        
    loop2:
        lda [<cp],y
        and #0xff
        cmp #'\r'
        beq match
        cmp #'\n'
        beq match
        iny
        dex
        beq done
        bra loop2
         
    match:
        sty <rv
        bra exit
        
    done:
        lda #-1
        sta <rv
        sta <rv+2
    exit:
    }
    
    return rv;
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


    char *cp;
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
    cp = *(char **)h;
    
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
    
    //buffer->size = size;
    //buffer->term = term;
    
    // read the data.
    // read will stop reading if there was a push.
    // if using \r\n, there could be something stupid like a push in the middle,
    // so read it in and then shrink it afterwards.
    //
    
    // 99% of the time, it should just be one read, but generating the handle now
    // and reading into a pointer keeps it simpler for the case where that is not the case.
    
    hsize = size + tlen;
    h = NewHandle(hsize, ur->uwUserID, attrNoSpec | attrLocked, 0);
    if (_toolErr) return -1;

    buffer->size = size;
    buffer->handle = h;
    buffer->term = term;

    cp = *(char **)h;
    
    while (hsize)
    {
        rv = TCPIPReadTCP(ipid, 0, cp, hsize, &rb);
        // break on tcp error?
        
        hsize -= rb.rrBuffCount;
        cp += rb.rrBuffCount
    }
        
    if (tlen)
    {
        // remove the delimiter
        
        // if line length is 0, dispose the handle entirely.
        // term will be set to indicate it's a blank line.
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