/*
 * Copyright (c) 2000-2010 Apple Inc.
 * All rights reserved.
 */

#ifndef _STDINT_H_
#define _STDINT_H_

/* from ISO/IEC 988:1999 spec */

/* 7.18.1.1 Exact-width integer types */
#ifndef _INT8_T
#define _INT8_T
typedef signed char            int8_t;
#endif /*_INT8_T */

#ifndef _INT16_T
#define _INT16_T
typedef short                 int16_t;
#endif /* _INT16_T */

#ifndef _INT32_T
#define _INT32_T
typedef long                  int32_t;
#endif /* _INT32_T */

#ifndef _UINT8_T
#define _UINT8_T
typedef unsigned char         uint8_t;
#endif /*_UINT8_T */

#ifndef _UINT16_T
#define _UINT16_T
typedef unsigned short       uint16_t;
#endif /* _UINT16_T */

#ifndef _UINT32_T
#define _UINT32_T
typedef unsigned long        uint32_t;
#endif /* _UINT32_T */

/* 7.18.1.2 Minimum-width integer types */
typedef int8_t           int_least8_t;
typedef int16_t         int_least16_t;
typedef int32_t         int_least32_t;
typedef uint8_t         uint_least8_t;
typedef uint16_t       uint_least16_t;
typedef uint32_t       uint_least32_t;

/* 7.18.1.3 Fastest-width integer types */
typedef int8_t            int_fast8_t;
typedef int16_t          int_fast16_t;
typedef int32_t          int_fast32_t;
typedef uint8_t          uint_fast8_t;
typedef uint16_t        uint_fast16_t;
typedef uint32_t        uint_fast32_t;

/* 7.18.1.4 Integer types capable of holding object pointers */

#ifndef _INTPTR_T
#define _INTPTR_T
typedef long   intptr_t;
#endif /* _INTPTR_T */

#ifndef _UINTPTR_T
#define _UINTPTR_T
typedef unsigned long   uintptr_t;
#endif /* _UINTPTR_T */

/* 7.18.1.5 Greatest-width integer types */
#ifndef _INTMAX_T
#define _INTMAX_T
#ifdef __INTMAX_TYPE__
typedef __INTMAX_TYPE__             intmax_t;
#else /* __INTMAX_TYPE__ */
typedef long                intmax_t;
#endif /* __INTMAX_TYPE__ */
#endif /* _INTMAX_T */

#ifndef _UINTMAX_T
#define _UINTMAX_T
#ifdef __UINTMAX_TYPE__
typedef __UINTMAX_TYPE__             uintmax_t;
#else /* __UINTMAX_TYPE__ */
typedef unsigned long      uintmax_t;
#endif /* __UINTMAX_TYPE__ */
#endif /* _UINTMAX_T */

/* 7.18.2 Limits of specified-width integer types:
 *   These #defines specify the minimum and maximum limits
 *   of each of the types declared above.
 */


/* 7.18.2.1 Limits of exact-width integer types */
#define INT8_MAX         127
#define INT16_MAX        32767
#define INT32_MAX        2147483647

#define INT8_MIN          -128
#define INT16_MIN         -32768
   /*
      Note:  the literal "most negative int" cannot be written in C --
      the rules in the standard (section 6.4.4.1 in C99) will give it
      an unsigned type, so INT32_MIN (and the most negative member of
      any larger signed type) must be written via a constant expression.
   */
#define INT32_MIN        (-INT32_MAX-1)

#define UINT8_MAX         255
#define UINT16_MAX        65535
#define UINT32_MAX        4294967295U

/* 7.18.2.2 Limits of minimum-width integer types */
#define INT_LEAST8_MIN    INT8_MIN
#define INT_LEAST16_MIN   INT16_MIN
#define INT_LEAST32_MIN   INT32_MIN

#define INT_LEAST8_MAX    INT8_MAX
#define INT_LEAST16_MAX   INT16_MAX
#define INT_LEAST32_MAX   INT32_MAX

#define UINT_LEAST8_MAX   UINT8_MAX
#define UINT_LEAST16_MAX  UINT16_MAX
#define UINT_LEAST32_MAX  UINT32_MAX

/* 7.18.2.3 Limits of fastest minimum-width integer types */
#define INT_FAST8_MIN     INT8_MIN
#define INT_FAST16_MIN    INT16_MIN
#define INT_FAST32_MIN    INT32_MIN

#define INT_FAST8_MAX     INT8_MAX
#define INT_FAST16_MAX    INT16_MAX
#define INT_FAST32_MAX    INT32_MAX

#define UINT_FAST8_MAX    UINT8_MAX
#define UINT_FAST16_MAX   UINT16_MAX
#define UINT_FAST32_MAX   UINT32_MAX

/* 7.18.2.4 Limits of integer types capable of holding object pointers */

#define INTPTR_MIN        INT32_MIN
#define INTPTR_MAX        INT32_MAX

#define UINTPTR_MAX       UINT32_MAX

/* 7.18.2.5 Limits of greatest-width integer types */
#define INTMAX_MIN        INT32_MIN
#define INTMAX_MAX        INT32_MAX

#define UINTMAX_MAX       UINT32_MAX

/* 7.18.3 "Other" */
#define PTRDIFF_MIN       INT32_MIN
#define PTRDIFF_MAX       INT32_MAX

/* We have no sig_atomic_t yet, so no SIG_ATOMIC_{MIN,MAX}.
   Should end up being {-127,127} or {0,255} ... or bigger.
   My bet would be on one of {U}INT32_{MIN,MAX}. */

#define SIZE_MAX          UINT32_MAX

/* nb - wchar_t should probably be 16-bit, but there
   isn't any support for it anywhere.  wint_t should
   be 32-bits. */

#ifndef WCHAR_MAX
#  ifdef __WCHAR_MAX__
#    define WCHAR_MAX     __WCHAR_MAX__
#  else
#    define WCHAR_MAX     0x7fff
#  endif
#endif

/* WCHAR_MIN should be 0 if wchar_t is an unsigned type and
   (-WCHAR_MAX-1) if wchar_t is a signed type.  Unfortunately,
   it turns out that -fshort-wchar changes the signedness of
   the type. */
#ifndef WCHAR_MIN
#  if WCHAR_MAX == 0xffff
#    define WCHAR_MIN       0
#  else
#    define WCHAR_MIN       (-WCHAR_MAX-1)
#  endif
#endif

#define WINT_MIN	  INT32_MIN
#define WINT_MAX	  INT32_MAX

#define SIG_ATOMIC_MIN	  INT16_MIN
#define SIG_ATOMIC_MAX	  INT16_MAX

/* 7.18.4 Macros for integer constants */
#define INT8_C(v)    (v)
#define INT16_C(v)   (v)
#define INT32_C(v)   (v ## L)

#define UINT8_C(v)   (v ## U)
#define UINT16_C(v)  (v ## U)
#define UINT32_C(v)  (v ## UL)

#define INTMAX_C(v)  (v ## L)
#define UINTMAX_C(v) (v ## UL)

#endif /* _STDINT_H_ */

