#ifdef __ORCAC__
#pragma optimize 79
#pragma noroot
#endif

#include <stdio.h>
#include <stdlib.h>

#include "options.h"

  extern void help(void);

int GetOptions(int argc, char **argv, 
  struct Options *options)
{
  int i, j;
  int eof = 0;
  int mindex = 1; /* mutation index */

  for (i = 1; i < argc; ++i)
  {
    char *cp = argv[i];
    char c = cp[0];

    

    if (eof || c != '-')
    {
      if (mindex != i) argv[mindex] = argv[i];
      ++mindex;
      continue;
    }

    // long opt check would go here...
    if (cp[1] == '-' && cp[2] == 0)
    {
      eof = 1;
      continue;
    }

    // special case for '-'
    j = 0;
    if (cp[1] != 0) j = 1;

    for (; ; ++j)
    {
      char *optarg = 0;

      c = cp[j];
      if (!c) break;
      switch(c)
      {
        case '0':
          options->_0 = 1;
            options->_1 = 0;
          break;
        case '1':
          options->_1 = 1;
            options->_0 = 0;
          break;
        case 'I':
          options->_I = 1;
          break;
        case 'O':
          options->_O = 1;
          break;
        case 'h':
            help();
            exit(0);
          break;
        case 'i':
          options->_i = 1;
          break;
        case 'o':
          ++j;
          if (cp[j]) {
            optarg = cp + j;
          } else {
            ++i;
            if (i < argc) optarg = argv[i];
          }
          if (!optarg) {
            fputs("-o requires an argument\n", stderr);
            exit(1);
          }
          options->_o = optarg;
            options->_O = 0;
          break;
        case 'v':
          options->_v = 1;
          break;
        default:
          fprintf(stderr, "-%c : invalid option\n", c);
          exit(1);
          break;
      }
      // could optimize out if no options have flags.
      if (optarg) break;
    }
  }
  return mindex;
}
