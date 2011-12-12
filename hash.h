#define HASH_SPACE_SIZE 32768
unsigned long _hash(unsigned char *str);
unsigned int hash(unsigned char *str, int salt);
int distance(int a, int b);
