#pragma optimize 79

#include <Locator.h>
#include <Memory.h>
#include <MiscTool.h>
#include <tcpip.h>

#include "url.h"
#include "connection.h"
#include "readline2.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifndef ORCA_BUILD
#include <unistd.h>
#endif

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
    fwrite(buffer, 1, count, file);

  }
 
  return rv;
}

int gopher_text(Word ipid, FILE *file)
{
  Word lines[2] = {0, 0};
  // text \r\n
  // ...
  // . \r\n
  // any leading '.' must be doubled.

  Word eof = 0;
  int rv = 0;

  TCPIPPoll();

  for(;;)
  { 
    Word count;
    Handle h;

    rlBuffer rb;

    rv = ReadLine2(ipid, &rb);

    h = rb.bufferHandle;
    count = rb.bufferSize;

    if (rv < 0) break; // eof
    if (rv == 0)  // no data available (yet)
    {
      TCPIPPoll();
      continue;
    }

    if (!rb.moreFlag) TCPIPPoll();


    if (count == 0)
    {
      DisposeHandle(h);
      if (rb.terminator)
      {
        fputc('\r', file);
      }

      continue;
    }

    if (count)
    {
      char *cp;

      HLock(h);
      cp = *((char **)h);

      // .. -> . 
      // . \r\n -> eof

      if (*cp == '.')
      {
        if (count == 1)
        {
          DisposeHandle(h);
          eof = 1;
          break;
        }

        cp++;
        count--;
      }

      fwrite(cp, 1, count, file);
      fputc('\r', file);
    }

    DisposeHandle(h);
  }

  if (!eof)
    fprintf(stderr, "Warning: eof not found.\n");

  return eof ? 0 : -1;
}

int gopher_dir(Word ipid, FILE *file)
{
  Word eof = 0;
  int rv = 0;

  TCPIPPoll();

  for(;;)
  {
    rlBuffer rb;
    Word count;
    Handle h;


    rv = ReadLine2(ipid, &rb);

    h = rb.bufferHandle;
    count = rb.bufferSize;

    if (rv < 0) break;
    if (rv == 0)
    {
      TCPIPPoll();
      continue;
    }
    if (!rb.moreFlag) TCPIPPoll();
 

    if (!count) 
    {
      // blank line?
      continue; 
    }

    if (count)
    {

      Word tabs[4];
      unsigned i,j;
      char type;
      char *buffer;

      HLock(h);
      buffer = *((char **)h);
     
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
          fwrite(buffer, 1, count, file);
        else 
          fputs(buffer, file);

        fputc('\r', file);
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
          fprintf(file, "[%s/%c%s]  %s\r",
            buffer + 1 + tabs[2], // server
            type, // type
            buffer + 1 + tabs[1], // file name
            buffer // description
          );
        }
        else
        {
          fprintf(file, "[%s:%u/%c%s]  %s\r",
            buffer + 1 + tabs[2], // server
            port, // port
            type, // type
            buffer + 1 + tabs[1], // file name
            buffer // description
          );
        }

      }
      
      DisposeHandle(h);
    }
  }

  if (!eof)
    fprintf(stderr, "Warning: eof not found.\n");

  return eof ? 0 : -1;
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

#ifdef ORCA_BUILD
  file = stdout; 
#else
  file = fdopen(STDOUT_FILENO, "wb");
#endif

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

