#ifndef __HTTP_UTILS_H__
#define __HTTP_UTILS_H__

#include "url.h"


int parseHeaderLine(const char *line, unsigned length, URLRange *key, URLRange *value);

int parseStatusLine(const char *cp, unsigned length, int *version, int *status);

#endif