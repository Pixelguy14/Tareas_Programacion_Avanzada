/*
 * Tarea:  Cambiar el programa original de clase, en lugar de usar doubles, usar precision simple (floats)
 * para el calculo de horner para el martes y medir correctamente el tiempo de diferencia entre ambos
 */
// g++ -O2 -march=native Tarea3.cpp -o Tarea3.bin
// ./Tarea3.o
#include <iostream>
#include <chrono> // Usamos Chrono para una diferencia mas notable en la medicion del tiempo.
#include <cstdlib>
#include <ctime>
#include <memory>
extern "C"
{
    #include <immintrin.h>
}
using namespace std;

// Función auxiliar para forzar la lectura del resultado
void do_not_optimize(double val) {
    volatile double sink = val;
    (void)sink;
}

double horner(double X, double *coef, long size){
    double ACC=0.0; // Acumulacion
    int i;
    for (i=0;i<size;i++){
        ACC=(ACC+coef[i])*X;
    }
    return ACC;
}

double horner_intrinsic(double X, double *coef, long size){
    double *R,P;
    int i;
    __m256d *ymm0,X256,Y;
    ymm0 = (__m256d*)coef; // ymm0 tendrá a1,a2,a3,a4... al hacerle el cast, ese grupo formara un _m256d con los 4 valores empaquetados, pero siguen siendo los mismos valores, solo en un vector.

    X256 = _mm256_set1_pd(X*X*X*X); // Esta instruccion, al ser 4 valores dobles, inicializa cada segmento de la variable de 256 a x⁴
    Y = _mm256_set1_pd(0.0);

    for (i=0;i<size/4-1;i++){ // los ultimos 4 exponentes no se toman en cuenta por ser los valores que no son multiplicados por x.
        Y = _mm256_add_pd(Y,ymm0[i]); // en una sola instruccion, suma el vector de los 4 a1,a2,a3,a4
        Y = _mm256_mul_pd(Y,X256); // en esta parte, se multiplica por los coeficientes que tambien estan empaquetados, el valor x⁴
        // al finalizar la primera iteracion, el Y será un vector con los valores: [a1x⁴,a2x⁴,a3x⁴,a4x⁴] y así sucesivamente.
        // si fueran flotantes, se tendrian 4 valores
    }
    Y = _mm256_add_pd(Y,ymm0[i]);

    R = (double *)(&Y); // este apuntador nos sirve para manejar los p1,p2,p3 y p4 dentro de y en vector, donde cada uno es un double, ahora lo tomamos como R[0]=p1.

    // Este ultimo calculo es p_1h^4+p_2h^3+p_3h^2+p_4h
    // Se hace el ajuste de potencias dado que el vector Y al salir ya ha multiplicado una de las porencias en el bucle.
    P  = R[3];
    P += R[2]*X;
    P += R[1]*X*X;
    P += R[0]*X*X*X;

    return P;
}

// El objetivo es hacer el mismo horner intrinsics para variables de precision simple
float horner_intrinsic_f(float X, float *coef, long size){
    float *R,P;
    int i;
    __m256 *ymm0,X256,Y; // Cambiamos de __m256d al simple al utilizar floats
    ymm0 = (__m256*)coef; // ymm0 tendrá a1,a2,a3,a4... al hacerle el cast, ese grupo formara un _m256d con los 8 valores empaquetados, siguen siendo los mismos valores, solo en un vector.
    // Cambiamos todos los _pd por _ps debido a que estamos usando precision simple
    X256 = _mm256_set1_ps(X*X*X*X *X*X*X*X); // Esta instruccion, al ser 8 valores float, inicializa cada segmento de la variable de 256 a x⁸
    Y = _mm256_set1_ps(0.0f); // se agrega el sufijo f para evitar conversiones implícitas de tipo durante la compilación
    // nos aseguramos que size sea multiplo de 8 para que concuerde:
    for (i=0;i<size/8-1;i++){ // Los ultimos 8 exponentes no se toman en cuenta por ser los valores que no son multiplicados por x.
        Y = _mm256_add_ps(Y,ymm0[i]); // En una sola instruccion, suma el vector de los 8 a1,a2,a3,a4,a5,a6,a7,a8
        Y = _mm256_mul_ps(Y,X256); // En esta parte, se multiplica por los coeficientes que tambien estan empaquetados, el valor x⁸
        // Al finalizar la primera iteracion, el Y será un vector con los valores: [a1x⁴,a2x⁴,a3x⁴,a4x⁴...,a8x⁸] y así sucesivamente.
        // Al ser flotantes, se tienen 4 valores
    }
    Y = _mm256_add_ps(Y,ymm0[i]);

    R = (float *)(&Y); // Este apuntador nos sirve para manejar los p1,p2,p3 y p4 dentro de y en vector, donde cada uno es un float, ahora lo tomamos como R[0]=p1.

    // Este ultimo calculo es p_1h^8+p_2h^7+p_3h^6+p_4h^5+...
    // Al ser float, en lugar de 4 segmentos de R, se tienen 8
    //  el vector Y al salir del bucle almacena los coeficientes en un orden específico. La potencia x⁸ ya fue multiplicada en el bucle sobre las iteraciones pasadas
    P  = R[7];
    P += R[6]*X;
    P += R[5]*X*X;
    P += R[4]*X*X*X;
    P += R[3]*X*X*X*X;
    P += R[2]*X*X*X*X*X;
    P += R[1]*X*X*X*X*X*X;
    P += R[0]*X*X*X*X*X*X*X;

    return P;
}

int main(){
    const long SIZE = 4000;
    const int NUM_TRIALS = 100000; // Número de repeticiones para promediar/acumular tiempo
    double X = 1.1;
    double R = 0.0;

    srand(time(NULL));

    // Reserva alineada a 32 bytes para AVX
    double *coeficientes  = (double *) _mm_malloc(SIZE * sizeof(double), 32);
    float *coeficientes_f = (float *) _mm_malloc(SIZE*sizeof(float),32);

    // Inicialización de datos para evitar medir punteros vacíos
    for (long i = 0; i < SIZE; i++) {
        coeficientes[i]   = (double)(rand() % 1000) / 1000.0;
        coeficientes_f[i] = (float)(rand() % 1000) / 1000.0f;
    }

    // Warm-up de caché L1/L2
    R = horner(X, coeficientes, SIZE);
    R = horner_intrinsic(X, coeficientes, SIZE);
    R = horner_intrinsic_f(X, coeficientes_f, SIZE);

    // Medición de Horner Clásico
    auto start_classic = chrono::high_resolution_clock::now();
    for(int j = 0; j < NUM_TRIALS; j++){
        R = horner(X, coeficientes, SIZE);
        // para que el bucle no se elimine en la flag -O2, se utiliza la funcion auxiliar:
        do_not_optimize(R);
    }
    auto end_classic = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration_classic = end_classic - start_classic;

    // Medición de Horner Intrinsic double (AVX)
    auto start_intrinsic = chrono::high_resolution_clock::now();
    for(int j = 0; j < NUM_TRIALS; j++){
        R = horner_intrinsic(X, coeficientes, SIZE);
        // para que el bucle no se elimine en la flag -O2, se utiliza la funcion auxiliar:
        do_not_optimize(R);
    }
    auto end_intrinsic = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration_intrinsic = end_intrinsic - start_intrinsic;

    // Medición de Horner Intrinsic float (AVX)
    auto start_intrinsic_f = chrono::high_resolution_clock::now();
    for(int j = 0; j < NUM_TRIALS; j++){
        R = horner_intrinsic_f(X, coeficientes_f, SIZE);
        // para que el bucle no se elimine en la flag -O2, se utiliza la funcion auxiliar:
        do_not_optimize(R);
    }
    auto end_intrinsic_f = chrono::high_resolution_clock::now();

    chrono::duration<double, milli> duration_intrinsic_f = end_intrinsic_f - start_intrinsic_f;

    // Resultados
    cout << "Resultados de " << NUM_TRIALS << " ejecuciones" << endl;
    cout << "Horner Clásico:   " << duration_classic.count() << " ms" << endl;
    cout << "Horner Intrinsics double: " << duration_intrinsic.count() << " ms" << endl;
    cout << "Horner Intrinsics float: " << duration_intrinsic_f.count() << " ms" << endl;
    cout << "Aceleración Clásico vs Double: " << duration_classic.count() / duration_intrinsic.count() << "x" << endl;
    cout << "Aceleración Clásico vs Float: " << duration_classic.count() / duration_intrinsic_f.count() << "x" << endl;
    cout << "Aceleración Double vs Float: " << duration_intrinsic.count() / duration_intrinsic_f.count() << "x" << endl;

    _mm_free(coeficientes);
    _mm_free(coeficientes_f);
    // En la versión vectorial AVX, al desempaquetar 4 términos independientes a la vez, eliminas cuellos de botella en la pipeline del procesador
}
