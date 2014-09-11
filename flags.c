
#ifdef __ORCAC__
#pragma optimize 79
#pragma noroot
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "flags.h"

void FlagsHelp(void)
{
  fputs("gopher [options] url\n", stdout);
  fputs("-h         display help information\n", stdout);
  fputs("-V         display version information\n", stdout);
  fputs("-v         be verbose\n", stdout);
  fputs("-i         display http headers\n", stdout);
  fputs("-I         http HEAD\n", stdout);
  fputs("-O         write output to file\n", stdout);
  fputs("-o <file>  write output to <file> instead of stdout\n", stdout);
  fputs("-0         use HTTP 1.0\n", stdout);
  fputs("-9         use HTTP 0.9\n", stdout);
  fputs("\n", stdout);
  exit(0);
}

int FlagsParse(int argc, char **argv)
{
  char *cp;
  char c;
  int i;
  int j;
    
  memset(&flags, 0, sizeof(flags));

  for (i = 1; i < argc; ++i)
  {    
    cp = argv[i];
    c = cp[0];
      
    if (c != '-')
      return i;
      
    // -- = end of options.
    if (cp[1] == '-' && cp[2] == 0)
      return i + 1;
   
    // now scan all the flags in the string...
    for (j = 1; ; ++j)
    {
      int skip = 0;
          
      c = cp[j];
      if (c == 0) break;
          
      switch (c)
      {
      case 'h':
        FlagsHelp();
        break;
      case 'o':
        // -xarg or -x arg
        skip = 1;
        if (cp[j + 1])
        {
          flags._o = cp + j + 1;
        }
        else
        {
          if (++i >= argc)
          {
            fprintf(stderr, "option requires an argument -- %c\n", c); 
            return -1;
          }
          flags._o = argv[i];
        }
        flags._O = 0;
        break;
      case 'i':
        flags._i = 1;
        break;
      case 'I':
        flags._I = 1;
        break;
      case 'V':
        flags._V = 1;
        break;
      case 'v':
        flags._v++;
        break;
      case 'O':
        flags._O = 1;
        break;
      case '0':
        flags._0 = 1;
        flags._1 = 0;
        break;
      case '1':
        flags._1 = 1;
        flags._0 = 0;
        break;

      default:
        fprintf(stderr, "illegal option -- %c\n", c);
        return -1; 
      }
            
      if (skip) break;
    }    
  }

  return i;
}
