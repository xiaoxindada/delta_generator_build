constexpr int LOOP_COUNT = 5000000;

void FunctionRecursive(int loop) {
  for (volatile int i = 0; i < LOOP_COUNT; i += 1) {
  }
  if (loop > 0) {
    FunctionRecursive(loop - 1);
  }
  for (volatile int i = 0; i < LOOP_COUNT; i += 1) {
  }
}

int main() {
  while (true) {
    FunctionRecursive(10);
  }
  return 0;
}
