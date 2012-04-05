#ifndef __DATA_H__
#define __DATA_H__

#include <Types.h>

Handle DataCreate(Word MemID, Word initialSize);
Word DataReset(Handle h);

Word DataAppendChar(Handle h, char c);

Word DataAppendBytes(Handle h, const char *data, LongWord count);

Word DataAppendCString(Handle h, const char *s);
Word DataAppendPString(Handle h, const char *s);
Word DataAppendGSString(Handle h, const GSString255Ptr s);

LongWord DataSize(Handle h);
void *DataBytes(Handle h);

char *DataLock(Handle h, Word minSize);
void DataUnlock(Handle h, Word newSize);

#endif
