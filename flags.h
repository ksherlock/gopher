#ifndef __flags_h__
#define __flags_h__

typedef struct Flags {

    int _0:1;   // -1 (use http 1.0)
    int _9:1;   // -9 (use http 0.9)
    int _i:1;   // -i (include http headers)
    int _O:1;   // -O (file name from url)
    int _v:1;   // -v (verbose)

    char *_o;
} Flags;


extern struct Flags flags;

#endif