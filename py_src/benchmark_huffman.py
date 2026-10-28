#!/usr/bin/env python3
"""
Tarea 7 - Comparación simple: huffman.py vs huffman_cython.

Antes de ejecutar:
    python3 setup_huffman_cython.py build_ext --inplace

Uso:
    python3 benchmark_huffman.py
"""

import os
import tempfile
import time
from contextlib import redirect_stdout
from io import StringIO
from pathlib import Path

from huffman import CompresorHuffman as Python
from huffman_cython import CompresorHuffman as Cython

REPETICIONES = 5
VECES_TEXTO = 5000  # cuántas veces se repite origen.txt para hacerlo medible


def medir(Clase, archivo_entrada, archivo_comprimido, archivo_salida, texto_original):
  tiempos_compresion = []
  tiempos_descompresion = []

  for _ in range(REPETICIONES):
    compresor = Clase(str(archivo_entrada))

    inicio = time.perf_counter()
    with redirect_stdout(StringIO()):
      _, raiz = compresor.comprimir(str(archivo_comprimido))
    fin_compresion = time.perf_counter()

    with redirect_stdout(StringIO()):
      recuperado = compresor.descomprimir(
          str(archivo_comprimido), raiz, str(archivo_salida)
      )
    fin = time.perf_counter()

    assert recuperado == texto_original, "El roundtrip falló"

    tiempos_compresion.append(fin_compresion - inicio)
    tiempos_descompresion.append(fin - fin_compresion)

  return (
      sum(tiempos_compresion) / REPETICIONES,
      sum(tiempos_descompresion) / REPETICIONES,
      os.path.getsize(archivo_comprimido),
  )


def main():
  ruta_base = Path(__file__).with_name("origen.txt")
  texto = ruta_base.read_text(encoding="utf-8") * VECES_TEXTO

  with tempfile.TemporaryDirectory() as directorio:
    directorio = Path(directorio)
    entrada = directorio / "entrada.txt"
    entrada.write_text(texto, encoding="utf-8")

    print(f"Texto de prueba: {len(texto):,} caracteres")
    print(f"Repeticiones: {REPETICIONES}\n")

    resultados = {}
    for nombre, Clase in (("Python", Python), ("Cython", Cython)):
      resultados[nombre] = medir(
          Clase,
          entrada,
          directorio / f"{nombre}.bin",
          directorio / f"{nombre}.txt",
          texto,
      )

  print(f"{'Implementación':<16} {'Compresión':>12} {'Descompresión':>14} {'Tamaño':>10}")
  print("-" * 56)
  for nombre, (compresion, descompresion, tamano) in resultados.items():
    print(f"{nombre:<16} {compresion:>10.4f} s {descompresion:>12.4f} s {tamano:>10,} B")

  py_comp, py_dec, _ = resultados["Python"]
  cy_comp, cy_dec, _ = resultados["Cython"]

  print("\nAceleración (Python / Cython):")
  print(f"  Compresión:    {py_comp / cy_comp:.2f}x")
  print(f"  Descompresión: {py_dec / cy_dec:.2f}x")


if __name__ == "__main__":
  main()
