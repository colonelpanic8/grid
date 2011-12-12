#define HASH_SPACE_SIZE 16777216
unsigned long _hash(unsigned char *str);
unsigned int hash(unsigned char *str, int salt);
int distance(int a, int b);
