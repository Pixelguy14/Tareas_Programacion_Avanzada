/*
 * Tarea 8 - Operaciones de matrices y producto punto con intrinsics ejecutadas en python
 *
 */
#ifndef TAREA8_H
#define TAREA8_H
#ifdef __cplusplus

namespace tarea8 {

void mul_mat(const double* A, const double* B, double* C, int n);

void suma_matrices(const double* A, const double* B, double* C, int n);

void resta_matrices(const double* A, const double* B, double* C, int n);

double producto_punto(const double* u, const double* v, int n);

}  // namespace tarea8

extern "C" {
#endif

/* API C para ctypes.  Todas las matrices son n x n, row-major. */
void t8_mul_mat(const double* A, const double* B, double* C, int n);
void t8_suma_matrices(const double* A, const double* B, double* C, int n);
void t8_resta_matrices(const double* A, const double* B, double* C, int n);
double t8_producto_punto(const double* u, const double* v, int n);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // TAREA8_H
