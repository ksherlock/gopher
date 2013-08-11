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

#include <unistd.h>

#include "url.h"
#include "connection.h"
#include "readline2.h"
#include "prototypes.h"
#include "flags.h"

#include "s16debug.h"

static FileInfoRecGS FileInfo;
static Word FileAttr;

static int gopher_binary(Word ipid, FILE *file)
{
  ReadBlock dcb;
  int ok;

  ok = read_binary(ipid, file, &dcb);

  return ok;
}

/*
 * connect gopher.floodgap.com:70
 * send path
 * read output.
 * text -- ends with . <CR><LF>
 * bin  -- ends when connection is closed.
 */

static int gopher_text(Word ipid, FILE *file)
{
  // text \r\n
  // ...
  // . \r\n
  // any leading '.' must be doubled.

  Word eof = 0;
  int rv = 0;
  Word lastTerm = 0;

  IncBusy();
  TCPIPPoll();
  DecBusy();
  
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
      IncBusy();
      TCPIPPoll();
      DecBusy();
      
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
            lastTerm = TERMINATOR_CR_LF;
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

static int gopher_dir(Word ipid, FILE *file)
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
      IncBusy();
      TCPIPPoll();
      DecBusy();
      continue;
    }
    if (!rb.moreFlag)
    {
      IncBusy();
      TCPIPPoll();
      DecBusy();
    }

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

int do_gopher(const char *url, URLComponents *components)
{
  Connection connection;
  char *host;
  char *path;
  char type;
  int ok;
  
  FILE *file;
  char *filename;
  
  file = stdout;
    

  FileAttr = 0;
  memset(&FileInfo, 0, sizeof(FileInfo));
    
  if (!components->portNumber) components->portNumber = 70;
  
  host = URLComponentGetCMalloc(url, components, URLComponentHost);
  path = URLComponentGetCMalloc(url, components, URLComponentPath);
  
  if (!host)
  {
    fprintf(stderr, "URL `%s': no host.", url);
    free(path);
    return -1;
  }  

  if (path && components->path.length <= 2)
  {
    free(path);
    path = NULL;
  }

  // open the file.
  filename = NULL;
  if (flags._o)
  {
     filename = flags._o;
     if (filename && !filename[0])
       filename = NULL;
     if (filename && filename[0] == '-' && !filename[1])
       filename = NULL;
  }
  
  if (flags._O)
  {    
    if (path)
    {
        filename = strrchr(path + 2, '/');
        if (filename) // *filename == '/'
        {
            filename++;
        }
        else
        {
            filename = path + 2;
        }
    }
    
    if (!filename || !filename[0])
    {
        // path/ ?
        fprintf(stderr, "-O flag cannot be used with this URL.\n");
        free(host);
        free(path);        
        return -1;            
    }   
    
  }
  
  if (filename)
  {
      file = fopen(filename, "w");
      if (!file)
      {
        fprintf(stderr, "Unable to to open file ``%s'': %s\n",
          filename, strerror(errno));
          
        free(host);
        free(path);
        return -1; 
      }
      
      if (parse_extension_c(filename, &FileInfo.fileType, &FileInfo.auxType))
      {
        FileAttr |= ATTR_FILETYPE | ATTR_AUXTYPE;
        setfileattr(filename, &FileInfo, FileAttr);
      }
  }


  ok = ConnectLoop(host, components->portNumber, &connection);
  
  if (!ok)
  {
    free(host);
    free(path);
    if (file != stdout) fclose(file);
    return -1;
  }


  // connected....
  
  // path is /[type][resource]
  // where [type] is 1 char and the leading / is ignored.

  IncBusy();
  
  if (path)
  {
    // path[0] = '/'
    // path[1] = type
    type = path[1];
    TCPIPWriteTCP(
      connection.ipid, 
      path + 2, 
      components->path.length - 2,
      false,
      false);
  }
  else
  {
    type = 1;
  }
  // 
  TCPIPWriteTCP(connection.ipid, "\r\n", 2, true, false);
  DecBusy();



  // 5 and 9 are binary, 1 is dir, all others text.

  switch(type)
  {
  case '1':
    gopher_dir(connection.ipid, file);
    break;
  case '5':
  case '9':
    fsetbinary(file);
    ok = gopher_binary(connection.ipid, file);
    break;
  default:
    ok = gopher_text(connection.ipid, file);
    break;
  }

  fflush(file);
  if (file != stdout) fclose(file);

  CloseLoop(&connection);
  free(host);
  free(path);

  return 0;
}





