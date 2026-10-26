#!/usr/bin/env python3
"""
Tarea 6 - Decodificador Huffman para el formato .huff de la Tarea 6.

python3 tarea6_decodificador.py <archivo_comprimido> <archivo_salida>

python3 tarea6_decodificador.py origen.huff origen_recuperado.txt

El decodificador lee el header, reconstruye el árbol que el codificador guardó 
dentro del archivo y usa ese árbol para recorrer los bits del payload.  
No necesita el archivo original ni una tabla de frecuencias externa.

Header:
    magic, version, flags, original_size, payload_size,
    tree_size, crc32, padding

Árbol en preorden:
    0                 -> nodo interno
    1 + símbolo (1 B) -> hoja
"""

import struct
import sys
import zlib

# Deben coincidir exactamente con el codificador.
MAGIC = b"HUFF6PY\x00"
VERSION = 1
HEADER_FORMAT = ">8sBBQQIIB"
HEADER_SIZE = struct.calcsize(HEADER_FORMAT)  # 35 bytes


class NodoHuffman:
  """Nodo reconstruido desde el archivo comprimido."""

  def __init__(self, simbolo=None):
    self.simbolo = simbolo
    self.izquierda = None
    self.derecha = None


def deserializar_arbol(datos):
  """Reconstruye el árbol a partir del recorrido en preorden.

  El formato es recursivo:
    - Si el byte es 1, sigue un byte de símbolo y se crea una hoja.
    - Si el byte es 0, se crea un nodo interno y se leen primero su
      subárbol izquierdo y después el derecho.
  """
  if not datos:
    return None

  posicion = 0

  def recorrer():
    nonlocal posicion

    if posicion >= len(datos):
      raise ValueError("El árbol termina inesperadamente.")

    tipo = datos[posicion]
    posicion += 1

    if tipo == 1:
      if posicion >= len(datos):
        raise ValueError("Falta el símbolo de una hoja del árbol.")
      simbolo = datos[posicion]
      posicion += 1
      return NodoHuffman(simbolo)

    if tipo == 0:
      nodo = NodoHuffman(None)
      nodo.izquierda = recorrer()
      nodo.derecha = recorrer()
      return nodo

    raise ValueError(f"Tipo de nodo desconocido: {tipo}")

  raiz = recorrer()

  if posicion != len(datos):
    raise ValueError("Sobran bytes al final del árbol serializado.")

  return raiz


def leer_header(blob):
  """Valida el header y devuelve sus campos desempaquetados."""
  if len(blob) < HEADER_SIZE:
    raise ValueError(
        f"El archivo es demasiado pequeño: {len(blob)} bytes, "
        f"se esperaban al menos {HEADER_SIZE}."
    )

  campos = struct.unpack(HEADER_FORMAT, blob[:HEADER_SIZE])
  (magic, version, flags, original_size,
   payload_size, tree_size, crc, padding) = campos

  if magic != MAGIC:
    raise ValueError(
        "El archivo no tiene el formato esperado (magic incorrecto). "
        f"Se encontró: {magic!r}"
    )

  if version != VERSION:
    raise ValueError(f"Versión no soportada: {version}")

  if flags != 0:
    raise ValueError(f"Flags no soportados: {flags}")

  if padding > 7:
    raise ValueError(f"Padding inválido: {padding}")

  esperado = HEADER_SIZE + tree_size + payload_size
  if len(blob) != esperado:
    raise ValueError(
        f"Tamaño inconsistente: el header dice {esperado} bytes "
        f"(header + árbol + payload), pero el archivo tiene {len(blob)}."
    )

  arbol = blob[HEADER_SIZE:HEADER_SIZE + tree_size]
  payload = blob[HEADER_SIZE + tree_size:]

  return original_size, payload_size, tree_size, crc, padding, arbol, payload


def decodificar_payload(payload, raiz, original_size, padding):
  """Recorre los bits del payload usando el árbol y devuelve los bytes."""
  if original_size == 0:
    return b""

  if raiz is None:
    raise ValueError("El archivo dice tener datos, pero no tiene árbol.")

  # Caso especial: un solo símbolo distinto.  El codificador no escribe
  # bits porque todos los bytes son iguales; el tamaño original indica
  # cuántas veces hay que repetirlo.
  if raiz.izquierda is None and raiz.derecha is None:
    if raiz.simbolo is None:
      raise ValueError("Árbol inválido: hoja sin símbolo.")
    return bytes([raiz.simbolo]) * original_size

  # Convertir cada byte a 8 caracteres '0'/'1'.
  bits = "".join(f"{byte:08b}" for byte in payload)

  # Quitar los ceros que se agregaron para completar el último byte.
  if padding:
    if padding > len(bits):
      raise ValueError("El padding es mayor que los bits disponibles.")
    bits = bits[:-padding]

  salida = bytearray()
  nodo = raiz

  for bit in bits:
    nodo = nodo.izquierda if bit == "0" else nodo.derecha

    if nodo is None:
      raise ValueError("Secuencia de bits inválida: se salió del árbol.")

    if nodo.simbolo is not None:
      salida.append(nodo.simbolo)
      nodo = raiz

      if len(salida) == original_size:
        break

  if len(salida) != original_size:
    raise ValueError(
        f"Faltan datos: se decodificaron {len(salida)} de "
        f"{original_size} bytes."
    )

  return bytes(salida)


def leer_archivo(ruta_entrada, ruta_salida):
  """Lee el .huff, reconstruye el original y lo escribe en disco."""
  with open(ruta_entrada, "rb") as f:
    blob = f.read()

  (original_size, payload_size, tree_size,
   crc, padding, arbol_bytes, payload) = leer_header(blob)

  raiz = deserializar_arbol(arbol_bytes)
  datos = decodificar_payload(payload, raiz, original_size, padding)

  crc_calculado = zlib.crc32(datos) & 0xFFFFFFFF
  if crc_calculado != crc:
    raise ValueError(
        "El CRC-32 no coincide: el archivo está corrupto o fue alterado. "
        f"Esperado: 0x{crc:08X}, calculado: 0x{crc_calculado:08X}"
    )

  with open(ruta_salida, "wb") as f:
    f.write(datos)

  return {
      "original": original_size,
      "comprimido": len(blob),
      "header": HEADER_SIZE,
      "arbol": tree_size,
      "payload": payload_size,
      "padding": padding,
  }


def main():
  if len(sys.argv) != 3:
    print(f"Uso: python3 {sys.argv[0]} <archivo_comprimido> <archivo_salida>")
    print(f"Ejemplo: python3 {sys.argv[0]} origen.huff origen_recuperado.txt")
    return 1

  ruta_entrada = sys.argv[1]
  ruta_salida = sys.argv[2]

  try:
    info = leer_archivo(ruta_entrada, ruta_salida)
  except OSError as e:
    print(f"Error al leer/escribir archivos: {e}", file=sys.stderr)
    return 1
  except ValueError as e:
    print(f"Archivo inválido: {e}", file=sys.stderr)
    return 1

  print("Archivo descomprimido correctamente.")
  print(f"  Archivo comprimido : {ruta_entrada} ({info['comprimido']} bytes)")
  print(f"  Archivo generado   : {ruta_salida} ({info['original']} bytes)")
  print(f"  Header             : {info['header']} bytes")
  print(f"  Árbol              : {info['arbol']} bytes")
  print(f"  Payload            : {info['payload']} bytes (padding: {info['padding']} bits)")

  return 0


if __name__ == "__main__":
  sys.exit(main())
