#!/usr/bin/env python3
"""
Tarea 6 - Codificador Huffman con formato de archivo propio.

Uso:
    python3 tarea6_codificador.py <archivo_original> <archivo_comprimido>

Ejemplo:
    python3 tarea6_codificador.py origen.txt origen.huff

--------------------------------------------------------------------------
FORMATO DEL ARCHIVO .huff
--------------------------------------------------------------------------
El archivo tiene tres partes:

    +----------------+----------------+----------------+
    | HEADER (35 B)  | ARBOL (N B)    | PAYLOAD (M B)  |
    +----------------+----------------+----------------+

El header se escribe con struct.pack(">8sBBQQIIB", ...).  El ">" indica
big-endian (el byte más significativo primero), que es lo habitual en
formatos de archivo porque no depende de la máquina.

Offset  Bytes  Campo            Descripción
------  -----  ---------------  -----------------------------------------
0       8      magic            b"HUFF6PY\\0", identifica el formato
8       1      version          1
9       1      flags            0 (reservado para el futuro)
10      8      original_size    tamaño del archivo original en bytes
18      8      payload_size     tamaño de los bits comprimidos en bytes
26      4      tree_size        tamaño del árbol serializado en bytes
30      4      crc32            CRC-32 del archivo original
34      1      padding          bits de relleno agregados al final (0-7)
35      ...    árbol            recorrido en preorden
...     ...    payload          códigos Huffman empaquetados en bytes

El árbol se guarda en preorden:
    0                 -> nodo interno
    1 + símbolo (1 B) -> hoja

Ejemplo de árbol:
        (interno)
        /       \\
      'a'       (interno)
                /       \\
              'b'       'c'

se serializa como:
    0 1 'a' 0 1 'b' 1 'c'

Se trabaja con bytes (0-255) en lugar de caracteres para que el formato
sirva también para imágenes, PDFs, etc.  Para texto ASCII el resultado es
equivalente al enfoque por caracteres; para UTF-8 se comprimen los bytes
de cada carácter y la descompresión devuelve el archivo idéntico.
"""

import heapq
import itertools
import struct
import sys
import zlib
from collections import Counter

# -------------------------------------------------------------------------
# Constantes del formato
# -------------------------------------------------------------------------
MAGIC = b"HUFF6PY\x00"          # 8 bytes exactos
VERSION = 1
FLAGS = 0
HEADER_FORMAT = ">8sBBQQIIB"    # ver tabla del docstring
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 35 bytes


class NodoHuffman:
  """Nodo del árbol de Huffman.

  Si `simbolo` es None es un nodo interno; si es un entero 0-255 es una
  hoja que representa un byte del archivo original.
  """

  # Contador global para desempatar nodos con la misma frecuencia.
  # Sin esto, el orden podría variar entre ejecuciones y el archivo
  # comprimido no sería reproducible.
  _orden = itertools.count()

  def __init__(self, simbolo, frecuencia=0):
    self.simbolo = simbolo
    self.frecuencia = frecuencia
    self.izquierda = None
    self.derecha = None
    self.orden = next(NodoHuffman._orden)

  def __lt__(self, otro):
    # heapq ordena de menor a mayor.  Se compara primero la frecuencia y,
    # si hay empate, el orden de creación para que el resultado sea estable.
    return (self.frecuencia, self.orden) < (otro.frecuencia, otro.orden)


def construir_arbol(frecuencias):
  """Construye el árbol de Huffman a partir de un diccionario byte->frecuencia."""
  if not frecuencias:
    return None

  heap = [NodoHuffman(simbolo, freq) for simbolo, freq in frecuencias.items()]
  heapq.heapify(heap)

  while len(heap) > 1:
    nodo1 = heapq.heappop(heap)  # el de menor frecuencia
    nodo2 = heapq.heappop(heap)  # el siguiente

    # Se crea un padre sin símbolo cuya frecuencia es la suma de los hijos.
    padre = NodoHuffman(None, nodo1.frecuencia + nodo2.frecuencia)
    padre.izquierda = nodo1
    padre.derecha = nodo2
    heapq.heappush(heap, padre)

  return heap[0]


def generar_codigos(raiz):
  """Devuelve {byte: 'cadena_de_bits'} recorriendo el árbol.

  Izquierda = '0', derecha = '1'.  Si el archivo tiene un solo símbolo
  distinto, su código queda vacío (no hace falta escribir ningún bit).
  """
  codigos = {}

  def recorrer(nodo, prefijo):
    if nodo is None:
      return
    if nodo.simbolo is not None:
      codigos[nodo.simbolo] = prefijo
      return
    recorrer(nodo.izquierda, prefijo + "0")
    recorrer(nodo.derecha, prefijo + "1")

  recorrer(raiz, "")
  return codigos


def serializar_arbol(raiz):
  """Convierte el árbol a bytes en preorden (ver formato en el docstring)."""
  datos = bytearray()

  def recorrer(nodo):
    if nodo.simbolo is not None:
      datos.append(1)              # marca de hoja
      datos.append(nodo.simbolo)   # byte que representa
    else:
      datos.append(0)              # marca de nodo interno
      recorrer(nodo.izquierda)
      recorrer(nodo.derecha)

  if raiz is not None:
    recorrer(raiz)

  return bytes(datos)


def empaquetar_bits(bits):
  """Convierte una cadena de '0'/'1' a bytes.

  Devuelve (payload, padding):
    payload -> bytes listos para escribir
    padding -> cuántos ceros se agregaron al final para completar el
               último byte (0 a 7).  El decodificador los elimina.

  Ejemplo: "101" -> padding=5, payload=b"\\xA0" (10100000).
  """
  if not bits:
    return b"", 0

  padding = (-len(bits)) % 8   # 0 si ya es múltiplo de 8; si no, 8-(n%8)
  bits_relleno = bits + "0" * padding

  payload = bytearray()
  for i in range(0, len(bits_relleno), 8):
    payload.append(int(bits_relleno[i:i + 8], 2))

  return bytes(payload), padding


def escribir_archivo(ruta_entrada, ruta_salida):
  """Lee el archivo original, lo comprime y escribe header + árbol + payload."""
  with open(ruta_entrada, "rb") as f:
    datos = f.read()

  frecuencias = Counter(datos)
  raiz = construir_arbol(frecuencias)
  codigos = generar_codigos(raiz)

  # Si hay un solo símbolo distinto, codigos[símbolo] == "".
  # En ese caso el payload queda vacío y el tamaño original le dice al
  # decodificador cuántas veces debe repetir ese símbolo.
  if len(codigos) <= 1:
    bits = ""
  else:
    bits = "".join(codigos[byte] for byte in datos)

  payload, padding = empaquetar_bits(bits)
  arbol = serializar_arbol(raiz)

  crc = zlib.crc32(datos) & 0xFFFFFFFF

  header = struct.pack(
      HEADER_FORMAT,
      MAGIC,
      VERSION,
      FLAGS,
      len(datos),      # Q: entero sin signo de 8 bytes
      len(payload),    # Q
      len(arbol),      # I: entero sin signo de 4 bytes
      crc,             # I
      padding,         # B: 1 byte
  )

  with open(ruta_salida, "wb") as f:
    f.write(header)
    f.write(arbol)
    f.write(payload)

  return {
      "original": len(datos),
      "comprimido": HEADER_SIZE + len(arbol) + len(payload),
      "header": HEADER_SIZE,
      "arbol": len(arbol),
      "payload": len(payload),
      "padding": padding,
      "simbolos": len(codigos),
  }


def main():
  if len(sys.argv) != 3:
    print(f"Uso: python3 {sys.argv[0]} <archivo_original> <archivo_comprimido>")
    print(f"Ejemplo: python3 {sys.argv[0]} origen.txt origen.huff")
    return 1

  ruta_entrada = sys.argv[1]
  ruta_salida = sys.argv[2]

  try:
    info = escribir_archivo(ruta_entrada, ruta_salida)
  except OSError as e:
    print(f"Error al leer/escribir archivos: {e}", file=sys.stderr)
    return 1

  print("Archivo comprimido correctamente.")
  print(f"  Archivo original : {ruta_entrada} ({info['original']} bytes)")
  print(f"  Archivo generado : {ruta_salida} ({info['comprimido']} bytes)")
  print(f"  Header           : {info['header']} bytes")
  print(f"  Árbol            : {info['arbol']} bytes")
  print(f"  Payload          : {info['payload']} bytes (padding: {info['padding']} bits)")
  print(f"  Símbolos distintos: {info['simbolos']}")

  if info["original"] > 0:
    porcentaje = 100.0 * info["comprimido"] / info["original"]
    print(f"  Tamaño relativo  : {porcentaje:.2f}% del original")

  return 0


if __name__ == "__main__":
  sys.exit(main())
