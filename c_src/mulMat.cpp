// Tarea 8 - Multiplicación de matrices con AVX2/FMA.

#include "Tarea8.h"

#include <immintrin.h>

#include <cstddef>

namespace tarea8 {

// Entrada: A, B, matrices n x n
// Salida: C matriz n x n
void mul_mat(const double* A, const double* B, double* C, int n) {
  if (n <= 0) {
    return;
  }

#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
  for (int i = 0; i < n; ++i) {
    double* fila_c = &C[(std::size_t)i * n];

    // C(i, :) = 0
    // Se procesa una fila de C a la vez: C(i, :) = 0.
    int j = 0;
    for (; j + 4 <= n; j += 4) {
      _mm256_storeu_pd(fila_c + j, _mm256_setzero_pd());
    }
    for (; j < n; ++j) {
      fila_c[j] = 0.0;
    }

    // C(i, :) += A(i, k) * B(k, :)
    // Para cada k se toma A(i, k) (un escalar) y se multiplica por la fila k de B, que está contigua en memoria.
    for (int k = 0; k < n; ++k) {
      const double aik = A[(std::size_t)i * n + k];
      const __m256d a = _mm256_set1_pd(aik);
      const double* fila_b = &B[(std::size_t)k * n];

      j = 0;
      // Se actualizan 4 columnas de C a la vez con _mm256_fmadd_pd:
      // C(i, j..j+3) += A(i,k) * B(k, j..j+3)
      for (; j + 4 <= n; j += 4) {
        __m256d b = _mm256_loadu_pd(fila_b + j);
        __m256d c = _mm256_loadu_pd(fila_c + j);
        _mm256_storeu_pd(fila_c + j, _mm256_fmadd_pd(a, b, c));
      // NumPy no garantiza que las filas empiecen en múltiplos de 32 bytes.
      }
      for (; j < n; ++j) {
        fila_c[j] += aik * fila_b[j]; //  Si n no es múltiplo de 4, las columnas sobrantes se calculan de forma escalar
      }
    }
  }
}

}  // namespace tarea8
