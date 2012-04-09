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
    
    char *cp;
    Handle h;
    LongWord hsize;
    LongWord size;
    Word term;
    Word tlen;    
    Word state;
    
    
    if (!buffer) return 0; // ?
    
    buffer->bufferhandle = NULL;
    buffer->bufferSize = 0;
    buffer->terminator = 0;
    buffer->moreFlag = 0;

    urh = TCPIPGetUserRecord(ipid);
    if (_toolErr || !urh) return 0;
    
    ur = *urh;
    

    state = ur->uwTCP_State;
    // TCPREAD returns these errors.
    if (state == TCPSCLOSED) return tcperrBadConnection;
    if (state < TCPSESTABLISHED) return tcperrNoResources;

    h = (Handle *)ur->uwTCPDataIn;
    // should never happen....
    if (!h) return tcperrNoResources;
    cp = *(char **)h;
    
    hsize = GetHandleSize(h);
    
    if (!hsize)
    {
        if (state > TCPSCLOSEWAIT) return tcperrConClosing;
        return 0;
    }
    
    size = find_crlf(cp, hsize);
    
    // -1 = not found.
    if (size == 0xffffffff)
    {
        
        // if state >= CLOSEWAIT, assume no more data incoming
        // and return as-is w/o terminator.
        // if state < TCPSCLOSEWAIT, the terminator has not yet been
        // received, so don't return anything.
        
        // tcpread does an implied push if state == tcpsCLOSEWAIT
        
        if (state < TCPSCLOSEWAIT)
        {
            buffer->moreFlag = 1;
            return 0;
        }
        
        term = 0;
        tlen = 0;
    }
    else
    {    
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
    }
    
    // read the data.
    // read will stop reading if there was a push.
    // if using \r\n, there could be something stupid like a push in the middle,
    // so read it in and then shrink it afterwards.
    //
    
    // 99% of the time, it should just be one read, but generating the handle now
    // and reading into a pointer keeps it simpler for the case where that is not the case.
    
    // size = data to return
    // hsize = data to read.
    
    hsize = size + tlen;
    h = NewHandle(hsize, ur->uwUserID, attrNoSpec | attrLocked, 0);
    if (_toolErr) return tcperrNoResources;

    buffer->bufferSize = size;
    buffer->bufferHandle = h;
    buffer->term = term;

    cp = *(char **)h;
    
    while (hsize)
    {
        Word rv;
        rrBuff rb;

        rv = TCPIPReadTCP(ipid, 0, cp, hsize, &rb);
        // tcperrConClosing is the only possible error 
        // (others were handled above via the state). 
        
        hsize -= rb.rrBuffCount;
        cp += rb.rrBuffCount;
        
        buffer->moreFlag = rb.rrMoreFlag;
        // should never hang in an infinite loop.
    }
        
    if (tlen)
    {
        // remove the delimiter
        
        // if line length is 0, dispose the handle entirely.
        // term will be set to indicate it's a blank line.
        if (!size)
        {
            DisposeHandle(h);
            buffer->bufferHandle = 0;
        }
        else
        {
            SetHandleSize(size, h);
        }
    }
    
    // will be conclosing or 0.
    return ur->uwTCP_ErrCode;
}