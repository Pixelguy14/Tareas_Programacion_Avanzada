#!/usr/bin/env bash
# Compila la Tarea 8:
#   - libtarea8.so : biblioteca para cargar desde Python con ctypes
#   - Tarea8.bin   : demo independiente en C++
set -euo pipefail

cd "$(dirname "$0")"

CXX="${CXX:-g++}"
FLAGS=(-O3 -std=c++17 -mavx2 -mfma -fopenmp -Wall -Wextra)
FUENTES=(Tarea8.cpp mulMat.cpp productoPunto.cpp sumaMatrices.cpp)

"$CXX" "${FLAGS[@]}" -fPIC -shared "${FUENTES[@]}" -o libtarea8.so
"$CXX" "${FLAGS[@]}" -DTAREA8_DEMO "${FUENTES[@]}" -o Tarea8.bin

echo "Biblioteca generada: $(pwd)/libtarea8.so"
echo "Demo generado:       $(pwd)/Tarea8.bin"
