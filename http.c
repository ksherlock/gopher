
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

#include <stdlib.h>
 
#include "url.h"
#include "connection.h"
#include "prototypes.h"
#include "dictionary.h"
#include "flags.h"

static int do_http_0_9(
  const char *url, 
  URLComponents *components, 
  Word ipid, 
  FILE *file, 
  const char *filename)
{

  TCPIPWriteTCP(ipid, "GET ", 4, false, false);

  if (components->pathAndQuery.length)
  {
    const char *path = url + components->pathAndQuery.location;
    int length = components->pathAndQuery.length;
    
    TCPIPWriteTCP(ipid, path, length, false, false);
  }
  else
  {
    TCPIPWriteTCP(ipid, "/", 1, false, false);
  }
  
  TCPIPWriteTCP(ipid, "\r\n", 2, true, false);

  ok = read_binary(ipid, file);
  return 0;
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

  char *cp;
  int length;
    
    
  // html headers.
  // (not really needed until 1.1)
  dict = DictionaryCreate(MMStartUp(), 2048);
  
  length = components->host.length;
  cp = url + components->host.location;
  
  DictionaryAdd(dict, "Host", 4, cp, length, false);
  DictionaryAdd(dict, "Connection", 10, "close", 5, false);
  
  // connected....
  
  // send the request.
  // GET path HTTP/version\r\n
  
  TCPIPWriteTCP(connection.ipid, "GET ", 4, false, false);

  length = components->pathAndQuery.length;
  cp = url + components->pathAndQuery.location;
    
  if (length)
  {
    TCPIPWriteTCP(ipid, cp, length, false, false);
  }
  else
  {
    TCPIPWriteTCP(connection.ipid, "/", 1, false, false);
  }
  
  if (flags._0)
  {
    TCPIPWriteTCP(connection.ipid, " HTTP/1.0\r\n", 11, false, false);
  }
  else
  {
    TCPIPWriteTCP(connection.ipid, " HTTP/1.1\r\n", 11, false, false);
  }
  
  // send the headers.

  cookie = 0;
  while ((cookie = DictionaryEnumerate(dict, &e, cookie)))
  {
    if (!e.keySize) continue;
    
    TCPIPWriteTCP(connection.ipid, e.key, e.keySize, false, false); 
    TCPIPWriteTCP(connection.ipid, ": ", 2, false, false);
    TCPIPWriteTCP(connection.ipid, e.value, e.valueSize, false, false);
    TCPIPWriteTCP(connection.ipid, "\r\n", 2, false, false);
  }
  
  // end headers and push.
  TCPIPWriteTCP(connection.ipid, "\r\n", 2, true, false);
  
  DisposeHandle(dict);
  dict = NULL;
  
  ok = read_binary(connection.ipid, file);
  return 0;
}

  
int do_http(const char *url, URLComponents *components)
{
  char *host;
  char *path;
  char *filename;
  
  Connection connection;
  int ok;
  
  File *file;
  
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
