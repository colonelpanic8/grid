#define HASH_SPACE_SIZE 16777216

#include "hash.h"

unsigned long sdbm(unsigned char *str) {
  unsigned long hash = 0;
  int c;
  while (c = *str++)
    hash = c + (hash << 6) + (hash << 16) - hash;
  return hash;
}

int hash(unsigned char *str) {
  return (int)(sdbm(str) % HASH_SPACE_SIZE);
}

int distance(unsigned long a, unsigned long b) {
  return abs(a-b);
}
