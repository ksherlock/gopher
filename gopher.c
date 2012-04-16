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
#include <errno.h>

#include <unistd.h>

extern int setfiletype(const char *filename);

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
  
  // TCPIP is an init, not a tool, so it should always
  // be loaded.
  
  status = TCPIPStatus();
  if (_toolErr)
  {
    LoadOneTool(54, 0x0300);
    if (_toolErr) return -1;

    status = 0;
    flags |= kLoaded;
  }

#if 0
  // require 3.0b3
  if (TCPIPLongVersion() < 0x03006003)
  {
    if (flags & kLoaded)
      UnloadOneTool(54);
    return -1;      
  }
#endif

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
  // text \r\n
  // ...
  // . \r\n
  // any leading '.' must be doubled.

  Word eof = 0;
  int rv = 0;
  Word lastTerm = 0;

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
        if (lastTerm == '\r' && rb.terminator == '\n')
        {
            /* nothing */
        }
        else 
            fputc('\r', file);
      }

      lastTerm = rb.terminator;
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


  // blank lines are ignored, so no need to check for terminator split.
  
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

void do_gopher(const char *url, URLComponents *components, FILE *file)
{
  Connection buffer;
  char *host;
  char type;
  
  LongWord qtick;

  if (!components->portNumber) components->portNumber = 70;
  
  host = malloc(components->host.length + 1);
  URLComponentGetC(url, components, URLComponentHost, host);

  ConnectionInit(&buffer, MMStartUp());
  
  ConnectionOpenC(&buffer, host,  components->portNumber);

  // 30 second timeout.
  qtick = GetTick() + 30 * 60;
  while (!ConnectionPoll(&buffer))
  {
    if (GetTick() >= qtick)
    {
      fprintf(stderr, "Connection timed out.\n");
      // todo -- still need to close it...
      free(host);
      return;
    }
  }

  if (buffer.state == kConnectionStateError)
  {
    fprintf(stderr, "Unable to open host: %s:%u\n", 
      host, 
      components->portNumber);
    free(host);
    return;
  }

  // connected....
  
  // path is /[type][resource]
  // where [type] is 1 char and the leading / is ignored.
  
  if (components->path.length <= 1)
  {
    // / or blank
    type = '1'; // directory
  }
  else if (components->path.length == 2)
  {
    // / type
    // invalid -- treat as /
    type = '1';
  }
  else
  {
    type = url[components->path.location+1];
    TCPIPWriteTCP(
      buffer.ipid, 
      url + components->path.location + 2, 
      components->path.length - 2,
      0,
      0);
  }
  // 
  TCPIPWriteTCP(buffer.ipid, "\r\n", 2, true, 0);

  // 5 and 9 are binary, 1 is dir, all others text.

  switch(type)
  {
  case '1':
    gopher_dir(buffer.ipid, file);
    break;
  case '5':
  case '9':
    fsetbinary(file);
    gopher_binary(buffer.ipid, file);
    break;
  default:
    gopher_text(buffer.ipid, file);
    break;
  }

  fflush(file);

  ConnectionClose(&buffer);

  while (!ConnectionPoll(&buffer)) ; // wait for it to close.

  free (host);
}

void help(void)
{

  fputs("gopher [options] url\n", stdout);
  fputs("-h         display help information.\n", stdout);
  fputs("-v         display version information.\n", stdout);
  fputs("-O         write output to file.\n", stdout);
  fputs("-o <file>  write output to <file> instead of stdout.\n", stdout);
  fputs("\n", stdout);
  
  exit(0);
}

/*
 *
 *
 */

char *get_url_filename(const char *cp, URLComponents *components)
{
    URLRange path;
    int slash;
    int i, j;
    char *out;
    
    path = components->path;
    
    if (path.length <= 0) return NULL;
    
    cp += path.location;
    
    // scan the path, for the last '/'
    slash = -1;
    for (i = 0; i < path.length; ++i)
    {
      if (cp[i] == '/') slash = i;
    }
    
    if (slash == -1 || slash + 1 >= path.length) return NULL;
    
    
    out = (char *)malloc(path.length - slash);
    if (!out) return NULL;
    
    j = 0;
    i = slash + 1; // skip the slash.
    while (i < path.length)
      out[j++] = cp[i++];
    
    out[j] = 0; // null terminate.
    
    return out;
}

int main(int argc, char **argv)
{
  int i;
  Word flags;
  int ch;
  char *filename = NULL;
  int flagO = 0;
  
  flags = StartUp(NULL);

  while ((ch = getopt(argc, argv, "o:Oh")) != -1)
  {
    switch (ch)
    {
    case 'v':
        fputs("gopher v 0.1\n", stdout);
        exit(0);
        break;
        
    case 'o':
        filename = optarg;
        break;
    
    case 'O':
        flagO = 1;
        break;
    
    case 'h':
    case '?':
    case ':':
    default:
        help();
        break;
    }
  
  }
  
  argc -= optind;
  argv += optind;

  if (argc != 1)
  {
    help();
  }

  if (argc == 1)
  {
    const char *url;
    URLComponents components;

    url = *argv;
    
    if (!ParseURL(url, strlen(url), &components))
    {
      fprintf(stderr, "Invalid URL: %s\n", url);
      exit(1);
    }
   
    if (!components.host.length)
    {
      fprintf(stderr, "No host.\n");
      exit(1);
    }
    
    if (components.schemeType == SCHEME_GOPHER)
    {
        FILE *file = NULL;
        
        if (!components.portNumber) components.portNumber = 70;
        
        if (filename)
        {
            file = fopen(filename, "w");
            if (!file)
            {
                fprintf(stderr, "Unable to open file ``%s'': %s", 
                  filename, strerror(errno));
                exit(1);
            }
            setfiletype(filename);
        }
        else if (flagO)
        {
            // get the file name from the URL.
            
            filename = get_url_filename(url, &components);
            if (!filename)
            {
                fprintf(stderr, "-O flag cannot be used with this URL.\n");
                exit(1);
            }
            
            file = fopen(filename, "w");
            if (!file)
            {
                fprintf(stderr, "Unable to open file ``%s'': %s", 
                  filename, strerror(errno));
                exit(1);            
            }
            
            setfiletype(filename);
            
            free(filename);
            filename = NULL;
        }
        else file = stdout;
        
        do_gopher(url, &components, file);
        
        if (file != stdout) fclose(file);
    }
    else
    {
        fprintf(stderr, "Unsupported scheme.\n");
        exit(1);
    }
  }

  ShutDown(flags, false, NULL);

  return 0;
}

