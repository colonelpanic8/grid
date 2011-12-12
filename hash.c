#include "hash.h"
#include <stdio.h>
#define SIZE 256

unsigned long _hash(unsigned char *str)
{
  unsigned long hash = 2231253;
  int c;
  while (c = *str++)
    hash = ((hash << 5) + hash) + c;
  return hash;
}

unsigned int hash(unsigned char *str, int salt) {
  char buffer[256];
  sprintf(buffer, "%s%d", str, salt);
  return (int)(_hash(buffer) & (HASH_SPACE_SIZE-1));
}

int distance(int a, int b) {
  return (HASH_SPACE_SIZE - abs(a-b)) < abs(a-b) ? (HASH_SPACE_SIZE - abs(a-b)) : abs(a-b);
}
