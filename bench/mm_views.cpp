//
// Program that tests the performance of the
// Volker Strassen algorithm for matrix multiplication.
//
// W. Cochran  wcochran@vancouver.wsu.edu
//
// Trivially modified by Leo Osvald (losvald@purdue.edu):
// - print average element of result matrix (prevents compiler optimizations)
// - read N from command line args (prevents compiler optimizations)
// - add #include <time.h> (otherwise it doesn't compile)
// - don't run classic O(N^3) implementation used for reference
// - print timing information to stderr, use stdout to printing checksum (avg);
//   if second cmd args is supplied, print the resulting matrix Z (for testing)
// - make timing portable and reliable (use monotonic clock)

#include "../src/list.hpp"

#include <stdlib.h>
#include <stdio.h>

//
// Classic O(N^3) square matrix multiplication.
// Z = X*Y
// All matrices are NxN and stored in row major order
// each with a specified pitch.
// The pitch is the distance (in double's) between
// elements at (row,col) and (row+1,col).
//
void mmult(int N,
           int Xpitch, const double X[],
           int Ypitch, const double Y[],
           int Zpitch, double Z[]) {
  // the slow way - using a function pointer under the hood (runtime overhead)
  // v::ImplicitList<const double> Xview([=](int index) {
  //     int r = index / N, c = index - r * N;
  //     return &X[r*Xpitch + c];
  //   }, N*N);
  // v::ImplicitList<const double> Yview([=](int index) {
  //     int r = index / N, c = index - r * N;
  //     return &Y[r*Ypitch + c];
  //   }, N*N);

  // a faster way - using the template functors (no runtime overhead)
  // the verbose way: 1) create a functor 2) create List directly using ctor
  class Accessor {
    const double* arr_;
    const int pitch_, N_;
   public:
    Accessor(const double* arr, int pitch, int N)
        : arr_(arr), pitch_(pitch), N_(N) {}
    inline const double* operator()(unsigned index) const {
      int r = index / N_, c = index - r * N_;
      return &arr_[r*pitch_ + c];
    }
  };
  v::ImplicitList<const double, Accessor> Xview(Accessor(X, Xpitch, N), N*N);
  v::ImplicitList<const double, Accessor> Yview(Accessor(Y, Ypitch, N), N*N);

  // non-verbose way: 1) create an anonymous functor and pass it to MakeList
  auto Zview = v::MakeList([=](unsigned index) {
      int r = index / N, c = index - r * N;
      return &Z[r*Zpitch + c];
    }, N*N);

  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      double sum = 0.0;
      for (int k = 0; k < N; k++)
        sum += //X[i*Xpitch + k]*Y[k*Ypitch + j];
            Xview.get(i*N + k) *
            Yview.get(k*N + j);
      // Z[i*Zpitch + j] = sum;
      Zview.set(i*N + j, sum);
    }
}

//
// S = X + Y
//
void madd(int N,
          int Xpitch, const double X[],
          int Ypitch, const double Y[],
          int Spitch, double S[]) {
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      S[i*Spitch + j] = X[i*Xpitch + j] + Y[i*Ypitch + j];
}

//
// S = X - Y
//
void msub(int N,
          int Xpitch, const double X[],
          int Ypitch, const double Y[],
          int Spitch, double S[]) {
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      S[i*Spitch + j] = X[i*Xpitch + j] - Y[i*Ypitch + j];
}

//
// Volker Strassen algorithm for matrix multiplication.
// Theoretical Runtime is O(N^2.807).
// Assume NxN matrices where N is a power of two.
// Algorithm:
//   Matrices X and Y are split into four smaller
//   (N/2)x(N/2) matrices as follows:
//          _    _          _   _
//     X = | A  B |    Y = | E F |
//         | C  D |        | G H |
//          -    -          -   -
//   Then we build the following 7 matrices (requiring
//   seven (N/2)x(N/2) matrix multiplications -- this is
//   where the 2.807 = log2(7) improvement comes from):
//     P0 = A*(F - H);
//     P1 = (A + B)*H
//     P2 = (C + D)*E
//     P3 = D*(G - E);
//     P4 = (A + D)*(E + H)
//     P5 = (B - D)*(G + H)
//     P6 = (A - C)*(E + F)
//   The final result is
//        _                                            _
//   Z = | (P3 + P4) + (P5 - P1)   P0 + P1              |
//       | P2 + P3                 (P0 + P4) - (P2 + P6)|
//        -                                            -
//
void mmult_fast(int N,
                int Xpitch, const double X[],
                int Ypitch, const double Y[],
                int Zpitch, double Z[]) {
  //
  // Recursive base case.
  // If matrices are 16x16 or smaller we just use
  // the conventional algorithm.
  // At what size we should switch will vary based
  // on hardware platform.
  //
  if (N <= 16) {
    mmult(N, Xpitch, X, Ypitch, Y, Zpitch, Z);
    return;
  }

  const int n = N/2;      // size of sub-matrices

  const double *A = X;    // A-D matrices embedded in X
  const double *B = X + n;
  const double *C = X + n*Xpitch;
  const double *D = C + n;

  const double *E = Y;    // E-H matrices embeded in Y
  const double *F = Y + n;
  const double *G = Y + n*Ypitch;
  const double *H = G + n;

  double *P[7];   // allocate temp matrices off heap
  const int sz = n*n*sizeof(double);
  for (int i = 0; i < 7; i++)
    P[i] = (double *) malloc(sz);
  double *T = (double *) malloc(sz);
  double *U = (double *) malloc(sz);

  // P0 = A*(F - H);
  msub(n, Ypitch, F, Ypitch, H, n, T);
  mmult_fast(n, Xpitch, A, n, T, n, P[0]);

  // P1 = (A + B)*H
  madd(n, Xpitch, A, Xpitch, B, n, T);
  mmult_fast(n, n, T, Ypitch, H, n, P[1]);

  // P2 = (C + D)*E
  madd(n, Xpitch, C, Xpitch, D, n, T);
  mmult_fast(n, n, T, Ypitch, E, n, P[2]);

  // P3 = D*(G - E);
  msub(n, Ypitch, G, Ypitch, E, n, T);
  mmult_fast(n, Xpitch, D, n, T, n, P[3]);

  // P4 = (A + D)*(E + H)
  madd(n, Xpitch, A, Xpitch, D, n, T);
  madd(n, Ypitch, E, Ypitch, H, n, U);
  mmult_fast(n, n, T, n, U, n, P[4]);

  // P5 = (B - D)*(G + H)
  msub(n, Xpitch, B, Xpitch, D, n, T);
  madd(n, Ypitch, G, Ypitch, H, n, U);
  mmult_fast(n, n, T, n, U, n, P[5]);

  // P6 = (A - C)*(E + F)
  msub(n, Xpitch, A, Xpitch, C, n, T);
  madd(n, Ypitch, E, Ypitch, F, n, U);
  mmult_fast(n, n, T, n, U, n, P[6]);

  // Z upper left = (P3 + P4) + (P5 - P1)
  madd(n, n, P[4], n, P[3], n, T);
  msub(n, n, P[5], n, P[1], n, U);
  madd(n, n, T, n, U, Zpitch, Z);

  // Z lower left = P2 + P3
  madd(n, n, P[2], n, P[3], Zpitch, Z + n*Zpitch);

  // Z upper right = P0 + P1
  madd(n, n, P[0], n, P[1], Zpitch, Z + n);

  // Z lower right = (P0 + P4) - (P2 + P6)
  madd(n, n, P[0], n, P[4], n, T);
  madd(n, n, P[2], n, P[6], n, U);
  msub(n, n, T, n, U, Zpitch, Z + n*(Zpitch + 1));

  free(U);  // deallocate temp matrices
  free(T);
  for (int i = 6; i >= 0; i--)
    free(P[i]);
}

void mprint(int N, int pitch, const double M[]) {
  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++)
      printf("%+0.4f ", M[i*pitch + j]);
    printf("\n");
  }
}

#ifdef MM_TEST1

int main(void) {
  double X[4*4] = {
    1, 2, 3, 1,
    -1, 1, 2, 3,
    0, 4, 5, -3,
    -1, 1, 2, 3
  };
  double Y[4*4] = {
    1, 2, 3, 4,
    4, 3, 2, 1,
    -1, -1, 2, 2,
    3, 0, 1, 2
  };
  double Z[4*4];
  mmult(4, 4, X, 4, Y, 4, Z);
  mprint(4, 4, Z);
  printf("=========\n");

  double Zfast[4*4];
  mmult_fast(4, 4, X, 4, Y, 4, Zfast);
  mprint(4, 4, Zfast);

  return 0;
}

#else

void mrand(int N, int pitch, double M[]) {
  const double r = 10.0;
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      M[i*pitch + j] = r*(2*drand48() - 1);
}

#include <chrono>

int main(int argc, char *argv[]) {
  int N = atoi(argv[1]);

  double *X, *Y, *Z;
  X = (double*) malloc(N*N*sizeof(double));
  Y = (double*) malloc(N*N*sizeof(double));
  Z = (double*) malloc(N*N*sizeof(double));

  mrand(N, N, X);
  mrand(N, N, Y);

  {
    using namespace std::chrono;
    auto tp_before = steady_clock::now();
    mmult_fast(N, N, X, N, Y, N, Z);
    auto tp_after = steady_clock::now();
    fprintf(stderr, "%lf\n", 1e-9 * duration_cast<nanoseconds>(
        tp_after - tp_before).count());
  }

  // print the average element value to prevent compiler optimizations
  double avg = 0;
  double *Zfrom = Z, *Zto = Z;
  for (int i = 0; i < N; ++i) {
    double row_avg = 0.0;
    for (Zto += N; Zfrom != Zto; )
      row_avg += *Zfrom++;
    avg += row_avg / N;
  }
  fprintf(stderr, "Average element: %lf\n", avg);

  if (argc > 2) { // print the resulting matrix if 2nd cmd arg is supplied
    Zfrom = Z, Zto = Z;
    for (int i = 0; i < N; ++i)
      for (Zto += N; Zfrom != Zto; )
        printf("%lf\n", *Zfrom++);
  }

  return 0;
}

#endif
