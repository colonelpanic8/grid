#define HASH_SPACE_SIZE 1000000

#include "hash.h"

unsigned long sdbm(unsigned char *str) {
  unsigned long hash = 0;
  int c;
  while (c = *str++)
    hash = c + (hash << 6) + (hash << 16) - hash;
  return hash;
}

unsigned long distance(unsigned long a, unsigned long b) {
  return abs(a-b);
}
