/*
 * Tarea:  Implementar bien la tabla hash para aceptar colisiones, 
 * y si hay dos inserts con la misma clave, 
 * pero diferente valor, alojar en la lista de siguiente
 * (manejo de colisiones y duplicados)
 */
// g++ Tarea4.cpp -o Tarea4.bin
// ./Tarea4.bin
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAMANIO 10

// Nodo para la lista enlazada
typedef struct HashNode {
    char *clave;
    int valor;
    struct HashNode *siguiente;
} HashNode;

// Estructura de la tabla hash
typedef struct {
    HashNode *tabla[TAMANIO];
} HashTable;

// Función hash básica
unsigned int funcion_hash(const char *clave) {
    unsigned int hash = 0;
    while (*clave) {
        hash = (hash * 31) + *clave;
        clave++;
    }
    return hash % TAMANIO;
}

// Permite claves duplicadas con diferentes valores, añade al inicio los valores
void hash_insertar(HashTable *ht, const char *clave, int valor) {
    unsigned int indice = funcion_hash(clave);

    // Crear nuevo nodo
    HashNode *nuevo = (HashNode *)malloc(sizeof(HashNode));
    if (nuevo == NULL) {
        fprintf(stderr, "Error: Fallo de asignación de memoria\n");
        return;
    }
    nuevo->clave = strdup(clave);
    nuevo->valor = valor;
    
    // Insertar al principio de la lista enlazada de la cubeta
    nuevo->siguiente = ht->tabla[indice];
    ht->tabla[indice] = nuevo;
}

// Imprime o recupera todos los valores asociados a una clave
void hash_buscar_todos(HashTable *ht, const char *clave) {
    unsigned int indice = funcion_hash(clave);
    HashNode *actual = ht->tabla[indice];
    int encontrados = 0;

    printf("Buscando clave '%s':\n", clave);
    while (actual != NULL) {
        if (strcmp(actual->clave, clave) == 0) {
            printf("  -> Valor encontrado: %d\n", actual->valor);
            encontrados = 1;
        }
        actual = actual->siguiente;
    }

    if (!encontrados) {
        printf("  -> Clave no encontrada.\n");
    }
}

// Actualiza el valor de la primera coincidencia de la clave
int hash_actualizar(HashTable *ht, const char *clave, int valor_antiguo, int valor_nuevo) {
    unsigned int indice = funcion_hash(clave);
    HashNode *actual = ht->tabla[indice];

    while (actual != NULL) {
        if (strcmp(actual->clave, clave) == 0 && actual->valor == valor_antiguo) {
            actual->valor = valor_nuevo;
            return 1; // Actualizado con éxito
        }
        actual = actual->siguiente;
    }
    return 0; // No se encontró el par clave-valor exacto
}

// Elimina todos los nodos que coincidan con la clave
int hash_borrar(HashTable *ht, const char *clave) {
    unsigned int indice = funcion_hash(clave);
    HashNode *actual = ht->tabla[indice];
    HashNode *anterior = NULL;
    int borrados = 0;

    while (actual != NULL) {
        if (strcmp(actual->clave, clave) == 0) {
            HashNode *temporal = actual;

            if (anterior == NULL) { // revinculamos la lista enlazada de hash
                ht->tabla[indice] = actual->siguiente;
                actual = ht->tabla[indice];
            } else {
                anterior->siguiente = actual->siguiente;
                actual = actual->siguiente;
            }

            free(temporal->clave);
            free(temporal);
            borrados++;
        } else {
            anterior = actual;
            actual = actual->siguiente;
        }
    }
    return borrados; // Retorna cuántos elementos fueron eliminados
}

// Libera toda la memoria de la tabla hash
void hash_destruir(HashTable *ht) {
    for (int i = 0; i < TAMANIO; i++) {
        HashNode *actual = ht->tabla[i];
        while (actual != NULL) {
            HashNode *temporal = actual;
            actual = actual->siguiente;

            free(temporal->clave); // Liberar la copia de la cadena
            free(temporal);        // Liberar el nodo
        }
        ht->tabla[i] = NULL;
    }
}

int main() {
    HashTable HT;

    // Inicializar la tabla hash asegurando que apunte a NULL
    for (int i = 0; i < TAMANIO; i++) {
        HT.tabla[i] = NULL;
    }

    // Inserciones con la misma clave y diferentes valores
    hash_insertar(&HT, "Ernesto", 10);
    hash_insertar(&HT, "Braulio", 15);
    hash_insertar(&HT,"Catalina",56);
    hash_insertar(&HT,"Alondra",18);
    hash_insertar(&HT,"Freddy",59);
    hash_insertar(&HT, "Ernesto", 18); // Duplicado con diferente valor

    // Buscar todos los valores para "Ernesto"
    hash_buscar_todos(&HT, "Ernesto");

    // Prueba de actualización: cambiar el 10 de Ernesto por un 12
    printf("\nActualizando 'Ernesto' (10 -> 12)...\n");
    hash_actualizar(&HT, "Ernesto", 10, 12);
    hash_buscar_todos(&HT, "Ernesto");

    // Eliminará ambas instancias de Ernestos
    printf("\nBorrando la clave 'Ernesto'...\n");
    hash_borrar(&HT, "Ernesto");
    hash_buscar_todos(&HT, "Ernesto");

    // Liberar memoria final
    hash_destruir(&HT);

    return 0;
}