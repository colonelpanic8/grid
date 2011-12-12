#include "hash.h"

unsigned long _hash(unsigned char *str, int salt)
{
  unsigned long hash = 5381;
  int c;
  hash += salt;
  while (c = *str++)
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
  return hash;
}

unsigned int hash(unsigned char *str, int salt) {
  return (int)(_hash(str, salt) % HASH_SPACE_SIZE);
}

int distance(int a, int b) {
  return (HASH_SPACE_SIZE - abs(a-b)) < abs(a-b) ? (HASH_SPACE_SIZE - abs(a-b)) : abs(a-b);
}
