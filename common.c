/*
 *
 * common routines.
 */
#pragma noroot
#pragma optimize 79

#include <Memory.h>
#include <MiscTool.h>
#include <tcpip.h>


#include <stdio.h>

#include "prototypes.h"
 #include "connection.h"

int read_binary(Word ipid, FILE *file, ReadBlock *dcb)
{
  Word rv = 0;


  if (dcb) dcb->transferCount = 0;

  IncBusy();
  TCPIPPoll();
  DecBusy();

  for(;;)
  { 
    static char buffer[512];
    rrBuff rb;
    Word count;
    Word tcount;

    IncBusy();
    rv = TCPIPReadTCP(ipid, 0, (Ref)buffer, 512, &rb);
    DecBusy();
    
    count = rb.rrBuffCount;
    if (!count)
    {
        if (rv) break;
        IncBusy();
        TCPIPPoll();
        DecBusy();
        continue;
    }

    tcount = fwrite(buffer, 1, count, file);
    if (dcb) dcb->transferCount += tcount;

    if (tcount != count) return -1;

  }
 
  return 0;
}

// ReadTCP will only return if the entire request is fulfilled or the connection closes,
// so it could just do that.  For a large request, this is probably nicer.
int read_binary_size(Word ipid, FILE *file, ReadBlock *dcb)
{
  Word rv = 0;
  LongWord size;
  if (!dcb) return -1;

  dcb->transferCount = 0;
  size = dcb->requestCount;
  if (!size) return 0;

  IncBusy();
  TCPIPPoll();
  DecBusy();

  for(;;)
  { 
    static char buffer[512];
    rrBuff rb;
    Word count;
    Word tcount;

    count = 512;
    if (count > size) count = size;

    IncBusy();
    rv = TCPIPReadTCP(ipid, 0, (Ref)buffer, count, &rb);
    DecBusy();
    
    count = rb.rrBuffCount;
    if (!count)
    {
        if (rv) break;
        IncBusy();
        TCPIPPoll();
        DecBusy();
        continue;
    }

    tcount = fwrite(buffer, 1, count, file);

    dcb->transferCount += tcount;
    size -= tcount;

    if (tcount != count) return -1;

    if (!size) return 0;
  }
 
  return 0;

}


int ConnectLoop(char *host, Word port, Connection *connection)
{
    LongWord qtick;

    ConnectionInit(connection, MMStartUp());
    ConnectionOpenC(connection, host,  port);

  // 30 second timeout.
  qtick = GetTick() + 30 * 60;
  while (!ConnectionPoll(connection))
  {
    if (GetTick() >= qtick)
    {
      fprintf(stderr, "Connection timed out.\n");

      IncBusy();
      TCPIPAbortTCP(connection->ipid);
      TCPIPLogout(connection->ipid);      
      DecBusy();
      
      return 0;
    }
  }

  if (connection->state != kConnectionStateConnected)
  {
    fprintf(stderr, "Unable to open host: %s:%u\n", 
      host, 
      port);
    return 0;
  }

  return 1;
}


int CloseLoop(Connection *connection)
{
    ConnectionClose(connection);

    while (!ConnectionPoll(connection)) ; // wait for it to close.
      
    return 1;
}