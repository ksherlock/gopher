#pragma optimize 79
#pragma noroot

#include <Types.h>

int parse_mime(const char *cp, Word *ftype, LongWord *atype)
{
  Word size;
  Word *wp = (Word *)cp;
  Word h;
  int i;
  int semi;
  int slash;

  *ftype = 0;
  *atype = 0;

  /*
   * two pass
   * 1. type/subtype
   * 2. type
   */



  if (!cp || !*cp) return 0;

  // find any optional ';'
  semi = slash = -1;
  for (i = 0; ; ++i)
  {
    char c = cp[i];
    if (c == 0) break;
    if (c == '/') slash = i;
    if (c == ';')
    {
      semi = i;
      break;
    }
  }
  size = i;

  for (i = 0; i < 2; ++i)
  {
    h = ((*cp | 0x20) ^ size) & 0x0f;

    switch (h)
    {
    case 0x00:
      // text
      if (size == 4
        && (wp[0] | 0x2020) == 0x6574 // 'te'
        && (wp[1] | 0x2020) == 0x7478 // 'xt'
      ) {
        *ftype = 0x04;
        *atype = 0x0000;
        return 1;
      }
      break;

    case 0x09:
      // application/octet-stream
      if (size == 24
        && (wp[0] | 0x2020) == 0x7061 // 'ap'
        && (wp[1] | 0x2020) == 0x6c70 // 'pl'
        && (wp[2] | 0x2020) == 0x6369 // 'ic'
        && (wp[3] | 0x2020) == 0x7461 // 'at'
        && (wp[4] | 0x2020) == 0x6f69 // 'io'
        && (wp[5] | 0x2020) == 0x2f6e // 'n/'
        && (wp[6] | 0x2020) == 0x636f // 'oc'
        && (wp[7] | 0x2020) == 0x6574 // 'te'
        && (wp[8] | 0x2020) == 0x2d74 // 't-'
        && (wp[9] | 0x2020) == 0x7473 // 'st'
        && (wp[10] | 0x2020) == 0x6572 // 're'
        && (wp[11] | 0x2020) == 0x6d61 // 'am'
      ) {
        *ftype = 0x02;
        *atype = 0x0000;
        return 1;
      }
      // text/x-pascal
      if (size == 13
        && (wp[0] | 0x2020) == 0x6574 // 'te'
        && (wp[1] | 0x2020) == 0x7478 // 'xt'
        && (wp[2] | 0x2020) == 0x782f // '/x'
        && (wp[3] | 0x2020) == 0x702d // '-p'
        && (wp[4] | 0x2020) == 0x7361 // 'as'
        && (wp[5] | 0x2020) == 0x6163 // 'ca'
        && (cp[12] | 0x20) == 0x6c     // 'l'
      ) {
        *ftype = 0xb0;
        *atype = 0x0005;
        return 1;
      }
      break;

    case 0x0c:
      // text/x-c
      if (size == 8
        && (wp[0] | 0x2020) == 0x6574 // 'te'
        && (wp[1] | 0x2020) == 0x7478 // 'xt'
        && (wp[2] | 0x2020) == 0x782f // '/x'
        && (wp[3] | 0x2020) == 0x632d // '-c'
      ) {
        *ftype = 0xb0;
        *atype = 0x0008;
        return 1;
      }
      break;

    }

    size = slash;
    if (size == -1) break;
  }

  return 0;
}
