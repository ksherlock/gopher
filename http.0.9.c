/*
 * HTTP 0.9
 *
 * request:
 * GET url CR LF
 *
 * response:
 * data EOF
 *
 * http://www.w3.org/Protocols/HTTP/AsImplemented.html
 *
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

int do_http_0_9(const char *url, URLComponents *components, FILE *file)
{
  char *host;
  Connection connection;
  int ok;
  
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

    
  // connected....
  TCPIPWriteTCP(connection.ipid, "GET ", 4, false, false);

  if (components->pathAndQuery.length)
  {
    const char *path = url + components->pathAndQuery.location;
    int length = components->pathAndQuery.length;
    
    TCPIPWriteTCP(connection.ipid, path, length, false, false);
  }
  else
  {
    TCPIPWriteTCP(connection.ipid, "/", 1, false, false);
  }
  
  TCPIPWriteTCP(connection.ipid, "\r\n", 2, true, false);

  // read until eof.
  
  ok = read_binary(connection.ipid, file);
  
  fflush(file);

  CloseLoop(&connection);
  free(host);
  
  return 0;

}
 