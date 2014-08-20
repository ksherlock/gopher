#pragma optimize 79
#pragma noroot

#include <GSOS.h>
#include <Memory.h>
#include <MiscTool.h>
#include <tcpip.h>


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>

#include "url.h"
#include "connection.h"
#include "readline2.h"
#include "options.h"
#include "s16debug.h"

#include "prototypes.h"
#include "smb.h"

static struct smb2_header_sync header;


Handle read_response(Word ipid)
{
  static srBuff sr;
  static rrBuff rb;

  // read an smb response.  Check the first 4 bytes for the message length.
  uint32_t size = 0;
  uint8_t nbthead[4];
  LongWord qtick;
  Word terr;

  qtick = GetTick() + 30 * 60;
  for(;;)
  {


    TCPIPPoll();

    terr = TCPIPStatusTCP(ipid, &sr);

    if (sr.srRcvQueued >= 4) break;


    /*
     * the only reasonable error is if the connection is closing.
     * that's ok if there's still pending data (handled above)
     */
    if (terr) return (Handle)0;


    if (GetTick() >= qtick)
    {
      fprintf(stderr, "Read timed out.\n");
      return (Handle)0;
    }

  }

  terr = TCPIPReadTCP(ipid, 0, (Ref)&nbthead, 4, &rb);

  if (nbthead[0] != 0) return (Handle)0;
  size = nbthead[3];
  size |= nbthead[2] << 8; 
  size |= nbthead[1] << 16; 

  for(;;)
  {


    TCPIPPoll();

    terr = TCPIPStatusTCP(ipid, &sr);

    if (sr.srRcvQueued >= size) break;

    if (GetTick() >= qtick)
    {
      fprintf(stderr, "Read timed out.\n");
      return (Handle)0;
    }

  }
  terr = TCPIPReadTCP(ipid, 2, (Ref)0, size, &rb);

  return rb.rrBuffHandle;

}

int negotiate(Word ipid)
{
  static struct smb2_negotiate_request req;

  static uint16_t dialects[] = { 0x0202 };

  uint8_t nbthead[4];
  uint32_t size = 0;



  memset(&header, 0, sizeof(header));
  memset(&req, 0, sizeof(req));


  header.protocol_id = SMB2_MAGIC; // '\xfeSMB';
  header.structure_size = 64;
  header.command = SMB2_NEGOTIATE;

  req.structure_size = 36;
  req.dialect_count = 1; // ?
  req.security_mode = SMB2_NEGOTIATE_SIGNING_ENABLED;
  req.capabilities = 0;

  //req.dialects[0] = 0x202; // smb 2.002


  // http://support.microsoft.com/kb/204279
  size = sizeof(header) + sizeof(req) + sizeof(dialects);
  nbthead[0] = 0;
  nbthead[1] = size >> 16;
  nbthead[2] = size >> 8;
  nbthead[3] = size;

  TCPIPWriteTCP(ipid, (dataPtr)nbthead, sizeof(nbthead), false, false);
  TCPIPWriteTCP(ipid, (dataPtr)&header, sizeof(header), false, false);
  TCPIPWriteTCP(ipid, (dataPtr)&req, sizeof(req), false, false);
  TCPIPWriteTCP(ipid, (dataPtr)&dialects, sizeof(dialects), true, false);
  // push.

  // read a response...

  return 0;
}


int do_smb(char *url, URLComponents *components)
{
  static Connection connection;
  LongWord qtick;

  char *host;
  Word err;
  Word terr;
  Word ok;
  FILE *file;

  if (!components->portNumber) components->portNumber = 445;

  host = URLComponentGetCMalloc(url, components, URLComponentHost);

  if (!host)
  {
    fprintf(stderr, "URL `%s': no host.", url);
    return -1;
  }  

  ok = ConnectLoop(host, components->portNumber, &connection);
  
  if (!ok)
  {
    free(host);
    if (file != stdout) fclose(file);
    return -1;
  }

  ok = negotiate(connection.ipid);
  if (ok) return ok;


  qtick = GetTick() + 30 * 60;
  for(;;)
  {
    static srBuff sr;

    TCPIPPoll();

    terr = TCPIPStatusTCP(connection.ipid, &sr);

    if (sr.srRcvQueued > 0) break;


    if (GetTick() >= qtick)
    {
      fprintf(stderr, "Read timed out.\n");
      return -1;
    }

  }

  CloseLoop(&connection);

  free(host);
  return 0;
}
