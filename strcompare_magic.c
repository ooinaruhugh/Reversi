#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

int main(void)
{
  char *str1 = "init: A";
  char *str = calloc(1, 100);
  fgets(str, 99, stdin);
  printf("%s\n", str);

  uint64_t *test = str;
  printf("%i\n", (*test & 0xFFFFFFFFFFFF) == 0x203A74696E69);

  // Forgive me
#define INIT_MAGIC 0x203A74696E69
// Maybe
#define INIT_X_MAGIC 0x58203A74696E69
#define INIT_O_MAGIC 0x4F203A74696E69

  if ((*test & 0xFFFFFFFFFFFF) == INIT_MAGIC)
  {
    if ((*test & 0xFFFFFFFFFFFFFF) ^ INIT_X_MAGIC && (*test & 0xFFFFFFFFFFFFFF)  ^ INIT_O_MAGIC)
    {
      fprintf(stderr, "Illegal stone: 0x%llx\n", *test);
      // exit(0);
    }
  }

  uint64_t my_stone = *test - INIT_MAGIC;
  printf("%llx %llx\n", my_stone, my_stone >> ((__builtin_ctzll(my_stone) / 8) * 8));
  char c = 0xFF & (my_stone >> __builtin_ctzll(my_stone));
  printf("%c\n", c);

#define SRAND_DPS_MAGIC 0x203A646E617273

  char *test2 = memcpy(malloc(strlen("srand: 1234567") + 1), "srand: 1234567", strlen("srand: 1234567") + 1);
  uint64_t *test2_bin = test2;
  *test2_bin -= SRAND_DPS_MAGIC - 0x20202020202020;
  printf("%i\n", atoi(test2));
  return 0;
}