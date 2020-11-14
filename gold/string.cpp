struct String {
  static String *create(int sz) {
    auto a = new String;
    a->data = new char[sz];
    a->capacity = sz;
    a->size = 0;
    return a;
  }

  char get(int i) { return data[i]; }

  void set(int i, char c) { data[i] = c; }

  int capacity;
  int size;
  char *data;
};

int main() {
  auto s = String::create(10);
  s->set(0, 'a');

  return s->get(0);
}
