#include "types.h"
#include "stat.h"
#include "user.h"

void recursion(int n) {
  if (n == 0) {
    printf(1, "Recursion finished.---------\n");
    return;
  }
  int array[1024];
  printf(1, "Recursion n = %d, pushing stack[%x].\n", n, array);
  // test write
  for (int i = 0; i < 1024; i++) {
    array[i] = i;
  }
  recursion(n - 1);

  printf(1, "Recursion n = %d, popping stack[%x].\n", n, array);
}

int main() {
  printf(1, "==================================\n");
  int depth = 256;
  printf(1, "Stack grow test begin. Test by %d recursion.\n", depth);
  recursion(depth);
  printf(1, "Stack grow test finish.\n");
  printf(1, "==================================\n");
  return 0;
}