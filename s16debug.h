#ifndef __s16_debug_h__
#define __s16_debug_h__

void s16_debug_puts(const char *str);
void s16_debug_printf(const char *format, ...);
void s16_debug_dump(const char *data, unsigned size);

void s16_debug_tcp(unsigned ipid);

#ifdef __TCPIP__
void s16_debug_srbuff(const srBuff *sb);
#endif

#endif
