#include "dictionary.h"
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

int main(int argc, char **argv)
{
  Handle dict;
  Word rv;

  dict = DictionaryCreate(MMStartUp(), 0);
  if (!dict)
  {
    fprintf(stderr, "Error: DictionaryCreate() == NULL\n");
    exit(1);
  }

  rv = DictionaryAdd(dict, "key",3,"value",5, 0);
  if (!rv)
  {
    fprintf(stderr, "Error: DictionaryAdd() == 0\n");
    DisposeHandle(dict);
    exit(1);
  }

  rv = DictionaryContains(dict, "key",3);
  if (!rv)
  {
    fprintf(stderr, "Error: DictionaryContains() == 0\n");
    DisposeHandle(dict);
    exit(1);
  }

  rv = DictionaryCount(dict);
  if (rv != 1)
  {
    fprintf(stderr, "Error: DictionaryCount() != 1\n");
    DisposeHandle(dict);
    exit(1);
  }

  rv = DictionaryAdd(dict, "key2",4, "value2", 6, false);
  if (!rv)
  {
    fprintf(stderr, "Error: DictionaryAdd() == 0\n");
    DisposeHandle(dict);
    exit(1);
  }

  {
    Word cookie = 0;
    DictionaryEnumerator e;

    while ((cookie = DictionaryEnumerate(dict, &e, cookie)) != 0)
    {
      fwrite(e.key, 1, e.keySize, stdout);
      fwrite(" -> ", 1, 4, stdout);
      fwrite(e.value, 1, e.valueSize, stdout);
      fputc('\n', stdout);
    }
  }



  DisposeHandle(dict);
  exit(0);
  return 0;
}
