struct S {
  S *foo() { return this; }
  static S *bar(int init) {
    auto s = new S;
    s->f = init;
    return s;
  }
  int f;
};

int main() {
  S *s2 = S::bar(10);
  return s2->f;
}
