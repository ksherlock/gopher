#include "s16debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <tcpip.h>
#include <tcpipx.h>

#pragma optimize 79
#pragma noroot

static char buffer[512];

static Word CheckSweet16(void)
{
  static Word Sweet16 = -1;
  if (Sweet16 == -1)
  {
        /*
         * Apple II tech note 201
         */
        word emu_id = 0;
        word emu_version = 0;
        asm {
            // the lda ..,x is to prevent
            // the 2nd lda from being optimized out.
            lda #0
            ldx #0
            sep #0x20
            sta >0x00c04f,x
            lda >0x00c04f,x
            sta <emu_id
            lda >0x00c04f,x
            sta <emu_version
            rep #0x20
        }
        
        if (emu_id == 0x16 && emu_version >= 0x23) Sweet16 = 1;
        else Sweet16 = 0;
  }
  
  return Sweet16;
}

void s16_debug_puts(const char *str)
{
    if (!CheckSweet16()) return;
    
    asm {
        ldx <str+2
        ldy <str
        cop 0x84
    }
}


void s16_debug_dump(const char *bytes, unsigned length)
{
    static const char *HexMap = "0123456789abcdef";

    if (!CheckSweet16()) return;

    while (length)
    {
        Word i, j, l;
        l = 16;
        if (l > length) l = length;
    
        memset(buffer, ' ', sizeof(buffer));
        
        for (i = 0, j = 0; i < l; ++i)
        {
            unsigned x = bytes[i];
            buffer[j++] = HexMap[x >> 4];
            buffer[j++] = HexMap[x & 0x0f];
            j++;
            if (i == 7) j++;
            
            buffer[50 + i] = isascii(x) && isprint(x) ? x : '.';
        }
        
        buffer[50 + 16 + 1] = 0;
        s16_debug_puts(buffer);
    
        length -= l;
        bytes += l;
    }

}

void s16_debug_printf(const char *format, ...)
{
    // need to format the data even if sweet-16 is
    // not present in order to clean up the stack.
    
    va_list ap;
    va_start(ap, format);
    vsnprintf(buffer, sizeof(buffer), format, ap);
    va_end(ap);
    
    s16_debug_puts(buffer);
}

//---

void s16_debug_srbuff(const srBuff *sb)
{
    if (!CheckSweet16()) return;

/*
    s16_debug_printf("%04x %04x %08lx %08lx %08lx %04x %04x %04x",
        sb->srState,
        sb->srNetworkError,
        sb->srSndQueued,
        sb->srRcvQueued,
        sb->srDestIP,
        sb->srDestPort,
        sb->srConnectType, 
        sb->srAcceptCount
    );
*/

    s16_debug_printf("             srState: $%04x", sb->srState);
    s16_debug_printf("      srNetworkError: $%04x", sb->srNetworkError);
    s16_debug_printf("         srSndQueued: $%08lx", sb->srSndQueued);
    s16_debug_printf("         srRcvQueued: $%08lx", sb->srRcvQueued);
    s16_debug_printf("            srDestIP: $%08lx", sb->srDestIP);
    s16_debug_printf("          srDestPort: $%04x", sb->srDestPort);
    s16_debug_printf("       srConnectType: $%04x", sb->srConnectType);
    s16_debug_printf("       srAcceptCount: $%04x", sb->srAcceptCount);

}



void s16_debug_tcp(unsigned ipid)
{
    userRecordPtr ur;
    userRecordHandle urh;
  
    if (!CheckSweet16()) return;

    urh = TCPIPGetUserRecord(ipid);
    
    if (!urh) return;
    
    ur = *urh;
    if (!ur) return;
    if (!ur->uwUserID) return;
    
    // -- auto-generated --
    
    s16_debug_printf("            uwUserID: $%04x", ur->uwUserID);
    s16_debug_printf("            uwDestIP: $%08lx", ur->uwDestIP);
    s16_debug_printf("          uwDestPort: $%04x", ur->uwDestPort);
    s16_debug_printf("            uwIP_TOS: $%04x", ur->uwIP_TOS);
    s16_debug_printf("            uwIP_TTL: $%04x", ur->uwIP_TTL);
    s16_debug_printf("        uwSourcePort: $%04x", ur->uwSourcePort);
    s16_debug_printf("     uwLogoutPending: $%04x", ur->uwLogoutPending);
    s16_debug_printf("         uwICMPQueue: $%08lx", ur->uwICMPQueue);
    s16_debug_printf("          uwTCPQueue: $%08lx", ur->uwTCPQueue);
    s16_debug_printf("     uwTCPMaxSendSeg: $%04x", ur->uwTCPMaxSendSeg);
    s16_debug_printf("  uwTCPMaxReceiveSeg: $%04x", ur->uwTCPMaxReceiveSeg);
    s16_debug_printf("        uwTCPDataInQ: $%08lx", ur->uwTCPDataInQ);
    s16_debug_printf("         uwTCPDataIn: $%08lx", ur->uwTCPDataIn);
    s16_debug_printf("     uwTCPPushInFlag: $%04x", ur->uwTCPPushInFlag);
    s16_debug_printf("   uwTCPPushInOffset: $%08lx", ur->uwTCPPushInOffset);
    s16_debug_printf("    uwTCPPushOutFlag: $%04x", ur->uwTCPPushOutFlag);
    s16_debug_printf("     uwTCPPushOutSEQ: $%08lx", ur->uwTCPPushOutSEQ);
    s16_debug_printf("        uwTCPDataOut: $%08lx", ur->uwTCPDataOut);
    s16_debug_printf("           uwSND_UNA: $%08lx", ur->uwSND_UNA);
    s16_debug_printf("           uwSND_NXT: $%08lx", ur->uwSND_NXT);
    s16_debug_printf("           uwSND_WND: $%04x", ur->uwSND_WND);
    s16_debug_printf("            uwSND_UP: $%04x", ur->uwSND_UP);
    s16_debug_printf("           uwSND_WL1: $%08lx", ur->uwSND_WL1);
    s16_debug_printf("           uwSND_WL2: $%08lx", ur->uwSND_WL2);
    s16_debug_printf("               uwISS: $%08lx", ur->uwISS);
    s16_debug_printf("           uwRCV_NXT: $%08lx", ur->uwRCV_NXT);
    s16_debug_printf("           uwRCV_WND: $%04x", ur->uwRCV_WND);
    s16_debug_printf("            uwRCV_UP: $%04x", ur->uwRCV_UP);
    s16_debug_printf("               uwIRS: $%08lx", ur->uwIRS);
    s16_debug_printf("         uwTCP_State: $%04x", ur->uwTCP_State);
    s16_debug_printf("     uwTCP_StateTick: $%08lx", ur->uwTCP_StateTick);
    s16_debug_printf("       uwTCP_ErrCode: $%04x", ur->uwTCP_ErrCode);
    s16_debug_printf("     uwTCP_ICMPError: $%04x", ur->uwTCP_ICMPError);
    s16_debug_printf("        uwTCP_Server: $%04x", ur->uwTCP_Server);
    s16_debug_printf("     uwTCP_ChildList: $%08lx", ur->uwTCP_ChildList);
    s16_debug_printf("    uwTCP_ACKPending: $%04x", ur->uwTCP_ACKPending);
    s16_debug_printf("      uwTCP_ForceFIN: $%04x", ur->uwTCP_ForceFIN);
    s16_debug_printf("        uwTCP_FINSEQ: $%08lx", ur->uwTCP_FINSEQ);
    s16_debug_printf("    uwTCP_MyFINACKed: $%04x", ur->uwTCP_MyFINACKed);
    s16_debug_printf("         uwTCP_Timer: $%08lx", ur->uwTCP_Timer);
    s16_debug_printf("    uwTCP_TimerState: $%04x", ur->uwTCP_TimerState);
    s16_debug_printf("      uwTCP_rt_timer: $%04x", ur->uwTCP_rt_timer);
    s16_debug_printf("    uwTCP_2MSL_timer: $%04x", ur->uwTCP_2MSL_timer);
    s16_debug_printf("       uwTCP_SaveTTL: $%04x", ur->uwTCP_SaveTTL);
    s16_debug_printf("       uwTCP_SaveTOS: $%04x", ur->uwTCP_SaveTOS);
    s16_debug_printf("       uwTCP_TotalIN: $%08lx", ur->uwTCP_TotalIN);
    s16_debug_printf("      uwTCP_TotalOUT: $%08lx", ur->uwTCP_TotalOUT);
    s16_debug_printf("        uwUDP_Server: $%04x", ur->uwUDP_Server);
    s16_debug_printf("          uwUDPQueue: $%08lx", ur->uwUDPQueue);
    s16_debug_printf("          uwUDPError: $%04x", ur->uwUDPError);
    s16_debug_printf("      uwUDPErrorTick: $%08lx", ur->uwUDPErrorTick);
    s16_debug_printf("          uwUDPCount: $%08lx", ur->uwUDPCount);

}

