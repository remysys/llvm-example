extern int GET();
extern void* MALLOC(int);
extern void FREE(void*);
extern void PRINT(int);

int* f(int b) {
  int* a = (int*)MALLOC(sizeof(int));
  *a = b;
  return a;
}

int main() {
  int* a;

  a = f(1211);

  PRINT(*a);
  FREE(a);
}