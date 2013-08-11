#pragma optimize 79

#include <Locator.h>
#include <TimeTool.h>
#include <tcpip.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "url.h"
#include "connection.h"
#include "options.h"

#include "prototypes.h"

// startup/shutdown flags.
enum {
  kLoaded = 1,
  kStarted = 2,
  kConnected = 4,

  kLoadError = -1,
  kVersionError = -2
};

int StartUpTZ(void)
{
  Word status;
  Word flags = 0;

  status = tiStatus();

  if (_toolErr)
  {
    LoadOneTool(0x38, 0x104);
    if (_toolErr == toolVersionErr) return kVersionError;
    if (_toolErr) return kLoadError;

    status = 0;
    flags |= kLoaded;
  }

  if (tiVersion() < 0x0104)
  {
    return kVersionError;
  }

  if (!status)
  {
    tiStartUp();
    flags |= kStarted;
  }

  return flags;
}

int StartUpTCP(displayPtr fx)
{
  word status;
  word flags = 0;
  
  // TCPIP is an init, not a tool, so it should always
  // be loaded.
  
  status = TCPIPStatus();
  if (_toolErr)
  {
    LoadOneTool(54, 0x0300);
    if (_toolErr == toolVersionErr) return kVersionError;
    if (_toolErr) return kLoadError;

    status = 0;
    flags |= kLoaded;
  }


  // require 3.0b3
  if (TCPIPLongVersion() < 0x03006003)
  {
    if (flags & kLoaded)
      UnloadOneTool(54);

    return kVersionError;     
  }

  if (!status)
  {
    TCPIPStartUp();
    if (_toolErr) return kLoadError;
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

void ShutDownTZ(int flags)
{
  if (flags <= 0) return;

  if (flags & kStarted) tiShutDown();
  if (flags & kLoaded) UnloadOneTool(0x38);
}

void ShutDownTCP(int flags, Boolean force, displayPtr fx)
{
  if (flags <= 0) return;

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

#define GOPHER_VERSION "0.3"
void version(void)
{
  puts("gopher version " GOPHER_VERSION);
}

void help(void)
{
  puts("gopher version " GOPHER_VERSION);
  puts("usage: gopher [options] url");
  puts("");
  puts("-h       show help");
  puts("-V       show version");
  puts("-v       be verbose");
  puts("-o file  write output to file");
  puts("-O       write output to file based on URL");
  puts("");
  puts("HTTP options:");
  puts("-9       use HTTP version 0.9");
  puts("-0       use HTTP version 1.0");
  puts("-1       use HTTP version 1.1");
  puts("-i       print headers");
  puts("-I       print only headers (HEAD)");
}

// #pragma databank [push | pop] would be nice...
#pragma databank 1
pascal void DisplayCallback(const char *message)
{
  unsigned length;

  // message is a p-string.
  length = message ? message[0] : 0;
  if (!length) return;

  fprintf(stderr, "%.*s\n", length, message + 1);
}
#pragma databank 0

int main(int argc, char **argv)
{
  int i;
  int mf;
  int tf;
  int x;


  memset(&flags, 0, sizeof(flags));
  argc = GetOptions(argc, argv, &flags);


  if (argc != 2)
  {
    help();
    return 1;
  }


  tf = StartUpTZ();

  if (tf < 0)
  {
    fprintf(stderr, "Time Tool 1.0.4 or greater is required.\n");
    exit(1);
  }

    
  mf = StartUpTCP(flags._v ? DisplayCallback : NULL);

  if (mf < 0)
  {
    ShutDownTZ(tf);
    fprintf(stderr, "Marinetti 3.0b3 or greater is required.\n");
    exit(1);
  }



  if (argc == 2)
  {
    const char *url;
    URLComponents components;

    url = argv[1];
    
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

  ShutDownTCP(mf, false, flags._v ? DisplayCallback : NULL);
  ShutDownTZ(tf);

  return 0;
}
