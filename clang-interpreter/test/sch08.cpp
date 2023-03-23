extern int GET();
extern void *MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int f() { return 10000000; }

int main() {
  int x = f();
  PRINT(x);
  x = 11;
  PRINT(x);
}