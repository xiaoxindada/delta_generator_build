constexpr int LOOP_COUNT = 100000000;

void Function1() {
  for (volatile int i = 0; i < LOOP_COUNT; i += 1) {
  }
}

int main() {
  while (true) {
    Function1();
  }
  return 0;
}
