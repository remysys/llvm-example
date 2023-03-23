extern int GET();
extern void* MALLOC(int);
extern void FREE(void*);
extern void PRINT(int);

int f(int b) { return b + 1; }

int main() {
  int a;
  a = 10;

  while (a == 10) {
    a = f(a) + 1;
  }

  PRINT(a);
}