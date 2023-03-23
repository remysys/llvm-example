extern int GET();
extern void *MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

int main() {
  int x = 0;
  int y = 20;
  if (x) {
    PRINT(1);
  } else {
    PRINT(0);
  }
  if ((x = 10) > 1) {
    PRINT(x);
    PRINT(y);
  }
}