/*
 * Tarea 5: Algoritmo genético para minimizar la función de Rastrigin
 *
 *   f(x, y) = 20 + (x^2 - 10 cos(2*pi*x)) + (y^2 - 10 cos(2*pi*y))
 *   x, y en [-5.12, 5.12]
 *
 * Cada variable se representa con 32 bits. El genoma de un individuo son
 * dos enteros sin signo (x e y); los bits se usan directamente y se decodifican
 * con:
 *
 *   valor_real = A + (valor_entero / (2^32 - 1)) * (B - A)
 *
 * Compilar:
 *   gcc -O2 -march=native Tarea5.c -o Tarea5.bin -lm
 *
 * Ejecutar:
 *   ./Tarea5.bin
 *   ./Tarea5.bin 12345 # semilla para reproducir una ejecucion en especifico
 *
 * -------------------------------------------------------------------------
 * Repaso rápido de los operadores de bits que se usan en este programa:
 *
 *   a & b   AND bit a bit: deja un 1 sólo donde AMBOS tienen 1.
 *                         Sirve para "filtrar" o conservar bits con una máscara.
 *   a | b   OR  bit a bit: deja un 1 donde CUALQUIERA tiene 1.
 *                         Sirve para "pegar" o unir dos partes.
 *   a ^ b   XOR bit a bit: deja 1 donde los bits SON DIFERENTES.
 *                         Sirve para invertir bits (si b tiene un 1) o mezclar.
 *   ~a      NOT bit a bit: invierte todos los bits (0 -> 1, 1 -> 0).
 *   a << n  desplaza los bits n posiciones a la izquierda (llena con 0).
 *   a >> n  desplaza los bits n posiciones a la derecha (llena con 0).
 *
 * Ejemplo con 8 bits:   a = 10101100
 *   a & 00001111 = 00001100   (conserva los 4 bits bajos)
 *   a | 00000011 = 10101111   (enciende los 2 bits bajos)
 *   a ^ 00001000 = 10100100   (invierte el bit 3)
 *   ~a           = 01010011   (invierte todos)
 *   a << 2       = 10110000   (recorre a la izquierda)
 *   a >> 2       = 00101011   (recorre a la derecha)
 * -------------------------------------------------------------------------
 */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Parámetros del algoritmo genético
#define POBLACION      50
#define GENERACIONES   500     // iteraciones del ciclo evolutivo
#define PROB_CRUZA     0.90    // probabilidad de cruzar una pareja
#define PROB_MUTACION  0.10    // probabilidad de mutar cada bit
#define BITS           32
#define LIMITE         5.12    // x, y pertenecen a [-LIMITE, LIMITE]
#define PI             3.14159265358979323846

/* Individuo
 *
 *   xy[0] -> genoma de x
 *   xy[1] -> genoma de y
 *
 * Guardamos también x, y y fx ya decodificados/evaluados para no repetir
 * trabajo al ordenar o imprimir.  La parte importante del genoma son los
 * 32 bits de cada entero; x e y son sólo su interpretación real.
 */
typedef struct {
    uint32_t xy[2];
    double   x;
    double   y;
    double   fx;
} Individuo;

/* Generador pseudoaleatorio xorshift64
 *
 * rand() de C sólo garantiza 15 bits (RAND_MAX suele ser 2^31-1), así que
 * no alcanza para llenar 32 bits correctamente.  
 * xorshift64 produce 64 bits; tomamos los 32 altos.
 */
static uint64_t estado_rng;

static uint32_t rng_u32(void)
{
    /* Cada línea mezcla los 64 bits del estado consigo mismos desplazados.
     * "^=" significa "estado_rng = estado_rng ^ (...)".
     * Al hacer XOR con una copia recorrida, los bits se combinan entre sí:
     * es la parte de "shift" de xorshift.  Los desplazamientos 12, 25 y 27
     * son los recomendados por George Marsaglia para este generador.
     */
    estado_rng ^= estado_rng >> 12;  // mezcla la parte alta hacia la baja
    estado_rng ^= estado_rng << 25;  // mezcla la parte baja hacia la alta
    estado_rng ^= estado_rng >> 27;  // vuelve a mezclar

    /* Multiplicar por 2685821657736338717ULL es el "scramble" final de la
     * variante xorshift64*: cambia los bits para que la salida no muestre
     * patrones.  ULL significa unsigned long long (entero de 64 bits).
     *
     * >> 32 toma los 32 bits ALTOS del producto de 64 bits y descarta los
     * 32 bajos; (uint32_t) los guarda en un entero sin signo de 32 bits.
     * Así podemos llenar xy[0] y xy[1] sin quedarnos con sólo 31 bits,
     * que es lo que pasaría con rand().
     */
    return (uint32_t)((estado_rng * 2685821657736338717ULL) >> 32);
}

// Número real uniforme en rango [0, 1)
static double rng_double(void)
{
    /* rng_u32() produce un entero entre 0 y 2^32-1.  Al dividirlo entre
     * 2^32 = 4294967296.0 obtenemos un valor en [0, 1):
     * el máximo posible es (2^32-1)/2^32, que es menor que 1.
     * Es importante dividir entre 2^32 y no entre 2^32-1 si queremos que
     * el resultado nunca llegue exactamente a 1.
     */
    return (double)rng_u32() / 4294967296.0;
}

/* Decodificación de un gen (32 bits) al intervalo [A, B]
 *
 *   2^32 - 1 = 4294967295  (UINT32_MAX)
 */
static double decodificar(uint32_t gen)
{
    const double A = -LIMITE;
    const double B =  LIMITE;

    /* gen es un entero de 32 bits entre 0 y UINT32_MAX = 2^32-1.
     * (double)gen / (double)UINT32_MAX convierte ese entero a un valor
     * real en [0, 1]: 0 si gen=0 y 1 si gen=2^32-1.
     * Multiplicar por (B - A) estira ese [0,1] al ancho del intervalo
     * (10.24 en este caso) y sumar A lo traslada para que quede en [A, B].
     *
     * Ejemplo: gen = 0          -> A + 0*(B-A) = -5.12
     *          gen = UINT32_MAX -> A + 1*(B-A) = +5.12
     *          gen = 2^31       -> aproximadamente 0
     */
    return A + ((double)gen / (double)UINT32_MAX) * (B - A);
}

// Función de Rastrigin para dos variables.  El mínimo global es 0 en (0,0)
static double rastrigin(double x, double y)
{
    return 20.0
         + (x * x - 10.0 * cos(2.0 * PI * x))
         + (y * y - 10.0 * cos(2.0 * PI * y));
}

// Decodifica el genoma y evalúa la función.
static void evaluar(Individuo *ind)
{
    ind->x  = decodificar(ind->xy[0]);
    ind->y  = decodificar(ind->xy[1]);
    ind->fx = rastrigin(ind->x, ind->y);
}

// Inicialización: 32 bits aleatorios para x y para y.
static void inicializar_poblacion(Individuo *pob, int n)
{
    for (int i = 0; i < n; i++) {
        pob[i].xy[0] = rng_u32();
        pob[i].xy[1] = rng_u32();
        evaluar(&pob[i]);
    }
}

/* Cruza de un punto sobre cada variable.
 *
 * Se elige un punto de corte entre [1 y 31] se construye una máscara 
 * con los bits bajos y se combinan los padres:
 *
 *   hijo1 = (padre1 & mascara) | (padre2 & ~mascara)
 *   hijo2 = (padre2 & mascara) | (padre1 & ~mascara)
 */
static void cruzar(const Individuo *p1, const Individuo *p2,
                   Individuo *h1, Individuo *h2)
{
    /* rng_u32() % (BITS-1) da un valor en [0, 30] y al sumar 1 queda en
     * [1, 31].  No permitimos 0 ni 32 porque en esos casos la máscara sería
     * 0 o todos unos: un hijo sería copia exacta de un padre y el cruce no
     * mezclaría nada.
     */
    int punto = 1 + (int)(rng_u32() % (BITS - 1)); /* 1 .. 31 */

    /* "1u" es el número 1 como unsigned int (32 bits).
     * "1u << punto" crea un número con un solo 1 en la posición "punto"
     * y ceros a su derecha.  Al restarle 1, todos los bits menores que
     * "punto" se encienden.
     *
     * Ejemplo con punto = 3:
     *   1u << 3  = 00001000
     *   - 1      = 00000111  <- máscara de los 3 bits bajos
     */
    uint32_t mascara = (1u << punto) - 1u;

    for (int g = 0; g < 2; g++) {
        /* & (AND) conserva sólo los bits donde la máscara tiene 1.
         * ~mascara invierte la máscara, así que conserva los bits altos.
         * | (OR) pega las dos partes.  Como una parte usa los bits bajos y
         * la otra los altos, no se empalman: el hijo queda completo.
         *
         * hijo1 = bits bajos de p1 + bits altos de p2
         * hijo2 = bits bajos de p2 + bits altos de p1
         */
        h1->xy[g] = (p1->xy[g] & mascara) | (p2->xy[g] & ~mascara);
        h2->xy[g] = (p2->xy[g] & mascara) | (p1->xy[g] & ~mascara);
    }
}

/* Mutación: cada bit se invierte con probabilidad PROB_MUTACION.
 *
 * Nota: 0.1 por bit es una tasa alta (en promedio 6.4 bits por individuo).
 * Si se observa que la población no converge, bajar este valor (por ejemplo
 * a 0.01) es lo primero que conviene probar.
 */
static void mutar(Individuo *ind)
{
    for (int g = 0; g < 2; g++) {
        for (int b = 0; b < BITS; b++) {
            if (rng_double() < PROB_MUTACION) {
                /* b es la posición del bit: 0 = menos significativo (el de
                 * hasta la derecha), 31 = más significativo (el de la
                 * izquierda).
                 *
                 * "1u << b" crea una máscara con un 1 en la posición b:
                 *   b=0 -> 00000000...0001
                 *   b=3 -> 00000000...1000
                 *
                 * "^=" es XOR con asignación: si el bit b vale 0 lo cambia
                 * a 1, y si vale 1 lo cambia a 0.  Es decir, invierte
                 * exactamente ese bit y deja los demás intactos.
                 *
                 * Ejemplo: 10101100 ^ (1u<<3) = 10100100
                 *                              (el bit 3 pasó de 1 a 0)
                 */
                ind->xy[g] ^= (1u << b);
            }
        }
    }
}

// Comparador para qsort: orden ascendente por aptitud, buscamos el menor f, que es mejor
static int comparar_aptitud(const void *a, const void *b)
{
    const Individuo *ia = (const Individuo *)a;
    const Individuo *ib = (const Individuo *)b;

    /* No se usa (ia->fx - ib->fx) porque el resultado es double y qsort
     * necesita un int: la conversión perdería precisión y, con valores
     * grandes, podría desbordar.
     *
     * En C, una comparación como (a > b) vale 1 si es verdadera y 0 si no.
     * Entonces:
     *   ia->fx < ib->fx  ->  (0) - (1) = -1  (ia va antes)
     *   ia->fx == ib->fx ->  (0) - (0) =  0  (empatados)
     *   ia->fx > ib->fx  ->  (1) - (0) = +1  (ib va antes)
     * qsort espera exactamente eso: negativo, cero o positivo.
     */
    return (ia->fx > ib->fx) - (ia->fx < ib->fx);
}

/* Imprime un gen de 32 bits como cadena binaria, del bit 31 al bit 0
 * (de izquierda a derecha, como se escribe normalmente).
 *
 * Ejemplo: valor = 5 = 00000000 00000000 00000000 00000101
 *          se imprime: 00000000000000000000000000000101
 */
static void imprimir_binario(uint32_t valor)
{
    for (int b = BITS - 1; b >= 0; b--) {
        /* valor >> b desplaza el bit que está en la posición b hasta la
         * posición 0.  Luego "& 1u" se queda únicamente con ese bit:
         * el resultado es 0 o 1.
         *
         * El operador ?: es un if corto:
         *   condición ? valor_si_verdadero : valor_si_falso
         * Así que imprime '1' cuando el bit vale 1 y '0' cuando vale 0.
         * putchar escribe un solo carácter en la salida estándar.
         *
         * El paréntesis extra alrededor de (valor >> b) & 1u deja clara
         * la precedencia: primero se desplaza y luego se enmascara.
         */
        putchar(((valor >> b) & 1u) ? '1' : '0');
    }
}

int main(int argc, char **argv)
{
    /* malloc(cantidad_de_bytes) reserva memoria en el heap y devuelve un
     * puntero al inicio.  sizeof(Individuo) es el tamaño en bytes de una
     * estructura; al multiplicarlo por la cantidad de individuos se reserva
     * el arreglo completo.
     *
     *   poblacion    : los 50 individuos actuales.
     *   descendencia : los 50 hijos de esta generación.
     *   pool         : 100 espacios para juntar padres e hijos y ordenarlos.
     */
    Individuo *poblacion   = malloc(POBLACION * sizeof(Individuo));
    Individuo *descendencia = malloc(POBLACION * sizeof(Individuo));
    Individuo *pool        = malloc(2 * POBLACION * sizeof(Individuo));

    if (poblacion == NULL || descendencia == NULL || pool == NULL) {
        fprintf(stderr, "Error: no se pudo reservar memoria.\n");
        free(poblacion);
        free(descendencia);
        free(pool);
        return 1;
    }

    /* Semilla: la de la línea de comandos o el reloj.  xorshift NO admite 0:
     * si el estado es 0, todos los desplazamientos y XOR dan 0 y el
     * generador se queda atorado para siempre.
     *
     * argc = número de argumentos (incluye el nombre del programa).
     * argv[1] = primer argumento después del nombre, por ejemplo "12345".
     * strtoull lo convierte a unsigned long long en base 10.
     */
    if (argc > 1) {
        estado_rng = strtoull(argv[1], NULL, 10);
    } else {
        /* time(NULL) devuelve la hora actual en segundos (un entero).
         * 0x9E3779B97F4A7C15 es la constante de la razón áurea en 64 bits;
         * se usa como "sal" para que la semilla cambie aunque dos
         * ejecuciones ocurran en el mismo segundo.  ULL indica que el
         * literal es unsigned long long (64 bits).
         *
         * "^" (XOR) combina los bits del tiempo con los de la constante:
         * no es una suma, pero sirve para dispersar la semilla.
         */
        estado_rng = (uint64_t)time(NULL) ^ 0x9E3779B97F4A7C15ULL;
    }

    /* Si el usuario pasa 0 (o si por alguna razón el XOR diera 0), se usa
     * la constante dorada para no arrancar el generador en el estado 0.
     */
    if (estado_rng == 0) {
        estado_rng = 0x9E3779B97F4A7C15ULL;
    }

    inicializar_poblacion(poblacion, POBLACION);
    qsort(poblacion, POBLACION, sizeof(Individuo), comparar_aptitud);

    printf("Rastrigin 2D - GA binario\n");
    printf("Poblacion=%d  Generaciones=%d  Pc=%.2f  Pm=%.2f\n\n",
           POBLACION, GENERACIONES, PROB_CRUZA, PROB_MUTACION);
    printf("%6s %14s %14s %14s\n",
           "Gen", "mejor f", "x", "y");

    int paso_impresion = GENERACIONES / 20;
    if (paso_impresion < 1) {
        paso_impresion = 1;
    }

    for (int gen = 0; gen <= GENERACIONES; gen++) {
        if (gen % paso_impresion == 0 || gen == GENERACIONES) {
            printf("%6d %14.8f %14.8f %14.8f\n",
                   gen, poblacion[0].fx, poblacion[0].x, poblacion[0].y);
        }

        if (gen == GENERACIONES) {
            break;
        }

        /* Generar la descendencia.
         *    Se eligen dos padres al azar; con probabilidad Pc se cruzan y
         *    con 1-Pc se copian.  Después se mutan ambos hijos.
         */
        /* Se generan dos hijos por vuelta: por eso i avanza de 2 en 2.
         * POBLACION debe ser par.
         */
        for (int i = 0; i < POBLACION; i += 2) {
            /* El operador % (módulo) da el residuo de la división.
             * rng_u32() % POBLACION produce un índice entre 0 y
             * POBLACION-1, siempre dentro del arreglo.
             */
            int p1 = (int)(rng_u32() % POBLACION);
            int p2 = (int)(rng_u32() % POBLACION);

            /* Si salieron iguales, se vuelve a tirar el dado para p2.
             * Cruzar un individuo consigo mismo no aporta diversidad.
             * El ciclo termina casi de inmediato porque hay 50 opciones.
             */
            while (p2 == p1) {
                p2 = (int)(rng_u32() % POBLACION);
            }

            if (rng_double() < PROB_CRUZA) {
                cruzar(&poblacion[p1], &poblacion[p2],
                       &descendencia[i], &descendencia[i + 1]);
            } else {
                descendencia[i]     = poblacion[p1];
                descendencia[i + 1] = poblacion[p2];
            }

            mutar(&descendencia[i]);
            mutar(&descendencia[i + 1]);
            evaluar(&descendencia[i]);
            evaluar(&descendencia[i + 1]);
        }

        /* Selección: juntar padres e hijos, ordenar y quedarse con los
         *    POBLACION mejores.  Obteniendo que el mejor nunca empeore
         *    (elitismo implícito).
         */
        /* memcpy(destino, origen, bytes) copia un bloque de memoria.
         * "POBLACION * sizeof(Individuo)" son los bytes que ocupan los 50
         * individuos (cantidad de elementos por tamaño de uno).
         *
         * En la segunda línea, "pool + POBLACION" es aritmética de punteros:
         * avanza POBLACION estructuras, no POBLACION bytes.  Así los padres
         * quedan en pool[0..49] y los hijos en pool[50..99].
         */
        memcpy(pool, poblacion, POBLACION * sizeof(Individuo));
        memcpy(pool + POBLACION, descendencia, POBLACION * sizeof(Individuo));

        /* qsort(arreglo, cuántos, tamaño de cada uno, función comparadora).
         * Ordena el arreglo en el mismo lugar (in-place), de menor a mayor
         * según comparar_aptitud.  Como el menor f es el mejor, los mejores
         * quedan al principio.
         */
        qsort(pool, 2 * POBLACION, sizeof(Individuo), comparar_aptitud);

        /* Se copian de vuelta sólo los primeros POBLACION (los 50 mejores)
         * y se descartan los 50 peores.  La población conserva su tamaño.
         */
        memcpy(poblacion, pool, POBLACION * sizeof(Individuo));
    }

    /* Resultado final.
     *
     * En "0x%08X":
     *   0x    es el prefijo que indica que el número está en hexadecimal.
     *   %X    imprime un entero unsigned int en hexadecimal con letras
     *         mayúsculas (A-F).
     *   08    significa ancho mínimo de 8 caracteres rellenando con ceros
     *         a la izquierda, para que siempre se vean los 32 bits
     *         (8 dígitos hexadecimales = 32 bits).
     */
    printf("\nMejor individuo encontrado:\n");
    printf("  x  = %.10f   genes x = 0x%08X = ", poblacion[0].x, poblacion[0].xy[0]);
    imprimir_binario(poblacion[0].xy[0]);
    printf("\n");
    printf("  y  = %.10f   genes y = 0x%08X = ", poblacion[0].y, poblacion[0].xy[1]);
    imprimir_binario(poblacion[0].xy[1]);
    printf("\n");
    printf("  f(x, y) = %.10f\n", poblacion[0].fx);

    free(poblacion);
    free(descendencia);
    free(pool);
    return 0;
}
