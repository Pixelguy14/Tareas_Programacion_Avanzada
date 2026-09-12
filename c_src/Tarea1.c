/*
Tarea:  Implementar la multiplicación de dos números enteros de 128 bits 
        usando uniones de C y la biblioteca Intel intrinsics
        con resultado en 128 bits
*/
#include <stdio.h>
#include <emmintrin.h>
#include <immintrin.h>

/*
union Vector4int{ // vector 128 bits que se puede descomponer en 4*8 arreglos de bits int
    int Num4[4];
    __m128i Num;
};
*/

// Descompone cada número de 128 bits en dos mitades de 64 bits (Alto y Bajo)
// Esta función requiere un procesador con soporte para la instrucción BMI2 (_mulx_u64)
// Se debe compilar con la bandera -O2 -march=native
union vector2int{ // vector de 128 bits descompuestos en dos arreglos long long
    long long Num2[2]; // Num2[0]=Low, Num2[1]=High
    __m128i Num;
};

// Metodo de la multiplicación larga
// Resultado de 128 bits truncado a mod 2^128.
union vector2int multiplicador128b (union vector2int *N1, union vector2int *N2);

int main() {
    //union Vector4int V4_1;
    union vector2int V1;
    union vector2int V2;
    union vector2int Res;

    // Si necesitamos llenar los numeros, se utiliza:
    V1.Num2[0]=10;
    V1.Num2[1]=20;

    V2.Num2[0]=100;
    V2.Num2[1]=200;

    Res= multiplicador128b(&V1,&V2);

    // Dado que la funcion __int64_mulx_u64 en realidad, multiplica 2 ints empaquetados (operando en datos de 32 bits)
    printf("\n %lld %lld",V1.Num2[0],V1.Num2[1]);
    printf(" * %lld %lld",V2.Num2[0],V2.Num2[1]);
    printf(" = %lld %lld\n",Res.Num2[0],Res.Num2[1]);

    printf("\n El tamaño del union es de: %lu",sizeof(Res));

    return 0;
}

// Sea A = AH * 2^64 + AL  (A segmentado en 64 bits alto y bajo)
// Sea B = BH * 2^64 + BL
// A * B = AL*BL  +  (AL*BH + AH*BL) * 2^64  +  AH*BH * 2^128
// A * B = (AL*BL) + (AL*BH + AH*BL) * 2^64 (mod 2^128) 
union vector2int multiplicador128b (union vector2int *N1, union vector2int *N2){
    union vector2int Res;
    // Num2[0]=Low, Num2[1]=High
    // Res = N1->Num2[0]*N2->Num2[0] + (N1->Num2[0]*N2->Num2[1]+N1->Num2[1]*N2->Num2[0]) * pow(2,64) % pow(2,128)
    unsigned long long lo1, hi1;
    lo1 = _mulx_u64(N1->Num2[0],N2->Num2[0],&hi1); // AL * BL
    unsigned long long lo2, hi2;
    lo2 = _mulx_u64(N1->Num2[0],N2->Num2[1],&hi2); // AL * BH
    unsigned long long lo3, hi3;
    lo3 = _mulx_u64(N1->Num2[1],N2->Num2[0],&hi3); // AH * BL

    // (AL*BH + AH*BL)
    // Sumamos las partes bajas de los productos cruzados
    unsigned long long sumlo, sumhi;
    unsigned char sumcarry = _addcarry_u64(0, lo2, lo3, &sumlo);
    _addcarry_u64(sumcarry, hi2, hi3, &sumhi); // Propagamos el posible acarreo hacia las partes altas de los productos cruzados

    // El segmento pow(2,64) % pow(2,128) no se toma debido al nivel de la ecuación

    // Se suma al resultado:
    Res.Num2[0] = lo1;
    _addcarry_u64(0, hi1, sumlo, &Res.Num2[1]);
    return Res;
}

/* Validación en Sage:

# Define A and B as 128-bit numbers split into high and low 64-bit parts
AL, AH = 10, 20
BL, BH = 100, 200

A = AH * 2^64 + AL
B = BH * 2^64 + BL

# Perform 128-bit multiplication (mod 2^128)
R = (A * B) % 2^128

# Extract low and high 64-bit components
result_lo = R % 2^64
result_hi = R // 2^64

print(f"Res.Num2[0] (Low 64 bits): {result_lo}")
print(f"Res.Num2[1] (High 64 bits): {result_hi}")

# output: 
# Res.Num2[0] (Low 64 bits): 1000
# Res.Num2[1] (High 64 bits): 4000

*/