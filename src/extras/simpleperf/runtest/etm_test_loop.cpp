
void f1() {
  for (volatile int i = 0; i < 100; i += 1) {
  }
}

void f2() {
  for (volatile int i = 0; i < 1000; i += 1) {
  }
}

int main() {
  for (volatile int i = 0; i < 10; i += 1) {
    if (i * 3 < 6) {
      f1();
    } else {
      f2();
    }
  }
}
