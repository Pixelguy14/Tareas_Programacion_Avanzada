// Tarea 8 - Suma y resta de matrices con AVX2.

#include "Tarea8.h"

#include <immintrin.h>

namespace tarea8 {

// Entrada: A,B que son matrices nxn
// Salida: C que es una matriz nxn
template <bool SUMAR>
static void operar_elemento_a_elemento(const double* A, const double* B,
                                       double* C, int n) {
  if (n <= 0) {
    return;
  }

  const long long total = static_cast<long long>(n) * n;

  // OpenMP exige la forma canónica "i < limite; i += 4".  Por eso el
  // ciclo vectorial termina en el múltiplo de 4 más cercano y los
  // sobrantes se atienden aparte.
  const long long limite_vectorial = total - (total % 4);

  // Cada posición de C es independiente, así que se puede paralelizar.
#ifdef _OPENMP
#pragma omp parallel for schedule(static)
#endif
  for (long long i = 0; i < limite_vectorial; i += 4) {
    __m256d a = _mm256_loadu_pd(&A[i]);
    __m256d b = _mm256_loadu_pd(&B[i]);

    // El compilador elige una sola de las dos instrucciones porque SUMAR
    // es un parámetro de plantilla conocido en tiempo de compilación.
    // Como la suma y la resta son operaciones elemento a elemento, no importa
    // la forma de la matriz: se procesan 4 doubles (256 bits) por iteración.
    __m256d c = SUMAR ? _mm256_add_pd(a, b) : _mm256_sub_pd(a, b);

    _mm256_storeu_pd(&C[i], c);
  }

  // Elementos sobrantes cuando n*n no es múltiplo de 4.
  for (long long i = limite_vectorial; i < total; ++i) {
    C[i] = SUMAR ? A[i] + B[i] : A[i] - B[i];
  }
}

void suma_matrices(const double* A, const double* B, double* C, int n) {
  operar_elemento_a_elemento<true>(A, B, C, n);
}

void resta_matrices(const double* A, const double* B, double* C, int n) {
  operar_elemento_a_elemento<false>(A, B, C, n);
}

}  // namespace tarea8
