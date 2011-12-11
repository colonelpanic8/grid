#include "hash.h"

unsigned long sdbm(unsigned char *str) {
  unsigned long hash = 0;
  int c;
  while (c = *str++)
    hash = c + (hash << 6) + (hash << 16) - hash;
  return hash;
}

unsigned int hash(unsigned char *str) {
  return (int)(sdbm(str) % HASH_SPACE_SIZE);
}

int distance(int a, int b) {
  return (HASH_SPACE_SIZE - abs(a-b)) < abs(a-b) ? (HASH_SPACE_SIZE - abs(a-b)) : abs(a-b);
}
