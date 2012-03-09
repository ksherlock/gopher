#include <Locator.h>
#include <Memory.h>

#include <tcpip.h>


//#include "url.h"
#include "Connect.h"

#include <stdio.h>

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

Word StartUp(void)
{
  word status;
  word flags = 0;
  
  status = TCPIPStatus();
  if (_toolErr)
  {
    LoadOneTool(54,0x0200);
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
    TCPIPConnect(NULL);
    flags |= kConnected;
  }
  return flags;
}

void ShutDown(word flags)
{
    if (flags & kConnected)
    {
      TCPIPDisconnect(false, NULL);
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

#if 0
void do_url(const char *url)
{
  URLComponents components;
  ConnectBuffer buffer;
  char *host;

  if (ParseURL(url, strlen(url), &components))
  {
    fprintf(stderr, "Invalid URL: %s\n", url);
    return;
  }
  if (!url.host.length)
  {
    fprintf(stderr, "No host\n");
    return;
  }
  if (!components.portNumber) components.portNumber = 70;
  host = malloc(url.host.length + 1);
  URLComponentGetCString(url, &components, host, URLComponentHost);

  ConnectInit(&buffer, myID);
  

  ConnectCString(&buffer, host,  components.portNumber);

  if (components.path.length)
  {
    rv = TCPIPWriteTCP(buffer.ipid, 
      url + components.path.location, 
      components.path.length, 
      0, 0);
  }
  rv = TCPIPWriteTCP(buffer.ipid, "\r\n", 2, 1, 0);


  while (!ConnectionPoll(&buffer)) ;
  if (buffer.state == ConnectionStateError)
  {
    fprintf(stderr, "Unable to open host: %s:%u\n", host, components.portNumber);
    free(host);
    return;
  }

  
  ConnectionClose(&buffer);

  while (!ConnectionPoll(&buffer)) ; // wait for it to close.

  free (host);
}

#endif
int main(int argc, char **argv)
{
  Handle h;
  ConnectBuffer buffer;

  int i;
  Word rv;
  Word flags;

 // getopt for -b binary

#if 0
  for (i = 1; i < argc; ++i)
  {
    do_url(argv[i]);
  }
#endif

  
  flags = StartUp();
  fprintf(stdout, "flags: %u\n", flags);

  ConnectionInit(&buffer, MMStartUp());

  rv = ConnectionOpen(&buffer, "\pgopher.floodgap.com", 70);
  while (!rv)
  {
     rv = ConnectionPoll(&buffer);
  }
  if (buffer.state == ConnectStateConnected)
  {
    fprintf(stdout, "Connected!\n");
  }

  fprintf(stdout, "%x %x %x\n", rv, buffer.state, buffer.terr);

  rv = TCPIPWriteTCP(buffer.ipid, "\r\n", 2, 1, 0);
  if (rv) 
  { 
    fprintf(stdout, "TCPIPWriteTCP: %x\n", rv); 
  }

  for(;;)
  { 
    rrBuff rb;
    //rlrBuff rb;
    Handle h;
    Word count;

    TCPIPPoll();
    rv = TCPIPReadTCP(buffer.ipid, 2, NULL, 4096, &rb);

    //rv = TCPIPReadLineTCP(buffer.ipid, "\p\r\n", 2, NULL, 4096, &rb);

    if (rv) break;
    h = rb.rrBuffHandle;
    count = rb.rrBuffCount;

    if (!h) continue;

    HLock(h);
    fwrite(*h, count, 1, stdout);
    fputs("", stdout);
    DisposeHandle(h);
  }


  rv = ConnectionClose(&buffer);
  while (!rv)
  {
    rv = ConnectionPoll(&buffer);
  }
  fprintf(stdout, "%x %x %x\n", rv, buffer.state, buffer.terr);

  ShutDown(flags);

}

