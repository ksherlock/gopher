#pragma optimize 79

#include <Locator.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "url.h"
#include "connection.h"
#include "prototypes.h"
#include "flags.h"

struct Flags flags;

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


  // require 3.0b3
  if (TCPIPLongVersion() < 0x03006003)
  {
    if (flags & kLoaded)
      UnloadOneTool(54);
    return -1;      
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


void help(void)
{

  fputs("gopher [options] url\n", stdout);
  fputs("-h         display help information.\n", stdout);
  fputs("-V         display version information.\n", stdout);
  fputs("-O         write output to file.\n", stdout);
  fputs("-o <file>  write output to <file> instead of stdout.\n", stdout);
  fputs("-0         use HTTP 1.0\n", stdout);
  fputs("-9         use HTTP 0.9\n", stdout);
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
  Word mf;
  int ch;
  
  mf = StartUp(NULL);

  if (mf == -1)
  {
    fprintf(stderr, "Marinetti 3.0b3 or higher is required.\n");
    exit(1);
  }
  
  memset(&flags, 0, sizeof(flags));
  
  while ((ch = getopt(argc, argv, "o:OhVv09")) != -1)
  {
    switch (ch)
    {
    case 'V':
        fputs("gopher v 0.2\n", stdout);
        exit(0);
        break;
        
    case 'v':
        flags._v = 1;
        break;
        
    case 'o':
        flags._o = optarg;
        flags._O = 0;
        break;
    
    case 'O':
        flags._O = 1;
        flags._o = NULL;
        break;
        
    case '9':
        flags._9 = 1;
        flags._0 = 0;
        break;
        
    case '0':
        flags._0 = 1;
        flags._9 = 0;
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
        do_gopher(url, &components);
    }
    else if (components.schemeType == SCHEME_HTTP)
    {
        do_http(url, &components);
    }
    else
    {
        fprintf(stderr, "Unsupported scheme.\n");
        exit(1);
    }
  }

  ShutDown(mf, false, NULL);

  return 0;
}
