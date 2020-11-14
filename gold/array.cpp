struct Array {
  static Array *create(int sz) {
    auto a = new Array;
    a->data = new int[sz];
    return a;
  }

  int *data;
};

int main() {
  auto a = new int[10];
  a[5] = 10;

  return a[5];
}
