# cython: language_level=3
# cython: boundscheck=False
# cython: wraparound=False
# cython: cdivision=True
"""
Tarea 7 - Cython Huffman.

Este archivo produce un módulo nativo `huffman_cython` que se importa desde Python:

    from huffman_cython import CompresorHuffman

1. NodoHuffman es una `cdef class`: sus atributos viven en una estructura
   C, no en un diccionario de Python.
2. El empaquetado de bits se hace directamente con operaciones de enteros
   y un `bytearray`, en lugar de crear una cadena gigante de "0"/"1" y
   convertir cada byte con int(byte, 2).
3. La descompresión lee los bits directamente de los bytes con
   desplazamientos y máscaras, en lugar de construir otra cadena de bits.
"""

import heapq
from collections import Counter


cdef class NodoHuffman:
    """Nodo del árbol de Huffman.

    `caracter` es None para nodos internos o el carácter para las hojas.
    `freq` se declara como long para que la comparación del heap sea correcta.
    """

    cdef public object caracter
    cdef public long freq
    cdef public NodoHuffman left
    cdef public NodoHuffman right

    def __cinit__(self, caracter, long freq):
        self.caracter = caracter
        self.freq = freq
        self.left = None
        self.right = None

    def __lt__(self, other):
        # heapq necesita esta comparación.  El cast le dice a Cython que
        # `other` también es un NodoHuffman, para acceder a .freq sin
        # pasar por el diccionario de Python.
        return self.freq < (<NodoHuffman>other).freq


cdef class CompresorHuffman:
    """Compresor Huffman compilado con Cython."""

    cdef public str ruta_archivo
    cdef public list heap
    cdef public dict codigos
    cdef public dict inverso_codigos
    cdef dict codigos_int

    def __cinit__(self, str ruta_archivo):
        self.ruta_archivo = ruta_archivo
        self.heap = []
        self.codigos = {}
        self.inverso_codigos = {}
        self.codigos_int = {}

    def _calcular_frecuencias(self, texto):
        return Counter(texto)

    def _construir_arbol(self, frecuencias):
        cdef NodoHuffman nodo
        cdef NodoHuffman nodo1
        cdef NodoHuffman nodo2
        cdef NodoHuffman combinado

        # Se reinicia el heap para que el objeto se pueda reutilizar.
        self.heap = []

        for caracter, freq in frecuencias.items():
            nodo = NodoHuffman(caracter, freq)
            heapq.heappush(self.heap, nodo)

        while len(self.heap) > 1:
            nodo1 = <NodoHuffman>heapq.heappop(self.heap)
            nodo2 = <NodoHuffman>heapq.heappop(self.heap)

            combinado = NodoHuffman(None, nodo1.freq + nodo2.freq)
            combinado.left = nodo1
            combinado.right = nodo2

            heapq.heappush(self.heap, combinado)

        return self.heap[0] if self.heap else None

    def _generar_codigos_rec(self, NodoHuffman nodo, str codigo_actual):
        if nodo is None:
            return

        if nodo.caracter is not None:
            self.codigos[nodo.caracter] = codigo_actual
            self.inverso_codigos[codigo_actual] = nodo.caracter
            return

        self._generar_codigos_rec(nodo.left, codigo_actual + "0")
        self._generar_codigos_rec(nodo.right, codigo_actual + "1")

    cdef void _preparar_codigos_int(self):
        """Convierte cada código de texto a (valor entero, longitud).

        Así el empaquetado no necesita interpretar cadenas de bits.
        """
        cdef object caracter
        cdef object codigo

        self.codigos_int.clear()
        for caracter, codigo in self.codigos.items():
            if codigo:
                self.codigos_int[caracter] = (int(codigo, 2), len(codigo))
            else:
                # Caso de un solo símbolo distinto: no se requiere ningún bit.
                self.codigos_int[caracter] = (0, 0)

    cdef tuple _codificar(self, str texto):
        """Codifica el texto y devuelve (bytes, padding)."""
        cdef Py_ssize_t i
        cdef Py_ssize_t n = len(texto)
        cdef long long total_bits = 0
        cdef object buffer = 0
        cdef int nbits = 0
        cdef int padding
        cdef object codigo
        cdef int longitud
        cdef dict codigos_int = self.codigos_int
        cdef bytearray salida = bytearray()

        # Primera pasada: cuántos bits ocupará el texto codificado.
        for i in range(n):
            codigo, longitud = codigos_int[texto[i]]
            total_bits += longitud

        # Igual que huffman.py: 8 - (bits % 8), que da 1..8.
        # Si el número de bits ya es múltiplo de 8, se agrega un byte
        # completo de relleno (8), no cero.
        padding = 8 - (total_bits % 8)

        # El primer byte del archivo es el contador de padding.
        buffer = padding
        nbits = 8

        # Segunda pasada: escribir los códigos bit a bit.
        for i in range(n):
            codigo, longitud = codigos_int[texto[i]]

            # Se agrega el código al buffer y se van vaciando los bytes
            # completos.  `buffer` es un entero de Python, así que no hay
            # problema si un código mide más de 64 bits.
            buffer = (buffer << longitud) | codigo
            nbits += longitud

            while nbits >= 8:
                nbits -= 8
                salida.append((buffer >> nbits) & 0xFF)

            # Conservar sólo los bits que aún no se han escrito.
            if nbits:
                buffer &= (1 << nbits) - 1
            else:
                buffer = 0

        # Vaciar el byte de padding si el texto estaba vacío.
        while nbits >= 8:
            nbits -= 8
            salida.append((buffer >> nbits) & 0xFF)
            buffer &= (1 << nbits) - 1 if nbits else 0

        # Agregar los ceros de relleno del último byte.
        if nbits:
            salida.append((buffer << (8 - nbits)) & 0xFF)
        else:
            salida.append(0)

        return bytes(salida), padding

    def comprimir(self, ruta_salida):
        cdef str texto
        cdef object frecuencias
        cdef object raiz
        cdef bytes bytes_salida
        cdef int padding

        with open(self.ruta_archivo, "r", encoding="utf-8") as f:
            texto = f.read()

        frecuencias = Counter(texto)
        raiz = self._construir_arbol(frecuencias)

        self.codigos = {}
        self.inverso_codigos = {}
        self._generar_codigos_rec(raiz, "")
        self._preparar_codigos_int()

        bytes_salida, padding = self._codificar(texto)

        with open(ruta_salida, "wb") as salida:
            salida.write(bytes_salida)

        print(f"Archivo comprimido exitosamente en: {ruta_salida}")
        return self.codigos, raiz

    def descomprimir(self, ruta_comprimido, raiz_arbol, ruta_salida):
        cdef bytes datos
        cdef const unsigned char[:] vista
        cdef int padding
        cdef long long total_bits
        cdef long long i
        cdef Py_ssize_t indice_byte
        cdef NodoHuffman nodo
        cdef NodoHuffman raiz = <NodoHuffman>raiz_arbol
        cdef list salida = []
        cdef str resultado

        with open(ruta_comprimido, "rb") as f:
            datos = f.read()

        if not datos:
            raise ValueError("El archivo comprimido está vacío.")

        padding = datos[0]
        if padding < 1 or padding > 8:
            raise ValueError(f"Padding inválido: {padding}")

        # El primer byte es el contador de padding; los demás contienen
        # los códigos.  Se descartan los últimos `padding` bits.
        total_bits = (len(datos) - 1) * 8 - padding
        if total_bits < 0:
            raise ValueError("El archivo comprimido está truncado.")

        if raiz is None:
            resultado = ""
        else:
            vista = datos
            nodo = raiz

            for i in range(total_bits):
                # Byte donde está el bit i (se salta el byte de padding).
                indice_byte = 1 + (i >> 3)

                # (i & 7) da la posición dentro del byte, de 0 a 7.
                # Se lee de izquierda a derecha: el bit más significativo
                # primero, igual que se escribió.
                if (vista[indice_byte] >> (7 - (i & 7))) & 1:
                    nodo = nodo.right
                else:
                    nodo = nodo.left

                if nodo is None:
                    raise ValueError("Secuencia de bits inválida.")

                if nodo.caracter is not None:
                    salida.append(nodo.caracter)
                    nodo = raiz

            resultado = "".join(salida)

        with open(ruta_salida, "w", encoding="utf-8") as salida_texto:
            salida_texto.write(resultado)

        print(f"Archivo descomprimido exitosamente en: {ruta_salida}")
        return resultado
