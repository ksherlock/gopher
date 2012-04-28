#pragma optimize 79
#pragma noroot

#include "dictionary.h"
#include <memory.h>
#include <string.h>

#include "fast.memory.h"

//#pragma debug -1

typedef struct _Dictionary {
  Word count;
  Word offset;
  Word table[16];
} _Dictionary;

typedef struct _DictionaryEntry {
  Word hash;
  Word next;
  Word keySize;
  Word dataSize;
} _DictionaryEntry;

#define SIZEOF_D sizeof(_Dictionary)
#define SIZEOF_DE sizeof(_DictionaryEntry)
#define H_TO_D(h) *((_Dictionary **)h)
#define D_TO_DE(d,offset) (_DictionaryEntry *)((char *)d + offset)
#define DE_TO_KEY(de) ((char *)de + SIZEOF_DE)
#define DE_TO_DATA(de,keySize) ((char *)de + SIZEOF_DE + keySize)

static Boolean keycmp(const char *key1, const char *key2, Word size)
{
	// key1 is guaranteed to be lowercase.
	Word i;
	
	for (i = 0; i < size; ++i)
	{
		char a, b;
		a = key1[i];
		b = key2[i];
		
		if (b >= 'A' && b <= 'Z') b |= 0x20;
		if (a != b) return 0;
	}
	
	return 1;
	
}
static Word findEntry(_Dictionary *d, const char *key, Word keySize, Word hash)
{
  Word offset;

  offset = d->table[hash & 0x0f];

  while (offset)
  {
	_DictionaryEntry *de;
	de = D_TO_DE(d, offset);
	if (de->hash == hash && de->keySize == keySize)
	{
	  char *tmp = DE_TO_KEY(de);
	  if (keycmp(tmp, key, keySize)) return offset;
	}
	offset = de->next;
  }

  return offset;
}

void deleteEntry(_Dictionary *d, Word offset)
{
  // remove an entry, adjusting the hashtable and next pointers.

  _DictionaryEntry *de;
  Word size;
  Word hash;
  Word xoffset;
  Word i;
  

  de = D_TO_DE(d, offset);

  size = SIZEOF_DE + de->keySize + de->dataSize;
  hash = de->hash;

  if (d->table[hash & 0x0f] == offset)
	d->table[hash & 0x0f] = de->next;

  for (i = 0; i < 16; ++i)
  {
	if (d->table[i] > offset) d->table[i] -= size;
  }

  // remove the entry.
  memmove(de, de + size, d->offset - offset - size);

  // de now points to the *next* entry.

  d->count--;
  d->offset -= size;

  xoffset = offset;

  while (xoffset < d->offset)
  {
	Word tmp = SIZEOF_DE + de->keySize + de->dataSize;

	if (de->next >= offset) de->next -= size;
	xoffset += tmp;
	de = (_DictionaryEntry *)((char *)de + tmp);
  }
}

// djb hash, modified to be case-insensitive 
// and 16-bit.
static Word hash(const char *key, Word keySize)
{
  Word h = 5381;
  unsigned char c;
  Word i;

  for (i = 0; i < keySize; ++i)
  {
	c = key[i];
	if (c >= 'A' && c <= 'Z') c |= 0x20;  // lowercase it.
	h = ((h << 5) + h) ^ c;
  }
  return h;
}

Handle DictionaryCreate(Word MemID, Word initialSize)
{
	Word i;
	Handle h;
	_Dictionary *d;
	
	if (initialSize < 2048) initialSize = 2048;
	
	h = NewHandle(initialSize, MemID, attrNoSpec | attrLocked, NULL);
	if (_toolErr) return NULL;
	
	d = H_TO_D(h);
	
	//bzero(d, SIZEOF_D);
	d->count = 0;
	for (i = 0; i < 16; ++i)
		d->table[i] = 0;
	
	d->offset = SIZEOF_D; // next entry.
	
	return h;
}


Word DictionaryCount(Handle h)
{
  _Dictionary *d;
  
  if (!h) return 0;
  
  _HLock(h);

  d = H_TO_D(h);
  
  return d->count;
}

Boolean DictionaryContains(Handle h, const char *key, Word keySize)
{
  Word hh;
  Word offset;
  _Dictionary *d;
  
  if (!h) return 0;
  
  _HLock(h);
  
  d = H_TO_D(h);
  
  hh = hash(key, keySize);
  offset = findEntry(d, key, keySize, hh);
  
  return offset != 0;
}
void *DictionaryGet(Handle h, const char *key, Word keySize, Word *dataSize)
{
  Word hh;
  Word offset;
  _Dictionary *d;
  _DictionaryEntry *de;

  if (!h) return NULL;
  
  _HLock(h);
  
  d = H_TO_D(h);

  hh = hash(key, keySize);
  offset = findEntry(d, key, keySize, hh);

  if (offset == 0)
  {
	if (dataSize) *dataSize = 0;
	return NULL;
  } 

  de = D_TO_DE(d, offset);

  if (dataSize) *dataSize = de->dataSize;

  return DE_TO_DATA(de,keySize);
}

Word DictionaryAdd(Handle h, 
  const char *key, Word keySize, 
  const char *data, Word dataSize, 
  Boolean replace)
{
  _Dictionary *d;
  _DictionaryEntry *de;

  Word hh;
  Word offset;
  Word size;
  Word i;
  
  LongWord hSize;
  LongWord newSize;
  

  if (!h) return 0;
  
  _HLock(h);
  
  d = H_TO_D(h);
  hh = hash(key, keySize);

  offset = findEntry(d, key, keySize, hh);

  if (offset)
  {
	if (!replace) return 0;
	// delete the previous entry iff datasize differs.
	de = D_TO_DE(d, offset);
	if (de->dataSize == dataSize)
	{
	  memcpy(DE_TO_DATA(de, keySize), data, dataSize);
	  return 1;
	}
	//
	deleteEntry(d, offset);
  } 

  // create and insert it.
  offset = d->offset;

  hSize = _GetHandleSize(h);
  
  newSize = offset + SIZEOF_DE + keySize + dataSize;
  if (newSize > 0xffff) return 0;
  
  if (newSize > hSize)
  {
    _HUnlock(h);
    newSize = (newSize + 4095) & ~4095;
    SetHandleSize(newSize, h);
    if (_toolErr) return 0;
    _HLock(h);

	d = H_TO_D(h);
  }

  de = D_TO_DE(d, offset);
  de->hash = hh;
  de->next = d->table[hh & 0x0f];
  de->keySize = keySize;
  de->dataSize = dataSize;

  // copy the key (lowercase).
  for (i = 0; i < keySize; ++i)
  {
	char c;
	c = key[i];
	if (c >= 'A' && c <= 'Z') c |= 0x020;
	DE_TO_KEY(de)[i] = c;
  }
  memcpy(DE_TO_DATA(de, keySize), data, dataSize);

  d->table[hh & 0x0f] = offset; 

  d->count++;  
  d->offset = newSize;

  return 1;
}

void DictionaryRemove(Handle h, const char *key, Word keySize)
{
  _Dictionary *d;
  Word offset;
  Word hh;

  if (!h) return;
  
  _HLock(h);
  
  d = H_TO_D(h);

  hh = hash(key, keySize);

  offset = findEntry(d, key, keySize, hh);
  if (offset)
  {
	deleteEntry(d, offset);
  }
}

Word DictionaryEnumerate(Handle h, DictionaryEnumerator *e, Word cookie)
{
  // returns the next offset;
  _Dictionary *d;
  _DictionaryEntry *de;

  if (!h) return 0;
  
  if (cookie == 0) cookie = SIZEOF_D;

  _HLock(h);
  d = H_TO_D(h);
  if (cookie >= d->offset) return 0;

  de = D_TO_DE(d, cookie);

  if (e)
  {
	e->keySize = de->keySize;
	e->valueSize = de->dataSize;
	e->key = DE_TO_KEY(de);
	e->value = DE_TO_DATA(de,de->keySize);
  }

  return cookie + SIZEOF_DE + de->keySize + de->dataSize;
}
