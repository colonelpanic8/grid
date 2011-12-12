#include <stdio.h>
#include <unistd.h>
#include "hash.h"

int main() {
  char buffer[256];
  int salt;
  while(1) {
    printf("Enter a string\n");
    scanf("%s", buffer);
    printf("Enter a number\n");
    scanf("%d", &salt);
    printf("hash in space: %d, actual hash: %ld\n", hash(buffer, salt), _hash(buffer));
  }
}
