#ifndef __orca__
#define __orca__

#include <stdio.h>

#define fsetbinary(f) (f->_flag &= ~_IOTEXT)
#define fsettext(f) (f->_flag |= _IOTEXT)


#endif
