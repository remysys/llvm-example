extern int GET();
extern void *MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

// simple call handling

int f(int x) {
  x = 10;
  if (x > 9) {
    PRINT(202);
  }
  // int b = 1211;
  return x;
}

int main() {
  int a;
  // a = f(10);
  PRINT(f(10));
}