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

#include "connection.h"

int read_binary(Word ipid, FILE *file)
{
  Word rv = 0;

  TCPIPPoll();

  for(;;)
  { 
    static char buffer[512];
    rrBuff rb;
    Word count;

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

    fwrite(buffer, 1, count, file);
  }
 
  return rv;
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