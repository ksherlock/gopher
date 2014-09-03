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
#include "smb.errors.h"
#include "asn1.h"

static struct smb2_header_sync header;


typedef struct smb_response {

  smb2_header_sync header;
  union {
    smb2_error_response error;
    smb2_negotiate_response negotiate;
    smb2_session_setup_response setup;
    smb2_tree_connect_response tree_connect;
    smb2_logoff_response logoff;
  } body;

} smb_response;


// 3.3.4.4 Sending an Error Response
// http://msdn.microsoft.com/en-us/library/cc246722.aspx
static int is_error(const smb_response *msg)
{
  uint32_t status = msg->header.status;
  uint16_t command = msg->header.command;

  if (status == 0) return false;

  switch (command)
  {
    case SMB2_SESSION_SETUP:
    if (status == STATUS_MORE_PROCESSING_REQUIRED)
      return false;
    break;

  case SMB2_QUERY_INFO:
  case SMB2_READ:
    if (status == STATUS_BUFFER_OVERFLOW)
      return false;
    break;

  case SMB2_CHANGE_NOTIFY:
    if (status == STATUS_NOTIFY_ENUM_DIR)
      return false;
    break;

  }

  return true;
}

static void dump_header(const smb2_header_sync *header)
{
  fprintf(stdout, "   protocol_id: %08lx\n", header->protocol_id);
  fprintf(stdout, "structure_size: %04x\n", header->structure_size);
  fprintf(stdout, " credit_charge: %04x\n", header->credit_charge);
  fprintf(stdout, "        status: %08lx\n", header->status);
  fprintf(stdout, "       command: %04x\n", header->command);
  fprintf(stdout, "        credit: %04x\n", header->credit);
  fprintf(stdout, "         flags: %08lx\n", header->flags);
  fprintf(stdout, "  next_command: %08lx\n", header->next_command);
  fprintf(stdout, "    message_id: %08lx%08lx\n", header->message_id[1], header->message_id[0]);
  fprintf(stdout, "      reserved: %08lx\n", header->reserved);
  fprintf(stdout, "       tree_id: %08lx\n", header->tree_id);
  fprintf(stdout, "    session_id: %08lx%08lx\n", header->session_id[1], header->session_id[0]);

  fprintf(stdout, "     signature:\n");
  hexdump(header->signature, 16);

}


static void dump_error(const smb2_error_response *msg)
{
  fprintf(stdout, "structure_size: %04x\n", msg->structure_size);
  fprintf(stdout, "      reserved: %04x\n", msg->reserved);
  fprintf(stdout, "     bytecount: %08lx\n", msg->bytecount);

  fprintf(stdout, "   error_data:\n");
  hexdump((const char *)msg + sizeof(smb2_error_response) , msg->bytecount);
}


static void dump_negotiate(const smb2_negotiate_response *msg)
{
  fprintf(stdout, "        structure_size: %04x\n", msg->structure_size);
  fprintf(stdout, "         security_mode: %04x\n", msg->security_mode);
  fprintf(stdout, "      dialect_revision: %04x\n", msg->dialect_revision);
  fprintf(stdout, "              reserved: %04x\n", msg->reserved);
  fprintf(stdout, "           server_guid:\n");
  hexdump(msg->server_guid, 16);

  fprintf(stdout, "          capabilities: %08lx\n", msg->capabilities);
  fprintf(stdout, "     max_transact_size: %08lx\n", msg->max_transact_size);
  fprintf(stdout, "         max_read_size: %08lx\n", msg->max_read_size);
  fprintf(stdout, "        max_write_size: %08lx\n", msg->max_write_size);

  fprintf(stdout, "           system_time: %08lx%08lx\n", 
    msg->system_time[1], msg->system_time[0]);

  fprintf(stdout, "     server_start_time: %08lx%08lx\n", 
    msg->server_start_time[1], msg->server_start_time[0]);

  fprintf(stdout, "security_buffer_offset: %04x\n", msg->security_buffer_offset);
  fprintf(stdout, "security_buffer_length: %04x\n", msg->security_buffer_length);
  fprintf(stdout, "             reserved2: %08lx\n", msg->reserved2);

  fprintf(stdout, "                buffer:\n");
  hexdump((const char *)msg - sizeof(smb2_header_sync) + msg->security_buffer_offset, 
    msg->security_buffer_length);
}

static void dump_setup(const smb2_session_setup_response *msg)
{
  fprintf(stdout, "        structure_size: %04x\n", msg->structure_size);
  fprintf(stdout, "         session_flags: %04x\n", msg->session_flags);
  fprintf(stdout, "security_buffer_offset: %04x\n", msg->security_buffer_offset);
  fprintf(stdout, "security_buffer_length: %04x\n", msg->security_buffer_length);

  fprintf(stdout, "                buffer:\n");
  hexdump((const char *)msg - sizeof(smb2_header_sync) + msg->security_buffer_offset, 
    msg->security_buffer_length);
}


static void dump_tree_connect(const smb2_tree_connect_response *msg)
{
  fprintf(stdout, "        structure_size: %04x\n", msg->structure_size);
  fprintf(stdout, "            share_type: %02x\n", msg->share_type);
  fprintf(stdout, "              reserved: %02x\n", msg->reserved);
  fprintf(stdout, "           share_flags: %08lx\n", msg->share_flags);
  fprintf(stdout, "          capabilities: %08lx\n", msg->capabilities);
  fprintf(stdout, "        maximal_access: %08lx\n", msg->maximal_access);
}

static void dump_error(const smb2_error_response *msg)
{
  fprintf(stdout, "        structure_size: %04x\n", msg->structure_size);
  fprintf(stdout, "              reserved: %04x\n", msg->reserved);
  fprintf(stdout, "             bytecount: %08lx\n", msg->bytecount);

  fprintf(stdout, "           error_data:\n");
  hexdump((const char *)msg + sizeof(smb2_error_response), msg->bytecount);
}

static void dump_response(const smb_response *msg)
{
  if (!msg) return;
  dump_header(&msg->header);

  if (is_error(msg))
  {
    dump_error(&msg->body.error);
    return;
  }

  switch (msg->header.command)
  {
    case SMB2_NEGOTIATE:
      dump_negotiate(&msg->body.negotiate);
      break;

    case SMB2_SESSION_SETUP:
      dump_setup(&msg->body.setup);
      break;

    case SMB2_TREE_CONNECT:
      dump_tree_connect(&msg->body.tree_connect);
      break;

    default:
      break;

  } 
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


static uint16_t *cstring_to_unicode(const char *str)
{
  unsigned length = strlen(str);
  uint16_t *path;
  unsigned i;

  path = (uint16_t *)malloc(length * 2 + 4);

  for (i = 0; i < length; ++i)
  {
    path[i + 1] = str[i];
  }
  path[0] = length;
  path[length + 1] = 0;

  return path;
}


static uint16_t *pstring_to_unicode(const char *str)
{
  unsigned length = str[0];
  uint16_t *path;
  unsigned i;

  path = (uint16_t *)malloc(length * 2 + 4);

  for (i = 0; i < length; ++i)
  {
    path[i + 1] = str[i + 1];
  }
  path[0] = length;
  path[length + 1] = 0;

  return path;
}


static uint16_t *gsstring_to_unicode(const GSString255 *str)
{
  unsigned length = str->length;
  uint16_t *path;
  unsigned i;

  path = (uint16_t *)malloc(length * 2 + 4);

  for (i = 0; i < length; ++i)
  {
    path[i + 1] = str->text[i];
  }
  path[0] = length;
  path[length + 1] = 0;

  return path;
}


unsigned has_spnego = 0;
unsigned has_mech_ntlmssp = 0;

uint32_t ntlmssp_challenge[2] = { 0, 0 };
uint32_t ntlmssp_flags = 0;

// returns the new offset.
unsigned scan_asn1(const uint8_t *data, unsigned offset, unsigned length)
{
  // '1.3.6.1.5.5.2'
  static uint8_t kSPNEGO[] = {0x2B, 0x06, 0x01, 0x05, 0x05, 0x02}; 

  // '1.3.6.1.4.1.311.2.2.10'
  static uint8_t kNTLMSSP[] = {0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x02, 0x0A}; 

  static uint8_t kType2[] = {
    'N', 'T', 'L', 'M', 'S', 'S', 'P', 0,
    0x02, 0x00, 0x00, 0x00
  };
  unsigned tag;
  unsigned len;

restart:

  tag = data[offset++];
  len = data[offset++];

  if (offset >= length) return offset;

  if (len >= 0x80) {
    switch (len & 0x7f)
    {
      case 1:
        len = data[offset++];
        break;
      case 2:
        len = data[offset++] << 8;
        len += data[offset++];
        break;

      default:
        // oops, too big.
        return length;
    }
  }

  fprintf(stdout, "%02x %02x\n", tag, len);

  switch(tag)
  {
  case ASN1_SEQUENCE: // sequence
  case ASN1_APPLICATION: // application
    while (offset < length)
      offset = scan_asn1(data, offset, length);
    break;

  case ASN1_CONTEXT:
  case ASN1_CONTEXT+1:
  case ASN1_CONTEXT+2:
  case ASN1_CONTEXT+3:
    // jump back to the start?
    //return scan_asn1(data, offset, length);
    goto restart;
    break;

  case ASN1_OID: // oid
    if (len == 6 && memcmp(data + offset, kSPNEGO, 6) == 0)
    {
      //fprintf(stdout, "spnego!\n");
      has_spnego = true;
      break;
    }
    if (len == 0x0a && memcmp(data + offset, kNTLMSSP, 0x0a) == 0)
    {
      //fprintf(stdout, "ntlmssp!\n");
      has_mech_ntlmssp = true;
      break;
    }
    break;

  case ASN1_OCTECT_STRING:
    // get the challenge bytes.
    if (len >= 32 && memcmp(data + offset, kType2, sizeof(kType2)) == 0)
    {
      ntlmssp_flags = *(uint32_t *)(data  + offset + 20);
      ntlmssp_challenge[0] = *(uint32_t *)(data  + offset + 24);
      ntlmssp_challenge[1] = *(uint32_t *)(data  + offset + 28);
      // domain, server name, etc skipped for now.
    }
    break;

  }

  return offset + len;
}

int negotiate(Word ipid, uint16_t *path)
{
  static struct smb2_negotiate_request negotiate_req;
  static struct smb2_session_setup_request setup_req;
  static struct smb2_tree_connect_request tree_req;

  static uint16_t dialects[] = { 0x0202 };

  #define DATA_SIZE 16
  static uint8_t setup1[] = {
    ASN1_APPLICATION, DATA_SIZE + 32, // size,
    ASN1_OID, 0x06, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x02, // spnego
    ASN1_CONTEXT, DATA_SIZE + 22, // size
    ASN1_SEQUENCE, DATA_SIZE + 20, //size
    ASN1_CONTEXT, 0x0e, // size
    ASN1_SEQUENCE, 0x0c, //size
    ASN1_OID, 0x0a,
    0x2B, 0x06, 0x01, 0x04, 0x01, 0x82, 0x37, 0x02, 0x02, 0x0A, // ntlmssp
    ASN1_CONTEXT + 2, DATA_SIZE + 2, // size
    ASN1_OCTECT_STRING, DATA_SIZE, // size
    // data...
    'N', 'T', 'L', 'M', 'S', 'S', 'P', 0x00,
    0x01, 0x00, 0x00, 0x00, // uint32_t - 0x01 negotiate
    0x02, 0x02, 0x00, 0x00 // flags - oem, ntlm
  };
  #undef DATA_SIZE

  uint16_t tmp;



  smb_response *responsePtr;


  Handle h;

  uint32_t size = 0;



  memset(&header, 0, sizeof(header));
  memset(&negotiate_req, 0, sizeof(negotiate_req));
  memset(&setup_req, 0, sizeof(setup_req));
  memset(&tree_req, 0, sizeof(tree_req));


  header.protocol_id = SMB2_MAGIC; // '\xfeSMB';
  header.structure_size = 64;
  header.command = SMB2_NEGOTIATE;

  negotiate_req.structure_size = 36;
  negotiate_req.dialect_count = 1; // ?
  negotiate_req.security_mode = SMB2_NEGOTIATE_SIGNING_ENABLED;
  negotiate_req.capabilities = 0;

  //negotiate_req.dialects[0] = 0x202; // smb 2.002


  write_message(ipid, &negotiate_req, sizeof(negotiate_req), dialects, sizeof(dialects));

  // read a response...

  h = read_response(ipid);
  if (!h) return -1;
  HLock(h);

  //hexdump(*h, GetHandleSize(h));

  responsePtr = *(smb_response **)h;

  dump_response(responsePtr);

  if (responsePtr->header.protocol_id != SMB2_MAGIC || responsePtr->header.structure_size != 64)
  {
    DisposeHandle(h);
    fprintf(stderr, "Invalid SMB2 header\n");
    return -1;
  }

  if (responsePtr->header.command != SMB2_NEGOTIATE)
  {
    DisposeHandle(h);
    fprintf(stderr, "Unexpected SMB2 command\n");
    return -1;
  }

  // the security buffer is asn.1.  This checks for spnego/ntlmssp.
  // probably not necessary... what else are we going to do?


  has_spnego = false;
  has_mech_ntlmssp = false;

  tmp = responsePtr->body.negotiate.security_buffer_length;
  if (tmp)
  {

    scan_asn1((const char *)responsePtr + responsePtr->body.negotiate.security_buffer_offset,
      0, tmp);

    if (flags._v)
    {
      fprintf(stdout, "has_spnego: %d\n", has_spnego);
      fprintf(stdout, "has_mech_ntlmssp: %d\n", has_mech_ntlmssp);
    }

  }

  DisposeHandle(h);


  // 

  // 
  header.command = SMB2_SESSION_SETUP;

  setup_req.structure_size = 25;
  setup_req.flags = 0;
  setup_req.security_mode = SMB2_NEGOTIATE_SIGNING_ENABLED;
  setup_req.capabilities = 0;
  setup_req.security_buffer_length = sizeof(setup1);
  setup_req.security_buffer_offset = sizeof(smb2_header_sync) + sizeof(smb2_session_setup_request);

  write_message(ipid, &setup_req, sizeof(setup_req), setup1, sizeof(setup1));


  h = read_response(ipid);
  if (!h) return -1;
  HLock(h);

  responsePtr = *(smb_response **)h;

  dump_response(responsePtr);

  if (responsePtr->header.protocol_id != SMB2_MAGIC || responsePtr->header.structure_size != 64)
  {
    DisposeHandle(h);
    fprintf(stderr, "Invalid SMB2 header\n");
    return -1;
  }

  if (responsePtr->header.command != SMB2_SESSION_SETUP)
  {
    DisposeHandle(h);
    fprintf(stderr, "Unexpected SMB2 command\n");
    return -1;
  }

  header.session_id[0] = responsePtr->header.session_id[0];
  header.session_id[1] = responsePtr->header.session_id[1];


  ntlmssp_challenge[0] = 0;
  ntlmssp_challenge[1] = 0;
  ntlmssp_flags = 0;
  has_spnego = false;
  has_mech_ntlmssp = false;

  tmp = responsePtr->body.setup.security_buffer_length;
  if (tmp)
  {

    scan_asn1((const char *)responsePtr + responsePtr->body.setup.security_buffer_offset,
      0, tmp);

    if (flags._v)
    {
      fprintf(stdout, "has_spnego: %d\n", has_spnego);
      fprintf(stdout, "has_mech_ntlmssp: %d\n", has_mech_ntlmssp);
      fprintf(stdout, "ntlmssp_flags: %08lx\n", ntlmssp_flags);
      fprintf(stdout, "ntlmssp_challenge: %08lx%08lx\n", 
        ntlmssp_challenge[1], ntlmssp_challenge[0]);
    }

  }



  DisposeHandle(h);


  // send a second session_setup if STATUS_MORE_PROCESSING_REQUIRED


  header.command = SMB2_TREE_CONNECT;

  tree_req.structure_size = 9;
  tree_req.path_offset = sizeof(smb2_header_sync) + sizeof(smb2_tree_connect_request);
  tree_req.path_length = path[0] * 2;

  write_message(ipid, &tree_req, sizeof(tree_req), path + 1, path[0] * 2);


  h = read_response(ipid);
  if (!h) return -1;
  HLock(h);

  responsePtr = *(smb_response **)h;

  dump_response(responsePtr);

  if (responsePtr->header.protocol_id != SMB2_MAGIC || responsePtr->header.structure_size != 64)
  {
    DisposeHandle(h);
    fprintf(stderr, "Invalid SMB2 header\n");
    return -1;
  }

  if (responsePtr->header.command != SMB2_TREE_CONNECT)
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
  uint16_t *path;
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


  path = cstring_to_unicode("\\\\192.168.1.254\\public");
  ok = negotiate(connection.ipid, path);
  if (ok) return ok;



  CloseLoop(&connection);

  free(host);
  return 0;
}
