// Tarea 8 - Handler C/C++ de la biblioteca.
//
// Este archivo funciona como handler del API para que Python puede cargar con ctypes.
//   bash build_tarea8.sh
//
// Demo:
// g++ -O3 -mavx2 -mfma -fopenmp -DTAREA8_DEMO Tarea8.cpp mulMat.cpp productoPunto.cpp sumaMatrices.cpp -o Tarea8.bin

#include "Tarea8.h"

#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <vector>

extern "C" {

void t8_mul_mat(const double* A, const double* B, double* C, int n) {
  if (A == nullptr || B == nullptr || C == nullptr || n <= 0) {
    return;
  }
  tarea8::mul_mat(A, B, C, n);
}

void t8_suma_matrices(const double* A, const double* B, double* C, int n) {
  if (A == nullptr || B == nullptr || C == nullptr || n <= 0) {
    return;
  }
  tarea8::suma_matrices(A, B, C, n);
}

void t8_resta_matrices(const double* A, const double* B, double* C, int n) {
  if (A == nullptr || B == nullptr || C == nullptr || n <= 0) {
    return;
  }
  tarea8::resta_matrices(A, B, C, n);
}

double t8_producto_punto(const double* u, const double* v, int n) {
  if (u == nullptr || v == nullptr || n <= 0) {
    return 0.0;
  }
  return tarea8::producto_punto(u, v, n);
}

}  // extern "C"


// Demo opcional
#ifdef TAREA8_DEMO

static double segundos_desde(std::chrono::steady_clock::time_point inicio) {
  auto fin = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(fin - inicio).count();
}

int main(int argc, char** argv) {
  int n = (argc > 1) ? std::atoi(argv[1]) : 256;
  if (n <= 0) {
    std::fprintf(stderr, "n debe ser positivo\n");
    return 1;
  }

  const std::size_t total = static_cast<std::size_t>(n) * n;

  std::vector<double> A(total);
  std::vector<double> B(total);
  std::vector<double> C(total);

  for (std::size_t i = 0; i < total; ++i) {
    A[i] = (i % 7) * 0.5 + 1.0;
    B[i] = ((i * 3) % 11) * 0.25;
  }

  auto t0 = std::chrono::steady_clock::now();
  tarea8::mul_mat(A.data(), B.data(), C.data(), n);
  double t_mul = segundos_desde(t0);

  t0 = std::chrono::steady_clock::now();
  tarea8::suma_matrices(A.data(), B.data(), C.data(), n);
  double t_suma = segundos_desde(t0);

  t0 = std::chrono::steady_clock::now();
  tarea8::resta_matrices(A.data(), B.data(), C.data(), n);
  double t_resta = segundos_desde(t0);

  t0 = std::chrono::steady_clock::now();
  double punto = tarea8::producto_punto(A.data(), B.data(), n * n);
  double t_punto = segundos_desde(t0);

  std::printf("n = %d\n", n);
  std::printf("mul_mat        : %.6f s\n", t_mul);
  std::printf("suma_matrices  : %.6f s\n", t_suma);
  std::printf("resta_matrices : %.6f s\n", t_resta);
  std::printf("producto_punto : %.6f s (resultado %.6f)\n", t_punto, punto);

  return 0;
}

#endif  // TAREA8_DEMO
