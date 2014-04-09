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
#include "../src/util/libdivide.h"

#include <stdlib.h>
#include <stdio.h>

template<typename T>
class Accessor {
  T* arr_;
  const int kPitch_, kN_;
  const libdivide::divider<unsigned> n_;
 public:
  Accessor(unsigned n, unsigned pitch, T* arr)
      : arr_(arr), kPitch_(pitch), kN_(n), n_(n) {}
  inline T* operator()(unsigned index) const {
    unsigned r = index / n_, c = index - r * kN_;
    return &arr_[r*kPitch_ + c];
  }
};

template<typename T>
using MatrixView = v::ImplicitList<T, Accessor<T> >;

template<typename T>
constexpr MatrixView<T> MakeMatrixView(unsigned n, unsigned pitch, T* arr) {
  return MatrixView<T>(Accessor<T>(n, pitch, arr), n*n);
}

template<class V>
inline typename V::DataType* ArrayBegin(const V& v) {
  // a hack to get the address at which the underlying array begins
  return &const_cast<typename V::DataType&>(v.get(0));
}

//
// Classic O(N^3) square matrix multiplication.
// Z = X*Y
// All matrices are NxN and stored in row major order
// each with a specified pitch.
// The pitch is the distance (in double's) between
// elements at (row,col) and (row+1,col).
//
void mmult(int N,
           const MatrixView<double>& Xv,
           const MatrixView<double>& Yv,
           const MatrixView<double>& Zv) {
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++) {
      double sum = 0.0;
      for (int k = 0; k < N; k++)
        sum += Xv.get(i*N + k) * Yv.get(k*N + j);
      Zv.set(sum, i*N + j);
    }
}

//
// S = X + Y
//
void madd(int N,
          const MatrixView<double>& Xv,
          const MatrixView<double>& Yv,
          const MatrixView<double>& Sv) {
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      Sv.set(Xv.get(i*N + j) + Yv.get(i*N + j), i*N + j);
}

//
// S = X - Y
//
void msub(int N,
          const MatrixView<double>& Xv,
          const MatrixView<double>& Yv,
          const MatrixView<double>& Sv) {
  for (int i = 0; i < N; i++)
    for (int j = 0; j < N; j++)
      Sv.set(Xv.get(i*N + j) - Yv.get(i*N + j), i*N + j);
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
                const MatrixView<double>& Xv,
                const MatrixView<double>& Yv,
                const MatrixView<double>& Zv) {
  //
  // Recursive base case.
  // If matrices are 16x16 or smaller we just use
  // the conventional algorithm.
  // At what size we should switch will vary based
  // on hardware platform.
  //
  if (N <= 16) {
    mmult(N, Xv, Yv, Zv);
    return;
  }

  const int n = N/2;      // size of sub-matrices
  const int Xpitch = (&Xv.get(N) - &Xv.get(0));
  const int Ypitch = (&Yv.get(N) - &Yv.get(0));
  const int Zpitch = (&Zv.get(N) - &Zv.get(0));

  // A-D matrices embedded in X
  auto Av = MakeMatrixView(n, Xpitch, ArrayBegin(Xv));
  auto Bv = MakeMatrixView(n, Xpitch, ArrayBegin(Xv) + n);
  auto Cv = MakeMatrixView(n, Xpitch, ArrayBegin(Xv) + n*Xpitch);
  auto Dv = MakeMatrixView(n, Xpitch, ArrayBegin(Xv) + n*Xpitch + n);

  // E-H matrices embeded in Y
  auto Ev = MakeMatrixView(n, Ypitch, ArrayBegin(Yv));
  auto Fv = MakeMatrixView(n, Ypitch, ArrayBegin(Yv) + n);
  auto Gv = MakeMatrixView(n, Ypitch, ArrayBegin(Yv) + n*Ypitch);
  auto Hv = MakeMatrixView(n, Ypitch, ArrayBegin(Yv) + n*Ypitch + n);

  double *P[7];   // allocate temp matrices off heap
  std::vector<MatrixView<double> > Pv;//[7];
  Pv.reserve(7);
  const int sz = n*n*sizeof(double);
  for (int i = 0; i < 7; i++) {
    P[i] = (double *) malloc(sz);
    Pv.push_back(MakeMatrixView(n, n, P[i]));
  }
  double *T = (double *) malloc(sz);
  double *U = (double *) malloc(sz);
  MatrixView<double> Tv = MakeMatrixView(n, n, T);
  MatrixView<double> Uv = MakeMatrixView(n, n, U);

  // P0 = A*(F - H);
  msub(n, Fv, Hv, Tv);
  mmult_fast(n, Av, Tv, Pv[0]);

  // P1 = (A + B)*H
  madd(n, Av, Bv, Tv);
  mmult_fast(n, Tv, Hv, Pv[1]);

  // P2 = (C + D)*E
  madd(n, Cv, Dv, Tv);
  mmult_fast(n, Tv, Ev, Pv[2]);

  // P3 = D*(G - E);
  msub(n, Gv, Ev, Tv);
  mmult_fast(n, Dv, Tv, Pv[3]);

  // P4 = (A + D)*(E + H)
  madd(n, Av, Dv, Tv);
  madd(n, Ev, Hv, Uv);
  mmult_fast(n, Tv, Uv, Pv[4]);

  // P5 = (B - D)*(G + H)
  msub(n, Bv, Dv, Tv);
  madd(n, Gv, Hv, Uv);
  mmult_fast(n, Tv, Uv, Pv[5]);

  // P6 = (A - C)*(E + F)
  msub(n, Av, Cv, Tv);
  madd(n, Ev, Fv, Uv);
  mmult_fast(n, Tv, Uv, Pv[6]);

  // Z upper left = (P3 + P4) + (P5 - P1)
  madd(n, Pv[4], Pv[3], Tv);
  msub(n, Pv[5], Pv[1], Uv);
  madd(n, Tv, Uv, MakeMatrixView(n, Zpitch, ArrayBegin(Zv)));

  // Z lower left = P2 + P3
  madd(n, Pv[2], Pv[3], MakeMatrixView(n, Zpitch, ArrayBegin(Zv) + n*Zpitch));

  // Z upper right = P0 + P1
  madd(n, Pv[0], Pv[1], MakeMatrixView(n, Zpitch, ArrayBegin(Zv) + n));

  // Z lower right = (P0 + P4) - (P2 + P6)
  madd(n, Pv[0], Pv[4], Tv);
  madd(n, Pv[2], Pv[6], Uv);
  msub(n, Tv, Uv, MakeMatrixView(n, Zpitch, ArrayBegin(Zv) + n*(Zpitch + 1)));

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
    mmult_fast(N, MakeMatrixView(N, N, X), MakeMatrixView(N, N, Y),
               MakeMatrixView(N, N, Z));
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
