#ifndef __flags_h__
#define __flags_h__

typedef struct Flags {

    int _0:1;   // -1 (use http 1.0)
    int _9:1;   // -9 (use http 0.9)
    int _i:1;   // -i (include http headers)
    int _I:1;   // -I (http HEAD command)
    int _O:1;   // -O (file name from url)
    int _v:1;   // -v (verbose)
    int _V:1;

    char *_o;
} Flags;


extern struct Flags flags;

int ParseFlags(int argc, char **argv);

void help(void);

#endif