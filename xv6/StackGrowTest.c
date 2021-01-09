#include "types.h"
#include "stat.h"
#include "user.h"

void testKB(int n) {
  if (n == 0) {
    printf(1, "Recursion finished.---------\n");
    return;
  }
  char array[1024];
  printf(1, "testKB n = %d, pushing stack[%x] %d bytes.\n", n, array, 1024);
  // test write
  for (int i = 0; i < 1024; i++) {
      array[i] = i%255;
  }
  testKB(n - 1);

  printf(1, "testKB n = %d, popping stack[%x].\n", n, array);
}

void testMB(int n) {
  if (n == 0) {
    printf(1, "Recursion finished.---------\n");
    return;
  }
  char array[1024][1024];
  printf(1, "testMB n = %d, pushing stack[%x] %d bytes.\n", n, array, 1024*1024);
  // test write
  for (int i = 0; i < 1024; i++) {
    for (int j=0; j<1024; j++)
      array[i][j] = i%255;
  }
  testMB(n - 1);

  printf(1, "testMB n = %d, popping stack[%x].\n", n, array);
}

#define TestMB 251

int main() {
  #ifndef TestMB
  printf(1, "==================================\n");
  printf(1, "Stack grow test begin.\n");
  int depth = 256;
  printf(1, "Test Stack size [%dKB] by %d recursion.\n", depth, depth);
  testKB(depth);
  printf(1, "=========Test Stack Size [%dKB] Success.=========\n", depth);

  depth = 249;
  printf(1, "Test Stack size [%dMB] by %d recursion.\n", depth, depth);
  testMB(depth);
  printf(1, "=========Test Stack Size [%dMB] Success.=========\n", depth);
  #endif

  #ifdef TestMB
  printf(1, "Test Stack size [%dMB] by claim.\n", TestMB);
  char array[TestMB][1024][1024];
  for (int i=0; i<TestMB; i++)
    for (int j=0; j<1024; j++)
      for (int k=0; k<1024; k++)
        array[i][j][k] = k%255;
  printf(1, "=========Test Stack Size [%dMB] Success.=========\n", TestMB);
  #endif

  printf(1, "Stack grow test finish.\n");
  printf(1, "==================================\n");
  return 0;
}