extern int GET();
extern void *MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
  int a[3];
  PRINT(a[2]);
  a[2] = 10;
  PRINT(a[2]);
}
