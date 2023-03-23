extern int GET();
extern void *MALLOC(int);
extern void FREE(void *);
extern void PRINT(int);

// handle if stmt

int main() {
  int a;
  a = 1;

  if (a == 1) {
    PRINT(1);
  } else {
    PRINT(0);
  }

  if (a == 1) {
    PRINT(1);
  }
}