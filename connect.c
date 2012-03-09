#include "Connect.h"

//static char pstring[256];


static Word LoginAndOpen(ConnectBuffer *buffer)
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
    buffer->state = ConnectStateError;
    return -1;
  }

  terr = TCPIPOpenTCP(ipid);
  if (_toolErr || terr)
  {
    TCPIPLogout(ipid);
    buffer->state = ConnectStateError;
    buffer->terr = terr;
    buffer->ipid = 0;
    return -1;
  }

  buffer->ipid = ipid;
  buffer->state = ConnectStateConnecting;

  return 0;
}


Word ConnectionPoll(ConnectBuffer *buffer)
{
  Word state;
  if (!buffer) return -1;
  state = buffer->state;

  if (state == 0) return -1;
  if (state == ConnectStateConnected) return 1;
  if (state == ConnectStateDisconnected) return 1;
  if (state == ConnectStateError) return -1;

  TCPIPPoll();

  if (state == ConnectStateDNR)
  {
    if (buffer->dnr.DNRstatus == DNR_OK)
    {
        return LoginAndOpen(buffer);
    }
    else if (buffer->dnr.DNRstatus != DNR_Pending)
    {
      buffer->state = ConnectStateError;
      return -1;
    }
  }

  if (state == ConnectStateConnecting  || state == ConnectStateDisconnecting)
  {
    Word terr;
    static srBuff sr;

    terr = TCPIPStatusTCP(buffer->ipid, &sr);

    if (state == ConnectStateDisconnecting)
    {
      // these are not errors.
      if (terr == tcperrConClosing || terr == tcperrClosing)
        terr = tcperrOK; 
    }

    if (terr || _toolErr)
    {
      //CloseAndLogout(buffer);

      TCPIPCloseTCP(buffer->ipid);
      TCPIPLogout(buffer->ipid);
      buffer->ipid = 0;
      buffer->state = ConnectStateError;
      buffer->terr = terr;
      return -1;
    }

    if (sr.srState == TCPSESTABLISHED) //  && state == ConnectStateConnecting)
    {
      buffer->state = ConnectStateConnected;
      return 1;
    }

    if (sr.srState == TCPSCLOSED || sr.srState == TCPSTIMEWAIT)
    {
      TCPIPLogout(buffer->ipid);
      buffer->ipid = 0;
      buffer->state = ConnectStateDisconnected;
      return 1;
    }
  }

  return 0;
}

Word ConnectionOpen(ConnectBuffer *buffer, const char *host, Word port)
{
  buffer->state = 0;
  buffer->ipid = 0;
  buffer->terr = 0;
  buffer->port = port;

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
    buffer->state = ConnectStateError;
    return -1;
  }
  buffer->state = ConnectStateDNR;
  return 0;
}

void ConnectionInit(ConnectBuffer *buffer, Word memID)
{
  buffer->memID = memID;
  buffer->ipid = 0;
  buffer->terr = 0;
  buffer->state = 0;
  buffer->port = 0;
  buffer->dnr.DNRstatus = 0;
  buffer->dnr.DNRIPaddress = 0;
}

Word ConnectionClose(ConnectBuffer *buffer)
{
  Word state = buffer->state;

  // todo -- how do you close if not yet connected?
  if (state == ConnectStateConnected)
  {
    buffer->state = ConnectStateDisconnecting;
    buffer->terr = TCPIPCloseTCP(buffer->ipid);
    return 0;
  }

  if (state == ConnectStateDNR)
  {
    TCPIPCancelDNR(&buffer->dnr);
    buffer->state = 0;
    return 1;
  }

  return -1;
}

