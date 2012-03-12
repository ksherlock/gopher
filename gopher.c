#include <Locator.h>
#include <Memory.h>

#include <tcpip.h>


#include "url.h"
#include "connection.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * connect gopher.floodgap.com:70
 * send path
 * read output.
 * text -- ends with . <CR><LF>
 * bin  -- ends when connection is closed.
 */

// startup/shutdown flags.
enum {
  kLoaded = 1,
  kStarted = 2,
  kConnected = 4
};

Word StartUp(displayPtr fx)
{
  word status;
  word flags = 0;
  
  status = TCPIPStatus();
  if (_toolErr)
  {
    LoadOneTool(54, 0x0201);
    if (_toolErr) return -1;

    status = 0;
    flags |= kLoaded;
  }

  if (!status)
  {
    TCPIPStartUp();
    if (_toolErr) return -1;
    flags |= kStarted;
  }

  status = TCPIPGetConnectStatus();
  if (!status)
  {
    TCPIPConnect(fx);
    flags |= kConnected;
  }

  return flags;
}

void ShutDown(word flags, Boolean force, displayPtr fx)
{
    if (flags & kConnected)
    {
      TCPIPDisconnect(force, fx);
      if (_toolErr) return;
    }
    if (flags & kStarted)
    {
      TCPIPShutDown();
      if (_toolErr) return;
    }
    if (flags & kLoaded)
    {
      UnloadOneTool(54);
    }
}


int gopher_binary(Word ipid, FILE *file)
{
  // gopher binary support.
  // format: raw data until eof.
  Word rv = 0;

  for(;;)
  { 
    static char buffer[512];
    rrBuff rb;
    Word count;

    TCPIPPoll();
    rv = TCPIPReadTCP(ipid, 0, (Ref)buffer, 512, &rb);

    count = rb.rrBuffCount;
    if (rv == 0 && count == 0) continue;

    if (rv && !count) break;

    if (!count) continue;
    fwrite(buffer, count, 1, file);

  }
 
  return rv;
}

int gopher_text(Word ipid, FILE *file)
{
  // text \r\n
  // ...
  // . \r\n
  // any leading '.' must be doubled.

  enum {
    kStateText = 0,
    kStateCR,
    kStateEOL,
    kStateDot,
    kStateDotCR,
    kStateEOF
  };

  static char buffer[256];
  unsigned state = kStateEOL;
  int rv = 0;


  // I have bad luck with ReadLineTCP.
  while (state != kStateEOF)
  {
    static char buffer[512];
    rrBuff rb;
    Word count;
    Word i;

    TCPIPPoll();
    rv = TCPIPReadTCP(ipid,  0, (Ref)buffer, 512, &rb);

    count = rb.rrBuffCount;
    if (rv == 0 && count == 0) continue;

    //fprintf(stderr, "rv = %x, count = %u\n", rv, rb.rrBuffCount); 

    //if (rv == tcperrConClosing) rv = 0;
    if (rv && !count) break;
    if (!count) continue;

    // scan the buffer for a line.
    for (i = 0; i < count; ++i)
    {
      char c = buffer[i];
    
      if (c == '.')
      {
        if (state == kStateEOL) state = kStateDot;
        else fputc(c, file); 
        continue;
      }
      if (c == '\r')
      {
        if (state == kStateDot) state = kStateDotCR;
        else state = kStateCR;
        continue;
      }
      if (c == '\n')
      {
         if (state == kStateCR)
         {
           state = kStateEOL;
           fputc('\r', file);
         }
         else if (state == kStateDotCR)
         {
           state = kStateEOF;
           break;
         }
         // otherwise, silently drop?
         continue;
      }

      {
         state = kStateText; // reset if kStateDot.
         // . and \r will be silently dropped
         fputc(c, file);
       }
    }
  }
  if (state != kStateEOF)
    fprintf(stderr, "Warning: eof not found.\n");

  return rv;
}

int gopher_dir(Word ipid, FILE *file)
{
  Word eof = 0;
  Word rv = 0;


  for(;;)
  {
    Word count;
    Handle h;

#if 1
    rlrBuff rb;

    TCPIPPoll();

    rv = TCPIPReadLineTCP(ipid,
      "\p\r\n",
      2,
      NULL,
      0xffff,
      &rb);

    count = rb.rlrBuffCount;
    h = rb.rlrBuffHandle;
#else
    rrBuff rb;

    TCPIPPoll();

    rv = TCPIPReadTCP(ipid, 2, NULL, 0xffff, &rb);

    count = rb.rrBuffCount;
    h = rb.rrBuffHandle;
#endif

#if 0
    if (rv || count || _toolErr) 
    {
      fprintf(stderr, "rv = %x, _toolErr = %u, count = %u\n", 
        rv, _toolErr, count);
    }
#endif

    if (rv && !count) break;

    if (count)
    {

      Word tabs[4];
      unsigned i,j;
      char type;
      char *buffer = *((char **)h);
     
      type = *buffer;
      ++buffer;
      --count;

      if (type == '.' && count == 0)
      {
        eof = 1;
        DisposeHandle(h);
        break;
      }
      // format is [type][name] \t [path] \t [server] \t [port]
      j = 1;
      tabs[0] = 0;
      tabs[1] = tabs[2] = tabs[3] = -1;

      for (i = 0; i < count; ++i)
      {
        if (buffer[i] == '\t')
        {
          tabs[j++] = i;
          buffer[i] = 0;
          if (j == 4) break;
        }
      }
      
      // 'i' is info record.
      if (type == 'i')
      {
        if (tabs[1] == -1)
          fprintf(file, "%.*s\r", count, buffer);
        else fprintf(file, "%s\r", buffer);
      }
      else if (j == 4) // all the tabs.
      {
        int port = 0;
        // description
        // file name
        // server
        // port
        for (i = tabs[3] + 1; i < count; ++i)
        {
          char c = buffer[i];
          if (c >= '0' && c <= '9') port = port * 10 + c - '0';
        }
        if (port == 70) 
        {
          fprintf(file, "%s <%s/%c%s>\r",
            buffer, // description
            buffer + 1 + tabs[2], // server
            type, // type
            buffer + 1 + tabs[1] // file name
          );
        }
        else
        {
          fprintf(file, "%s <%s:%u/%c%s>\r",
            buffer, // description
            buffer + 1 + tabs[2], // server
            port, // port
            type, // type
            buffer + 1 + tabs[1] // file name
          );
        }

      }
      
      DisposeHandle(h);
    }
  }

  if (!eof)
    fprintf(stderr, "Warning: eof not found.\n");

  return rv;
}

void do_url(const char *url)
{
  URLComponents components;
  Connection buffer;
  char *host;
  char type;
  FILE *file;

  if (!ParseURL(url, strlen(url), &components))
  {
    fprintf(stderr, "Invalid URL: %s\n", url);
    return;
  }

  if (!components.host.length)
  {
    fprintf(stderr, "No host\n");
    return;
  }


  if (!components.portNumber) components.portNumber = 70;

  host = malloc(components.host.length + 1);
  URLComponentGetC(url, &components, URLComponentHost, host);

  ConnectionInit(&buffer, MMStartUp());
  
  ConnectionOpenC(&buffer, host,  components.portNumber);

  while (!ConnectionPoll(&buffer)) ;

  if (buffer.state == kConnectionStateError)
  {
    fprintf(stderr, "Unable to open host: %s:%u\n", 
      host, 
      components.portNumber);
    free(host);
    return;
  }

  // connected....
  
  // path is /[type][resource]
  // where [type] is 1 char and the leading / is ignored.
  
  if (components.path.length <= 1)
  {
    // / or blank
    type = '1'; // directory
  }
  else if (components.path.length == 2)
  {
    // / type
    // invalid -- treat as /
    type = '1';
  }
  else
  {
    type = url[components.path.location+1];
    TCPIPWriteTCP(
      buffer.ipid, 
      url + components.path.location + 2, 
      components.path.length - 2,
      0,
      0);
  }
  // 
  TCPIPWriteTCP(buffer.ipid, "\r\n", 2, true, 0);

  // 5 and 9 are binary, 1 is dir, all others text.

  file = fdopen(STDOUT_FILENO, "wb");

  switch(type)
  {
  case '1':
    gopher_dir(buffer.ipid, file);
    break;
  case '5':
  case '9':
    gopher_binary(buffer.ipid, file);
    break;
  default:
    gopher_text(buffer.ipid, file);
    break;
  }

  fflush(file);
  fclose(file);

  ConnectionClose(&buffer);

  while (!ConnectionPoll(&buffer)) ; // wait for it to close.

  free (host);
}


int main(int argc, char **argv)
{
  int i;
  Word flags;

  flags = StartUp(NULL);

  for (i = 1; i < argc; ++i)
  {
    do_url(argv[i]);
  }

  ShutDown(flags, false, NULL);

  return 0;
}

