#!/usr/bin/env python3
"""Compila huffman_cython.pyx como extensión nativa.

Uso:
    python3 setup_huffman_cython.py build_ext --inplace

El resultado es un archivo del tipo:
    huffman_cython.cpython-313-x86_64-linux-gnu.so

que se importa simplemente con:
    from huffman_cython import CompresorHuffman
"""

import os
from pathlib import Path

from Cython.Build import cythonize
from setuptools import setup

# Ruta absoluta para que funcione sin importar desde qué directorio se ejecute.
BASE = Path(__file__).resolve().parent

# build_ext --inplace copia el .so al directorio actual.  Nos movemos a py_src
# para que el módulo quede junto a huffman_cython.pyx y lo pueda importar el
# benchmark, aunque el script se ejecute desde la raíz del repositorio.
os.chdir(BASE)

setup(
    name="huffman_cython",
    version="1.0",
    description="Compresor Huffman de la Tarea 7 compilado con Cython",
    ext_modules=cythonize(
        str(BASE / "huffman_cython.pyx"),
        compiler_directives={
            "language_level": 3,
            "boundscheck": False,
            "wraparound": False,
            "cdivision": True,
        },
    ),
    zip_safe=False,
)
