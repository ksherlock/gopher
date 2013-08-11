
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
#include <TimeTool.h>
#include <GSOS.h>

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


static FileInfoRecGS FileInfo;
static Word FileAttr;

static int do_http_0_9(
  const char *url, 
  URLComponents *components, 
  Word ipid, 
  FILE *file, 
  const char *filename)
{
  ReadBlock dcb;
  int ok;
  
  char *cp;
  int length;
      
  IncBusy();
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
  DecBusy();

  ok = read_binary(ipid, file, &dcb);
  return ok;
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
  
  IncBusy();
  TCPIPPoll();
  DecBusy();
  
  for (;;)
  {
      rlBuffer rb;
      Handle h;
      char *cp;
      int ok;
      
      ok = ReadLine2(ipid, &rb);
      
      if (ok == 0)
      {
        IncBusy();
        TCPIPPoll();
        DecBusy();
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

int http_read_chunked(word ipid, FILE *file)
{
    ReadBlock dcb;
    rlBuffer rb;
    LongWord chunkSize;
    LongWord count;
    Handle h;
    word i;
    
    int ok;
    
    for (;;)
    {
        char *cp;

        // get the chunk size.
        // 0 indicates end.
        for (;;)
        {
            ok = ReadLine2(ipid, &rb);     
        
            if (ok == 0)
            {
                IncBusy();
                TCPIPPoll();
                DecBusy();
                continue;
            }
            
            if (ok == -1) return -1;
            
            h = rb.bufferHandle;
            HLock(h);
            
            cp = *(char **)h;
            for (i = 0; i < (Word)rb.bufferSize; ++i)
            {
                if (!isxdigit(cp[i])) break;
            }
            if (i == 0) return -1;
            
            chunkSize = Hex2Long(cp, i);
            // is there another <CRLF> pair?
            break;
        }

        // now read the data.
        if (chunkSize == 0) return 0; // eof.

        dcb.requestCount = chunkSize;
        ok = read_binary_size(ipid, file, &dcb);
        if (ok < 0) return -1;
        if (dcb.requestCount != dcb.transferCount)
        {
          fprintf(stderr, "Read error - requested %ld, received %ld\n", 
            dcb.requestCount, dcb.transferCount);
          return -1;
        }

        // read CRLF.
        for(;;)
        {
            ok = ReadLine2(ipid, &rb);
            if (ok == -1) return -1;
            if (ok == 0)
            {
                TCPIPPoll();
                continue;
            }
            if (ok == 1)
            {
                if (rb.bufferSize)
                {
                  fprintf(stderr, "Unexpected data in chunked response.\n");
                  return -1;
                }
                break;
            }
        }
    }

    return 0;
}

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

    int haveTime = 0;    
    
    contentSize = 0;
    transferEncoding = -1;
    value = DictionaryGet(dict, "Content-Length",  14, &valueSize);
    
    if (value)
    {
        contentSize = Dec2Long(value, valueSize, 0);
        transferEncoding = 0;
    }
    
    /*
     * check the transfer encoding header 
     * should be chunked or identity (default)
     *
     */
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

    /*
     * convert a content-type header mime string into
     * a file type / aux type.
     *
     */
    value = DictionaryGet(dict, "Content-Type", 12, &valueSize);
    if (value && valueSize)
    {
      int i;
      int slash = -1;
      // strip ';'
      for (i = 0; i < valueSize; ++i)
      {
        char c = value[i];
        if (c == ';') break;
        if (c == '/') slash = i;
      }

      // todo -- flag for this or not.
      valueSize = i;
      if (parse_mime(value, valueSize, 
        &FileInfo.fileType, 
        &FileInfo.auxType))
      {
        FileAttr |= ATTR_FILETYPE | ATTR_AUXTYPE;
      }
      else if (slash != -1 && parse_mime(value, slash, 
        &FileInfo.fileType, 
        &FileInfo.auxType))
      {
        FileAttr |= ATTR_FILETYPE | ATTR_AUXTYPE;
      }

    }

    /*
     * convert the Last Modified header into a file mod date
     *
     */
    value = DictionaryGet(dict, "Last-Modified", 13, &valueSize);
    if (value && valueSize <= 255)
    {
        char *pstring;

        pstring = (char *)malloc(valueSize + 1);
        if (pstring)
        {
            struct {
              LongWord lo;
              LongWord hi;
            } xcomp;

            *pstring = valueSize;
            memcpy(pstring + 1, value, valueSize);

            // parse the last-modified timestamp.
            // 0x0e00 is rfc 822 format.
            // (which is now obsoleted by rfc 2822 but close enough)


            // should use _tiDateString2Sec to get seconds
            // then use ConvSeconds to get the date
            // this should handle timezones. 

            tiDateString2Sec(&xcomp, pstring, 0x0e00);
            if (!_toolErr && xcomp.hi == 0)
            {
              ConvSeconds(secs2TimeRec, (Long)xcomp.lo,
                (Pointer)&FileInfo.modDateTime);
              FileAttr |= ATTR_MODTIME;
              haveTime = 1;
            }

            free(pstring);
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

        ReadBlock dcb;
        int ok;

        dcb.requestCount = contentSize;
        ok = read_binary_size(ipid, file, &dcb);

        if (!ok) return -1;
        if (dcb.transferCount != dcb.requestCount)
        {
            fprintf(stderr, "Read error - requested %ld, received %ld\n", 
              dcb.requestCount, dcb.transferCount);
            return -1;
        }
        return 0;
    }
    
    if (transferEncoding == 1)
    {
        return http_read_chunked(ipid, file);
    }
    
    return -1;
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
  Word MyID = MMStartUp();
  int ok;
  
  char *cp;
  int length;
    
    
  // html headers.
  // (not really needed until 1.1)
  dict = DictionaryCreate(MyID, 2048);
  
  length = components->host.length;
  cp = url + components->host.location;
  
  DictionaryAdd(dict, "Host", 4, cp, length, false);
  DictionaryAdd(dict, "Connection", 10, "close", 5, false);
  // no gzip or compress.
  DictionaryAdd(dict, "Accept-Encoding", 15, "identity", 8, false);
  
  // connected....
  
  // send the request.
  // GET path HTTP/version\r\n
  IncBusy();
  
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
  DecBusy();
  
  DisposeHandle(dict);
  dict = NULL;
  
  dict = DictionaryCreate(MyID, 2048);
  ok = parseHeaders(ipid, file, dict);
  
  // todo -- check the headers for content length, transfer-encoding.
  // 
  
  #if 0
  cookie = 0;
  while ((cookie = DictionaryEnumerate(dict, &e, cookie)))
  {
    s16_debug_printf("%.*s -> %.*s\n", 
        e.keySize, e.key, e.valueSize, e.value);
  }
  #endif



  if (ok == 200)
  {
    if (!flags._I)
    read_response(ipid, file, dict);
  }
  DisposeHandle(dict);
  
  
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

  FileAttr = 0;
  memset(&FileInfo, 0, sizeof(FileInfo));
  
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

        // todo -- also need to strip any ? parameters.
        
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


  // todo -- write to a tmp file rather than the named file.
  // this will allow things like If-Modified-Since  or If-None-Match  
  // so it only updates if changed.
  // or -C continue to append
  // -N -- if-modified-since header.

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

      // hmm, flag for this vs content type?
      if (parse_extension_c(filename, &FileInfo.fileType, &FileInfo.auxType))
      {
        FileAttr |= ATTR_FILETYPE | ATTR_AUXTYPE;
      }
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
  
  if (filename) setfileattr(filename, &FileInfo, FileAttr);

  CloseLoop(&connection);
  free(host);
  free(path);
  
  return 0;

}
