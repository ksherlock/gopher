#ifndef __Options__
#define __Options__

typedef struct Options
{
  unsigned _0:1;
  unsigned _1:1;
  unsigned _9:1;
  unsigned _I:1;
  unsigned _O:1;
  unsigned _i:1;
  char *_o;
  unsigned _v:1;
} Options;

int GetOptions(int argc, char **argv,
  Options *options);
#endif
