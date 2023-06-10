int expr(int op, int lvalue, int rvalue) {
  if (op == '+') {
    return lvalue + rvalue;
  } else {
    return lvalue - rvalue;
  }
}

int main() {
  expr('+', 1, 4);
  return 0;
}
