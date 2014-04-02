#include <cmath>
#include <cstdio>
#include <cstdlib>

int main(int argc, char **argv) {
  const char* prog = argv[0];
  const bool quit = [&]{
    if (argc > 1 && argv[1][0] == '-' && argv[1][1] == 'q') {
      --argc, ++argv;
      return true;
    }
    return false;
  }();
  if (argc < 3) {
    fprintf(stderr, "Usage: %s [-q] MM_OUT_EXP MM_OUT_ACT [N]\n", prog);
    return 1;
  }
  FILE* f_exp = fopen(argv[1], "r");
  FILE* f_act = fopen(argv[2], "r");
  const int n = (argc > 3 ? atoi(argv[3]) : 1);

  bool ok = true;
  double exp, act;
  for (int row = 0, col = 0; (!quit || ok) &&
       fscanf(f_exp, "%lf", &exp) == 1 && fscanf(f_act, "%lf", &act) == 1;) {
    if (fabs(exp - act) > 1e-9) {
      printf("(%d, %d) not equal\n  Actual: %lf\nExpected: %lf\n", row, col,
             exp, act);
      ok = false;
    }
    if (++row == n) ++col, row -= n;
  }

  return !ok;
}
