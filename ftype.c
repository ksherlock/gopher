#pragma optimize 79
#pragma noroot

#include <Types.h>

// cp should be a filename w/ .ext
int parse_extension(const char *cp, Word size, Word *ftype, LongWord *atype);
int parse_extension_c(const char *cp, Word *ftype, LongWord *atype)
{
  int i;
  int pd;

  if (!cp || !*cp) return 0;

  pd = -1;
  for (i = 0; ; ++i)
  {
    char c;

    c = cp[i];
    if (c == 0) break;
    if (c == '.') pd = i;
  }

  // pd == position of final .
  // i == strlen

  if (pd == -1) return 0;
  if (pd + 1 >= i) return 0;
  pd++; // skip past it...

  return parse_extension(cp + pd, i - pd, ftype, atype);
}

// cp is just the extension
int parse_extension(const char *cp, Word size, Word *ftype, LongWord *atype)
{
  Word *wp = (Word *)cp;
  Word h;


  if (!cp || !size) return 0;


  h = ((*cp | 0x20) ^ size) & 0x0f;

  switch (h)
  {
    case 0x00:
      // shk
      if (size == 3
        && (wp[0] | 0x2020) == 0x6873 // 'sh'
        && (cp[2] | 0x20) == 0x6b     // 'k'
      ) {
        *ftype = 0xe0;
        *atype = 0x8002;
        return 1;
      }
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

    case 0x01:
      // bxy
      if (size == 3
        && (wp[0] | 0x2020) == 0x7862 // 'bx'
        && (cp[2] | 0x20) == 0x79     // 'y'
      ) {
        *ftype = 0xe0;
        *atype = 0x8000;
        return 1;
      }
      break;

    case 0x02:
      // asm
      if (size == 3
        && (wp[0] | 0x2020) == 0x7361 // 'as'
        && (cp[2] | 0x20) == 0x6d     // 'm'
      ) {
        *ftype = 0xb0;
        *atype = 0x0003;
        return 1;
      }
      // c
      if (size == 1
        && (cp[0] | 0x20) == 0x63     // 'c'
      ) {
        *ftype = 0xb0;
        *atype = 0x0008;
        return 1;
      }
      break;

    case 0x03:
      // pas
      if (size == 3
        && (wp[0] | 0x2020) == 0x6170 // 'pa'
        && (cp[2] | 0x20) == 0x73     // 's'
      ) {
        *ftype = 0xb0;
        *atype = 0x0005;
        return 1;
      }
      break;

    case 0x07:
      // txt
      if (size == 3
        && (wp[0] | 0x2020) == 0x7874 // 'tx'
        && (cp[2] | 0x20) == 0x74     // 't'
      ) {
        *ftype = 0x04;
        *atype = 0x0000;
        return 1;
      }
      break;

    case 0x09:
      // h
      if (size == 1
        && (cp[0] | 0x20) == 0x68     // 'h'
      ) {
        *ftype = 0xb0;
        *atype = 0x0008;
        return 1;
      }
      break;

  }

  return 0;
}
