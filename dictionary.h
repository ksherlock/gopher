#ifndef __DICTIONARY_H__
#define __DICTIONARY_H__

#include <Types.h>

typedef struct DictionaryEnumerator {
  Word keySize;
  Word valueSize;
  void *key;
  void *value;
} DictionaryEnumerator;

Handle DictionaryCreate(Word MemID, Word initialSize);

Boolean DictionaryContains(Handle h, const void *key, Word keySize);

void *DictionaryGet(Handle h, const char *key, Word keySize, Word *valueSize);

Word DictionaryCount(handle h);

Word DictionaryAdd(Handle h, const void *key, Word keySize, 
  const void *value, Word valueSize, Boolean replace);

// removal performance is awful.
void DictionaryRemove(Handle h, const char *key, Word keySize);

Word DictionaryEnumerate(Handle h, DictionaryEnumerator *e, Word cookie);

#endif
