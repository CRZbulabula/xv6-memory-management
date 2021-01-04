#include "types.h"
#include "stat.h"
#include "user.h"

void recursion(int n) {
  if (n == 0) {
    printf(1, "Recursion invoke finished.---------\n");
    return;
  }
  int arrayOnStack[1024];
  printf(1, "Recursion invoke pushing stack[%x]: %d.\n", arrayOnStack, n);
  for (int i = 0; i < 1024; i++) {
    arrayOnStack[i] = i;
  }

  recursion(n - 1);
  printf(1, "Recursion invoke popping stack[%x]: %d.\n", arrayOnStack, n);
}

int main() {
  printf(1, "================================\n");
  printf(1, "Stack grow test begin. Test by %d recursion.\n");
  int depth = 256;
  recursion(depth);
  printf(1, "Stack grow test finish.\n");
  printf(1, "================================\n");
  return 0;
}