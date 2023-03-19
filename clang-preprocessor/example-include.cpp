#include <stdio.h>

int main() {
  printf("hello world!\n");
  return 0;
}

template <typename T, int n>
struct s {
  T array[n];
};

int main() {
  s<int, 20> var;
}
