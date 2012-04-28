
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
 
int do_http_1_0(const char *url, URLComponents *components, FILE *file)
{
  char *host;
  Connection connection;
  int ok;
  Handle dict;  
  
  if (!components->portNumber) components->portNumber = 80;


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
    return -1;
  }

  // html headers.
  // (not really needed until 1.1)
  dict = DictionaryCreate(MMStartUp(), 2048);
  DictionaryAdd(dict, "Host", 4, host, components->host.length, false);
  DictionaryAdd(dict, "Connection", 10, "close", 5, false);
  
  // connected....
  
  // send the request.
  TCPIPWriteTCP(connection.ipid, "GET ", 4, false, false);

  if (components->pathAndQuery.length)
  {
    TCPIPWriteTCP(connection.ipid, 
      url + components->pathAndQuery.location, 
      components->pathAndQuery.length, 
      false, 
      false);
  }
  else
  {
    TCPIPWriteTCP(connection.ipid, "/", 1, false, false);
  }
  
  TCPIPWriteTCP(connection.ipid, " HTTP/1.0\r\n", 11, false, false);

  // send the headers.
  {
    DictionaryEnumerator e;
    Word cookie;
    
    cookie = 0;
    while ((cookie = DictionaryEnumerate(dict, &e, cookie)))
    {
      if (!e.keySize) continue;
      
      TCPIPWriteTCP(connection.ipid, e.key, e.keySize, false, false); 
      TCPIPWriteTCP(connection.ipid, ": ", 2, false, false);
      TCPIPWriteTCP(connection.ipid, e.value, e.valueSize, false, false);
      TCPIPWriteTCP(connection.ipid, "\r\n", 2, false, false);
    }  
  }
  // end headers and push.
  TCPIPWriteTCP(connection.ipid, "\r\n", 2, true, false);
  

  // read until eof.
  
  ok = read_binary(connection.ipid, file);
  
  fflush(file);

  CloseLoop(&connection);
  free (host);
  DisposeHandle(dict);
  
  return 0;

}
