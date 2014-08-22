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


static void dump_header(const smb2_header_sync *header)
{
  fprintf(stderr, "   protocol_id: %08lx\n", header->protocol_id);
  fprintf(stderr, "structure_size: %04x\n", header->structure_size);
  fprintf(stderr, " credit_charge: %04x\n", header->credit_charge);
  fprintf(stderr, "        status: %08lx\n", header->status);
  fprintf(stderr, "       command: %04x\n", header->command);
  fprintf(stderr, "        credit: %04x\n", header->credit);
  fprintf(stderr, "         flags: %08lx\n", header->flags);
  fprintf(stderr, "  next_command: %08lx\n", header->next_command);
  fprintf(stderr, "    message_id: %08lx%08lx\n", header->message_id[1], header->message_id[0]);
  fprintf(stderr, "      reserved: %08lx\n", header->reserved);
  fprintf(stderr, "       tree_id: %08lx\n", header->tree_id);
  fprintf(stderr, "    session_id: %08lx%08lx\n", header->session_id[1], header->session_id[0]);
  fprintf(stderr, "     signature:\n");
  hexdump(header->signature, 16);

}

static void write_message(Word ipid, const void *data1, unsigned size1, const void *data2, unsigned size2)
{
  uint8_t nbthead[4];
  uint32_t size;

  size = sizeof(header) + size1 + size2;

  // http://support.microsoft.com/kb/204279
  nbthead[0] = 0;
  nbthead[1] = size >> 16;
  nbthead[2] = size >> 8;
  nbthead[3] = size;

  if (++header.message_id[0] == 0)
    ++header.message_id[1];

  TCPIPWriteTCP(ipid, (dataPtr)nbthead, sizeof(nbthead), false, false);
  TCPIPWriteTCP(ipid, (dataPtr)header, sizeof(header), false, false);
  TCPIPWriteTCP(ipid, (dataPtr)data1, size1, false, false);
  TCPIPWriteTCP(ipid, (dataPtr)data2, size2, true, false);
}

static Handle read_response(Word ipid)
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

  if (nbthead[0] != 0) {
    fprintf(stderr, "Bad NBT Header\n");
    return (Handle)0;
  }
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
  static struct smb2_negotiate_request negotiate_req;
  static struct smb2_session_setup_request setup_req;

  static uint16_t dialects[] = { 0x0202 };

  smb2_header_sync *headerPtr;
  void *responsePtr;


  Handle h;

  uint8_t nbthead[4];
  uint32_t size = 0;



  memset(&header, 0, sizeof(header));
  memset(&negotiate_req, 0, sizeof(negotiate_req));
  memset(&setup_req, 0, sizeof(setup_req));


  header.protocol_id = SMB2_MAGIC; // '\xfeSMB';
  header.structure_size = 64;
  header.command = SMB2_NEGOTIATE;

  negotiate_req.structure_size = 36;
  negotiate_req.dialect_count = 1; // ?
  negotiate_req.security_mode = SMB2_NEGOTIATE_SIGNING_ENABLED;
  negotiate_req.capabilities = 0;

  //negotiate_req.dialects[0] = 0x202; // smb 2.002


#if 0
  // http://support.microsoft.com/kb/204279
  size = sizeof(header) + sizeof(negotiate_req) + sizeof(dialects);
  nbthead[0] = 0;
  nbthead[1] = size >> 16;
  nbthead[2] = size >> 8;
  nbthead[3] = size;

  next_message();
  TCPIPWriteTCP(ipid, (dataPtr)nbthead, sizeof(nbthead), false, false);
  TCPIPWriteTCP(ipid, (dataPtr)&header, sizeof(header), false, false);
  TCPIPWriteTCP(ipid, (dataPtr)&negotiate_req, sizeof(negotiate_req), false, false);
  TCPIPWriteTCP(ipid, (dataPtr)&dialects, sizeof(dialects), true, false);
  // push.
#endif

  write_message(ipid, &negotiate_req, sizeof(negotiate_req), dialects, sizeof(dialects));

  // read a response...

  h = read_response(ipid);
  if (!h) return -1;
  HLock(h);

  hexdump(*h, GetHandleSize(h));

  headerPtr = *(smb2_header_sync **)h;
  responsePtr = (uint8_t *)headerPtr + sizeof(smb2_header_sync);

  dump_header(headerPtr);

  if (headerPtr->protocol_id != SMB2_MAGIC || headerPtr->structure_size != 64)
  {
    DisposeHandle(h);
    fprintf(stderr, "Invalid SMB2 header\n");
    return -1;
  }

  if (headerPtr->command != SMB2_NEGOTIATE)
  {
    DisposeHandle(h);
    fprintf(stderr, "Unexpected SMB2 command\n");
    return -1;
  }
  DisposeHandle(h);

  // 
  header.command = SMB2_SESSION_SETUP;

  setup_req.structure_size = 25;
  setup_req.flags = 0;
  setup_req.security_mode = SMB2_NEGOTIATE_SIGNING_ENABLED;
  setup_req.capabilities = 0;


  write_message(ipid, &setup_req, sizeof(setup_req), NULL, 0);


  h = read_response(ipid);
  if (!h) return -1;
  HLock(h);

  hexdump(*h, GetHandleSize(h));

  headerPtr = *(smb2_header_sync **)h;
  responsePtr = (uint8_t *)headerPtr + sizeof(smb2_header_sync);

  dump_header(headerPtr);

  if (headerPtr->protocol_id != SMB2_MAGIC || headerPtr->structure_size != 64)
  {
    DisposeHandle(h);
    fprintf(stderr, "Invalid SMB2 header\n");
    return -1;
  }

  if (headerPtr->command != SMB2_SESSION_SETUP)
  {
    DisposeHandle(h);
    fprintf(stderr, "Unexpected SMB2 command\n");
    return -1;
  }
  DisposeHandle(h);

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



  CloseLoop(&connection);

  free(host);
  return 0;
}