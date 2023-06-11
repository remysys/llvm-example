namespace foo {
struct A1 {
  struct A2 {
    struct A3 {
      struct A4 {};
    };
  };
};

struct B {};

}  // namespace foo

struct Foo {
  struct Bar {};
};

union Something {
  union Wazzit {
    float f;
    char c;
  };
  union {
    char c;
  };
  int i;
  float f;
};