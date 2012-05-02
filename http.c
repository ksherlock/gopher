
/*
 * HTTP 1.0
 *
 * request:
 * verb url version CR LF
 * header CR LF *
 * CR LF
 *
 * response:
 * status CR LF
 * header CR LF *
 * CR LF
 * data
 */

#pragma optimize 79
#pragma noroot
 
#include <TCPIP.h>
#include <MiscTool.h>
#include <Memory.h>
#include <IntMath.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
 
#include "url.h"
#include "connection.h"
#include "prototypes.h"
#include "dictionary.h"
#include "flags.h"
#include "readline2.h"
#include "http.utils.h"
#include "s16debug.h"

static int do_http_0_9(
  const char *url, 
  URLComponents *components, 
  Word ipid, 
  FILE *file, 
  const char *filename)
{
  int ok;
  
  char *cp;
  int length;
      
  
  TCPIPWriteTCP(ipid, "GET ", 4, false, false);

  length = components->pathAndQuery.length;
  cp = url + components->pathAndQuery.location;

  if (!length)
  {
    length = 1;
    cp = "/";
  }

  TCPIPWriteTCP(ipid, cp, length, false, false);
  
  TCPIPWriteTCP(ipid, "\r\n", 2, true, false);

  ok = read_binary(ipid, file);
  return 0;
}


static int parseHeaders(Word ipid, FILE *file, Handle dict)
{
  int prevTerm = 0;
  int term = 0;
  int line;
  int status = -1;
  
  // todo -- timeout?
  
  /*
   * HTTP/1.1 200 OK <CRLF>
   * header: value <CRLF>
   * header: value <CRLF>
   * ...
   * <CRLF>
   */

  
  line = 0;
  
  TCPIPPoll();
  
  for (;;)
  {
      rlBuffer rb;
      Handle h;
      char *cp;
      int ok;
      
      ok = ReadLine2(ipid, &rb);
      
      if (ok == 0)
      {
        TCPIPPoll();
        continue;
      }
      
      // eof = error.
      if (ok == -1) return -1;
      
      // could be end of header...
      // or a split cr/lf
      if (rb.bufferSize == 0)
      {
        if (rb.terminator == '\n' && prevTerm == '\r')
        {
          term = TERMINATOR_CR_LF;
          prevTerm = TERMINATOR_CR_LF;
          continue;
        }
        
        // end of header.
        // TODO -- if term is CRLF and only a CR was read, 
        // need to yank it later.
        // if rb.terminater == '\r' && term == TERMINATOR_CR_LF ...
        
        if (rb.terminator)
        {
            if (flags._i || flags._I)
                fputc('\r', file);
        }
        
        if (line == 0) return -1; 
        
        break;
      }
      
      prevTerm = rb.terminator;
      
      h = rb.bufferHandle;
      HLock(h);
      
      cp = *(char **)h;

      if (flags._i || flags._I)
      {
        fwrite(cp, 1, rb.bufferSize, file);
        fputc('\r', file);
      }
      
            
      // line 1 is the status.
      if (++line == 1)
      {
        // HTTP/1.1 200 OK
        int i, l;
        int version;
        
        term = rb.terminator;
      
        if (parseStatusLine(cp, rb.bufferSize, &version, &status))
        {
            /* ok... */
        }
        else
        {
          fprintf(stderr, "Bad HTTP status: %.*s\n", (int)rb.bufferSize, cp);
          DisposeHandle(h);
          return -1;
        }
      
      }
      else
      {
        // ^([^\s:]+):\s*(.*)$
        // ^\s+*(.*)$ -- continuation of previous line (ignored)

        URLRange key, value;
        
        if (parseHeaderLine(cp, rb.bufferSize, &key, &value))
        {
          if (key.length)
          {
            DictionaryAdd(dict, 
              cp + key.location, key.length,
              cp + value.location, value.length,
              false);
          }
        
        }
      }
      
      DisposeHandle(h);
  }
  
  
  
  return status;
}  

enum {
    TE_unknown = -1,
    TE_identity = 0,
    TE_chunked
};

int read_response(Word ipid, FILE *file, Handle dict)
{
    // get the file size and content encoding 
    // from the dict.
    // todo -- check for text/* ?
    // -m <mimetype>?
    
    
    char *value;
    Word valueSize;
    int transferEncoding;
    
    LongWord contentSize;
    
    
    contentSize = 0;
    transferEncoding = -1;
    value = DictionaryGet(dict, "Content-Length",  14, &valueSize);
    
    if (value)
    {
        contentSize = Dec2Long(value, valueSize, 0);
        transferEncoding = 0;
    }
    
    value = DictionaryGet(dict, "Transfer-Encoding", 17, &valueSize);
    if (value)
    {
        Word *wp = (Word *)value;
        // id en ti ty ?
        // ch un ke d ?
        if (valueSize == 8
            && (wp[0] | 0x2020) == 0x6469
            && (wp[1] | 0x2020) == 0x6e65
            && (wp[2] | 0x2020) == 0x6974
            && (wp[3] | 0x2020) == 0x7974
        )
        {
            transferEncoding = 0;
        }
        else if (valueSize == 7
            && (wp[0] | 0x2020) == 0x6863
            && (wp[1] | 0x2020) == 0x6e75
            && (wp[2] | 0x2020) == 0x656b
            && (value[6] | 0x20) == 0x64
        )
        {
            transferEncoding = 1;
        }
        else
        {
            fprintf(stderr, "Unsupported Transfer Encoding: %.*s\n",
                valueSize, value);
                
            return -1;
        }    
    }
    
    // ?
    if (transferEncoding == -1)
    {
        fprintf(stderr, "Content Size not provided.\n");
        return -1;
    }
    
    
    if (transferEncoding == 0)
    {
        // identity.
        // just read contentLength bytes.
        
        static srBuff sr;
        static rrBuff rr;
        
        int terr;
        
        // todo -- time out if no data for 30 seconds?
        
        while (contentSize)
        {
            LongWord count = 256;
            
            
            TCPIPPoll();
            TCPIPStatusTCP(ipid, &sr);
            
            count = sr.srRcvQueued;
            if (count > contentSize) count = contentSize;

            if (count == 0)
            {
                continue;
            }
            
            terr = TCPIPReadTCP(ipid, 2, (Ref)0, count, &rr);
            
            if (_toolErr) break;
            
            if (rr.rrBuffCount)
            {
                Handle h = rr.rrBuffHandle;
                HLock(h);
                
                fwrite(*(char **)h, 1, count, file);
                contentSize -= rr.rrBuffCount;
                
                DisposeHandle(h);
                
                continue;
            }
            
            if (terr && !rr.rrMoreFlag)
            {
                fprintf(stderr, "read error\n");
                return -1;
            }
        }
        
        return 0;
    }
    
    if (transferEncoding == 1)
    {
        // chunked..
        // format = hex <CRLF>
        // data .. <CRLF>
        /// 0 <CRLF>
    
    }
      



}



static int do_http_1_1(
  const char *url, 
  URLComponents *components, 
  Word ipid, 
  FILE *file, 
  const char *filename)
{
  Handle dict;  
  DictionaryEnumerator e;
  Word cookie;
  int ok;
  
  char *cp;
  int length;
    
    
  // html headers.
  // (not really needed until 1.1)
  dict = DictionaryCreate(MMStartUp(), 2048);
  
  length = components->host.length;
  cp = url + components->host.location;
  
  DictionaryAdd(dict, "Host", 4, cp, length, false);
  DictionaryAdd(dict, "Connection", 10, "close", 5, false);
  // no gzip or compress.
  DictionaryAdd(dict, "Accept-Encoding", 15, "identity", 8, false);
  
  // connected....
  
  // send the request.
  // GET path HTTP/version\r\n
  
  if (flags._I)
    TCPIPWriteTCP(ipid, "HEAD ", 5, false, false);
  else
    TCPIPWriteTCP(ipid, "GET ", 4, false, false);

  length = components->pathAndQuery.length;
  cp = url + components->pathAndQuery.location;

  if (!length)
  {
    length = 1;
    cp = "/";
  }
  
  TCPIPWriteTCP(ipid, cp, length, false, false);
  
  if (flags._0)
  {
    TCPIPWriteTCP(ipid, " HTTP/1.0\r\n", 11, false, false);
  }
  else
  {
    TCPIPWriteTCP(ipid, " HTTP/1.1\r\n", 11, false, false);
  }
  
  // send the headers.

  cookie = 0;
  while ((cookie = DictionaryEnumerate(dict, &e, cookie)))
  {
    if (!e.keySize) continue;
    
    TCPIPWriteTCP(ipid, e.key, e.keySize, false, false); 
    TCPIPWriteTCP(ipid, ": ", 2, false, false);
    TCPIPWriteTCP(ipid, e.value, e.valueSize, false, false);
    TCPIPWriteTCP(ipid, "\r\n", 2, false, false);
  }
  
  // end headers and push.
  TCPIPWriteTCP(ipid, "\r\n", 2, true, false);
  
  DisposeHandle(dict);
  dict = NULL;
  
  dict = DictionaryCreate(MMStartUp(), 2048);
  ok = parseHeaders(ipid, file, dict);
  
  // todo -- check the headers for content length, transfer-encoding.
  // 
  
  cookie = 0;
  while ((cookie = DictionaryEnumerate(dict, &e, cookie)))
  {
    s16_debug_printf("%.*s -> %.*s", 
        e.keySize, e.key, e.valueSize, e.value);
  }
  
  if (ok == 200)
  {
    if (!flags._I)
    read_response(ipid, file, dict);
  }
  DisposeHandle(dict);
  
  
  //ok = read_binary(ipid, file);
  return 0;
}

  
int do_http(const char *url, URLComponents *components)
{
  char *host;
  char *path;
  char *filename;
  
  Connection connection;
  int ok;
  
  FILE *file;
  
  file = stdout;
  
  if (!components->portNumber) components->portNumber = 80;


  host = URLComponentGetCMalloc(url, components, URLComponentHost);
  path = URLComponentGetCMalloc(url, components, URLComponentPath);

  if (!host)
  {
    fprintf(stderr, "URL `%s': no host.", url);
    free(path);
    return -1;
  }
  
  // outfile.

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
        // path starts with /.
        
        filename = strrchr(path + 1, '/');
        if (filename) // *filename == '/'
        {
            filename++;
        }
        else
        {
            filename = path + 1;
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

      // should set from mime type?      
      setfiletype(filename);
      
  }
  
  
  ok = ConnectLoop(host, components->portNumber, &connection);
  
  if (!ok)
  {
    free(host);
    free(path);
    fclose(file);
    return -1;
  }

  if (flags._9)
  {
    ok = do_http_0_9(url, components, connection.ipid, file, filename);
  }
  else
  {
    ok = do_http_1_1(url, components, connection.ipid, file, filename);
  }
  
  fflush(file);
  if (file != stdout) fclose(file);
  
  CloseLoop(&connection);
  free(host);
  free(path);
  
  return 0;

}
