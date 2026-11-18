// Tarea 8 - Producto punto con AVX2/FMA.

#include "Tarea8.h"

#include <immintrin.h>

namespace tarea8 {

// Entrada: vector u y v
// Salida: sum(u[i] * v[i])
double producto_punto(const double* u, const double* v, int n) {
  if (n <= 0) {
    return 0.0;
  }

  __m256d acumulado0 = _mm256_setzero_pd();
  __m256d acumulado1 = _mm256_setzero_pd();

  int i = 0;

  // Dos vectores de 4 doubles por vuelta = 8 elementos.
  // Procesa 8 elementos por iteración usando dos acumuladores __m256d (4 doubles cada uno)
  // Dos acumuladores permiten que el procesador no espere siempre por el mismo registro
  for (; i + 8 <= n; i += 8) {
    __m256d u0 = _mm256_loadu_pd(&u[i]);
    __m256d v0 = _mm256_loadu_pd(&v[i]);
    acumulado0 = _mm256_fmadd_pd(u0, v0, acumulado0);

    __m256d u1 = _mm256_loadu_pd(&u[i + 4]);
    __m256d v1 = _mm256_loadu_pd(&v[i + 4]);
    acumulado1 = _mm256_fmadd_pd(u1, v1, acumulado1);
  }

  // Un vector de 4 doubles si todavía quedan al menos 4 elementos.
  for (; i + 4 <= n; i += 4) {
    __m256d u0 = _mm256_loadu_pd(&u[i]);
    __m256d v0 = _mm256_loadu_pd(&v[i]);
    acumulado0 = _mm256_fmadd_pd(u0, v0, acumulado0);
  }

  // Suma horizontal de los dos acumuladores:
  //   acumulado0 = [a0, a1, a2, a3]
  //   acumulado1 = [b0, b1, b2, b3]
  //   total = a0+a1+a2+a3 + b0+b1+b2+b3
  __m256d suma_vectores = _mm256_add_pd(acumulado0, acumulado1);

  // Al final se suman los 4 doubles de cada acumulador y se agregan los
  // elementos sobrantes de forma escalar.
  double parciales[4];
  _mm256_storeu_pd(parciales, suma_vectores);

  double resultado =
      parciales[0] + parciales[1] + parciales[2] + parciales[3];

  // Elementos sobrantes (n no múltiplo de 4).
  for (; i < n; ++i) {
    resultado += u[i] * v[i];
  }

  return resultado;
}

}  // namespace tarea8
