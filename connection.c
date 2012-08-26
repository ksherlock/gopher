#pragma optimize 79
#pragma noroot

#include "connection.h"
#include <string.h>
#include "s16debug.h"

static char pstring[256];


static Word LoginAndOpen(Connection *buffer)
{
  Word ipid;
  Word terr;

  ipid = TCPIPLogin(
    buffer->memID, 
    buffer->dnr.DNRIPaddress, 
    buffer->port, 
    0x0000, 0x0040);

  if (_toolErr)
  {
    buffer->state = kConnectionStateError;
    return -1;
  }

  terr = TCPIPOpenTCP(ipid);
  if (_toolErr || terr)
  {
    TCPIPLogout(ipid);
    buffer->state = kConnectionStateError;
    buffer->terr = terr;
    buffer->ipid = 0;
    return -1;
  }

  buffer->ipid = ipid;
  buffer->state = kConnectionStateConnecting;

  return 0;
}


Word ConnectionPoll(Connection *buffer)
{

  Word state;
  if (!buffer) return -1;
  state = buffer->state;

  if (state == 0) return -1;
  if (state == kConnectionStateConnected) return 1;
  if (state == kConnectionStateDisconnected) return 1;
  if (state == kConnectionStateError) return -1;

  TCPIPPoll();

  if (state == kConnectionStateDNR)
  {
    if (buffer->dnr.DNRstatus == DNR_OK)
    {
        return LoginAndOpen(buffer);
    }
    else if (buffer->dnr.DNRstatus != DNR_Pending)
    {
      buffer->state = kConnectionStateError;
      return -1;
    }
  }

  if (state == kConnectionStateConnecting  || state == kConnectionStateDisconnecting)
  {
    Word terr;
    static srBuff sr;

    terr = TCPIPStatusTCP(buffer->ipid, &sr);

    if (state == kConnectionStateDisconnecting)
    {
      // these are not errors.
      if (terr == tcperrConClosing || terr == tcperrClosing)
        terr = tcperrOK; 
    }

    if (terr || _toolErr)
    {
      //CloseAndLogout(buffer);

      s16_debug_printf("terr = %04x tool error = %04x", terr, _toolErr);
      s16_debug_srbuff(&sr);

      TCPIPCloseTCP(buffer->ipid);
      TCPIPLogout(buffer->ipid);
      buffer->ipid = 0;
      buffer->state = kConnectionStateError;
      buffer->terr = terr;
      return -1;
    }

    if (sr.srState == TCPSESTABLISHED) //  && state == kConnectionStateConnecting)
    {
      buffer->state = kConnectionStateConnected;
      return 1;
    }

    if (sr.srState == TCPSCLOSED || sr.srState == TCPSTIMEWAIT)
    {
    
      s16_debug_srbuff(&sr);

      TCPIPLogout(buffer->ipid);
      buffer->ipid = 0;
      buffer->state = kConnectionStateDisconnected;
      return 1;
    }
  }

  return 0;
}

Word ConnectionOpenC(Connection *buffer, const char *host, Word port)
{
  Word length;

  if (!host) return -1;

  length = strlen(host);
  if (length > 255) return -1;

  pstring[0] = length & 0xff;
  memcpy(pstring + 1, host, length);

  return ConnectionOpen(buffer, pstring, port);
}

Word ConnectionOpenGS(Connection *buffer, const GSString255 *host, Word port)
{
  if (!host) return -1;
  if (host->length > 255) return -1;
  
  pstring[0] = host->length & 0xff;
  memcpy(pstring + 1, host->text, host->length);
  
  return ConnectionOpen(buffer, pstring, port);
}

Word ConnectionOpen(Connection *buffer, const char *host, Word port)
{
  buffer->state = 0;
  buffer->ipid = 0;
  buffer->terr = 0;
  buffer->port = port;

  if (!buffer || !*buffer) return -1;

  // 1. check if we need to do DNR.
  if (TCPIPValidateIPString(host))
  {
    cvtRec cvt;
    TCPIPConvertIPToHex(&cvt, host);
    buffer->dnr.DNRIPaddress = cvt.cvtIPAddress;
    buffer->dnr.DNRstatus = DNR_OK; // fake it.
   
    return LoginAndOpen(buffer); 
  }
  // do dnr.
  TCPIPDNRNameToIP(host, &buffer->dnr);
  if (_toolErr)
  {
    buffer->state = kConnectionStateError;
    return -1;
  }
  buffer->state = kConnectionStateDNR;
  return 0;
}

void ConnectionInit(Connection *buffer, Word memID)
{
  buffer->memID = memID;
  buffer->ipid = 0;
  buffer->terr = 0;
  buffer->state = 0;
  buffer->port = 0;
  buffer->dnr.DNRstatus = 0;
  buffer->dnr.DNRIPaddress = 0;
}

Word ConnectionClose(Connection *buffer)
{
  Word state = buffer->state;

  // todo -- how do you close if not yet connected?
  if (state == kConnectionStateConnected)
  {
    buffer->state = kConnectionStateDisconnecting;
    buffer->terr = TCPIPCloseTCP(buffer->ipid);
    return 0;
  }

  if (state == kConnectionStateDNR)
  {
    TCPIPCancelDNR(&buffer->dnr);
    buffer->state = 0;
    return 1;
  }

  return -1;
}

