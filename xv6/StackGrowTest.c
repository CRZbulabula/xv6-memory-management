#include "types.h"
#include "stat.h"
#include "user.h"

void Recursion(int n) {
  if (n == 0) {
    printf(1, "Recursion finished.---------\n");
    return;
  }
  int array[1024];
  // printf(1, "Recursion n = %d, pushing stack[%x] %d sizeof(int).\n", n, array, 512);
  // test write
  for (int i = 0; i < 1024; i++) {
    array[i] = i;
  }
  Recursion(n - 1);

  // printf(1, "Recursion n = %d, popping stack[%x].\n", n, array);
}

int main() {
  printf(1, "==================================\n");
  printf(1, "Stack grow test begin.\n");
  int depth = 2;
  printf(1, "Test Stack size [8KB] by %d recursion.\n", depth);
  Recursion(depth);
  printf(1, "=========Test Stack Size [8KB] Success.=========\n");

  depth = 2048;
  printf(1, "Test Stack size [8MB] by %d recursion.\n", depth);
  Recursion(depth);
  printf(1, "=========Test Stack Size [8MB] Success.=========\n");

  #define TestMB 10
  printf(1, "Test Stack size [%dMB] by claim.\n", TestMB);
  char array[TestMB][1024][1024];
  for (int i=0; i<TestMB; i++)
    for (int j=0; j<1024; j++)
      for (int k=0; k<1024; k++)
        array[i][j][k] = 55;
  printf(1, "=========Test Stack Size [%dMB] Success.=========\n", TestMB);

  printf(1, "Stack grow test finish.\n");
  printf(1, "==================================\n");
  return 0;
}