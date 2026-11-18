#!/usr/bin/env python3
"""
Tarea 8 - Handler de Python para la biblioteca C++ con intrinsics AVX2/FMA.
Fecha de entrega: Jueves 19 de Noviembre

Compilar primero la biblioteca:
    bash c_src/build_tarea8.sh


Ejecutar como demo:
    python3 py_src/Tarea8.py
    python3 py_src/Tarea8.py 512
"""

from __future__ import annotations

import ctypes
import os
import sys
from pathlib import Path
from time import perf_counter

import numpy as np


class Tarea8:

  # Carga libtarea8.so y expone las operaciones como métodos de Python.
  def __init__(self, ruta_lib=None):
    self.ruta_lib = self._buscar_biblioteca(ruta_lib)
    self.lib = ctypes.CDLL(str(self.ruta_lib))
    self._configurar_prototipos()

  @staticmethod #Busca la biblioteca en la ruta indicada, TAREA8_LIB o c_src/.
  def _buscar_biblioteca(ruta_lib):
    candidatos = []

    if ruta_lib is not None:
      candidatos.append(Path(ruta_lib))
    else:
      variable_entorno = os.environ.get("TAREA8_LIB")
      if variable_entorno:
        candidatos.append(Path(variable_entorno))

      aqui = Path(__file__).resolve().parent
      candidatos.extend([
          aqui / "libtarea8.so",
          aqui.parent / "c_src" / "libtarea8.so",
      ])

    for candidato in candidatos:
      candidato = candidato.expanduser().resolve()
      if candidato.is_file():
        return candidato

    raise FileNotFoundError( # manejo de errores para tener la biblioteca
        "No se encontró libtarea8.so. Compila la biblioteca con:\n"
        "    bash c_src/build_tarea8.sh"
    )

# Declara los tipos de los argumentos para que ctypes no adivine
  def _configurar_prototipos(self):
    puntero = ctypes.POINTER(ctypes.c_double)
    # sintaxis necesaria para declarar y usar en python
    self.lib.t8_mul_mat.argtypes = [puntero, puntero, puntero, ctypes.c_int]
    self.lib.t8_mul_mat.restype = None

    self.lib.t8_suma_matrices.argtypes = [puntero, puntero, puntero, ctypes.c_int]
    self.lib.t8_suma_matrices.restype = None

    self.lib.t8_resta_matrices.argtypes = [puntero, puntero, puntero, ctypes.c_int]
    self.lib.t8_resta_matrices.restype = None

    self.lib.t8_producto_punto.argtypes = [puntero, puntero, ctypes.c_int]
    self.lib.t8_producto_punto.restype = ctypes.c_double

  @staticmethod # El staticmethod sirve para agrupar funciones utilitarias que tienen relación lógica con la clase, pero que no leen ni modifican el estado de la instancia ni el de la clase
  def _puntero(arreglo): # Convierte un ndarray contiguo de float64 a double*
    return arreglo.ctypes.data_as(ctypes.POINTER(ctypes.c_double))

  @staticmethod
  def _matriz_cuadrada(matriz, nombre): #Copia a float64 contiguo y valida que sea una matriz cuadrada.
    matriz = np.ascontiguousarray(matriz, dtype=np.float64)
    if matriz.ndim != 2 or matriz.shape[0] != matriz.shape[1]:
      raise ValueError(f"{nombre} debe ser una matriz cuadrada")
    return matriz

  @staticmethod 
  def _vector(vector, nombre): # Copia a float64 contiguo y lo deja como vector unidimensional.
    vector = np.ascontiguousarray(vector, dtype=np.float64).reshape(-1)
    if vector.size == 0:
      raise ValueError(f"{nombre} no puede estar vacío")
    return vector

  def _operar_matrices(self, A, B, funcion):
    A = self._matriz_cuadrada(A, "A") 
    B = self._matriz_cuadrada(B, "B")

    if A.shape != B.shape:
      raise ValueError(f"A y B deben tener la misma forma: {A.shape} != {B.shape}")

    n = A.shape[0]
    C = np.empty((n, n), dtype=np.float64)

    getattr(self.lib, funcion)(self._puntero(A), self._puntero(B),
                               self._puntero(C), n)
    return C

  # Todas las entradas se convierten a NumPy float64 y se pasan como  apuntadores a la biblioteca compartida mediante ctypes.
  # Multiplicación de matrices cuadradas: C = A * B.
  def mul_mat(self, A, B):
    return self._operar_matrices(A, B, "t8_mul_mat")

  # Suma elemento a elemento: C = A + B.
  def suma_matrices(self, A, B):
    return self._operar_matrices(A, B, "t8_suma_matrices")

  # Resta elemento a elemento: C = A - B.
  def resta_matrices(self, A, B):
    return self._operar_matrices(A, B, "t8_resta_matrices")

  # Producto punto de dos vectores.
  def producto_punto(self, u, v):
    u = self._vector(u, "u")
    v = self._vector(v, "v")

    if u.size != v.size:
      raise ValueError(f"u y v deben tener el mismo tamaño: {u.size} != {v.size}")

    return float(self.lib.t8_producto_punto(self._puntero(u),
                                           self._puntero(v), u.size))

# Tiempo promedio de ejecutar x funcion varias veces.
def _tiempo(funcion, repeticiones=5):
  inicio = perf_counter()
  for _ in range(repeticiones):
    funcion()
  return (perf_counter() - inicio) / repeticiones


def demo(n=256):
  rng = np.random.default_rng(1234)

  A = rng.standard_normal((n, n))
  B = rng.standard_normal((n, n))

  t8 = Tarea8()

  print(f"Biblioteca: {t8.ruta_lib}")
  print(f"Tamaño: {n} x {n}\n")

  C = t8.mul_mat(A, B)
  print(f"mul_mat         : {np.allclose(C, A @ B, rtol=1e-9, atol=1e-9)}")
  print(f"suma_matrices   : {np.allclose(t8.suma_matrices(A, B), A + B)}")
  print(f"resta_matrices  : {np.allclose(t8.resta_matrices(A, B), A - B)}")

  u = rng.standard_normal(n * n)
  v = rng.standard_normal(n * n)
  punto = t8.producto_punto(u, v)
  print(f"producto_punto  : {np.isclose(punto, np.dot(u, v), rtol=1e-9, atol=1e-9)}")

  print("\nTiempos promedio (5 repeticiones):")
  print(f"mul_mat         : {_tiempo(lambda: t8.mul_mat(A, B)):.6f} s")
  print(f"suma_matrices   : {_tiempo(lambda: t8.suma_matrices(A, B)):.6f} s")
  print(f"resta_matrices  : {_tiempo(lambda: t8.resta_matrices(A, B)):.6f} s")
  print(f"producto_punto  : {_tiempo(lambda: t8.producto_punto(u, v)):.6f} s")


if __name__ == "__main__":
  tamano = int(sys.argv[1]) if len(sys.argv) > 1 else 256
  demo(tamano)
