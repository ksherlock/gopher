
#ifndef __flags_h__
#define __flags_h__

typedef struct Flags {
    char *_o;
    unsigned _v;

    unsigned _i:1;
    unsigned _I:1;
    unsigned _V:1;
    unsigned _O:1;
    unsigned _0:1;
    unsigned _1:1;

} Flags;


extern struct Flags flags;

int FlagsParse(int argc, char **argv);

void FlagsHelp(void);

#endif

