/*
Tarea:  Implementar las estructuras FIFO y LIFO dentro
        del código de la doble lista enlazada
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct comparte {
    char nombre[50];
    char direccion[50];
    int edad;
    struct comparte *next;
    struct comparte *prev;
} Registro;

typedef struct l {
    Registro *Inicial;
    Registro *Final;
    int len;
} Lista;

Registro *crear() {
    Registro *T = (Registro *)malloc(sizeof(Registro));
    if (T == NULL) {
        return NULL;
    }
    T->next = NULL;
    T->prev = NULL;
    return T;
}

void crear_lista(Lista *lista) {
    lista->Inicial = crear(); // Como tenemos inicial y final determinados, las FIFO y LIFO salen solas
    lista->Final = crear();
    
    if (lista->Inicial == NULL || lista->Final == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for sentinel nodes.\n");
        exit(EXIT_FAILURE);
    }

    // Enlazamos las variables sentinela para establecer la estructura correctamente
    lista->Inicial->next = lista->Final;
    lista->Inicial->prev = NULL;

    lista->Final->prev = lista->Inicial;
    lista->Final->next = NULL;

    lista->len = 0;
}

int agregar(Lista *lista, const char *nombre, const char *direccion, int edad) {
    Registro *N = crear();
    if (N == NULL) {
        return 0; // Si falla el alojamiento
    }

    strncpy(N->nombre, nombre, sizeof(N->nombre) - 1);
    N->nombre[sizeof(N->nombre) - 1] = '\0';

    strncpy(N->direccion, direccion, sizeof(N->direccion) - 1);
    N->direccion[sizeof(N->direccion) - 1] = '\0';

    N->edad = edad;

    // Se agrega el elemento antes de lista->Final
    N->prev = lista->Final->prev;
    N->next = lista->Final;

    lista->Final->prev->next = N;
    lista->Final->prev = N;

    lista->len++;
    return 1;
}

int borrar(Lista *lista, const char *nombre) {
    Registro *iter;

    for (iter = lista->Inicial->next; iter != lista->Final; iter = iter->next) {
        if (strcmp(iter->nombre, nombre) == 0) {
            iter->prev->next = iter->next;
            iter->next->prev = iter->prev;

            free(iter);
            lista->len--;
            return 1; // Si es correctamente eliminado
        }
    }

    return 0; // No se encontró
}

void destruir_lista(Lista *lista) {
    Registro *iter = lista->Inicial;
    while (iter != NULL) {
        Registro *temp = iter->next;
        free(iter);
        iter = temp;
    }
    lista->Inicial = NULL;
    lista->Final = NULL;
    lista->len = 0;
}

void print_list(const Lista *lista) {
    Registro *iter;
    for (iter = lista->Inicial->next; iter != lista->Final; iter = iter->next) {
        printf("Nombre: %s | Direccion: %s | Edad: %d\n", iter->nombre, iter->direccion, iter->edad);
    }
}

void find_in_list(const Lista *lista, const char *nombre) {
    Registro *iter;
    for (iter = lista->Inicial->next; iter != lista->Final; iter = iter->next) {
        if (strcmp(iter->nombre, nombre) == 0) {
            printf("Encontrado - Nombre: %s, Direccion: %s, Edad: %d\n", iter->nombre, iter->direccion, iter->edad);
            return;
        }
    }
    printf("Elemento '%s' no encontrado.\n", nombre);
}

// Le añadimos un nuevo nodo (registro) a la pila de la lista (antes de lista->final que es el marcador final)
int push_lista(Lista *lista, const char *nombre, const char *direccion, int edad){
    Registro *N = crear();
    if (N == NULL) return 0; // Si falla el alojamiento

    strncpy(N->nombre, nombre, sizeof(N->nombre) - 1);
    N->nombre[sizeof(N->nombre) - 1] = '\0';

    strncpy(N->direccion, direccion, sizeof(N->direccion) - 1);
    N->direccion[sizeof(N->direccion) - 1] = '\0';

    N->edad = edad;

    N->prev = lista->Final->prev;
    N->next = lista->Final;

    lista->Final->prev->next = N;
    lista->Final->prev = N;

    lista->len++;
    return 1;
}

// Le quitamos el ultimo nodo (registro) de la pila a la lista (y reintegramos la conexion de nodos hacia final)
int pop_lista(Lista *lista){
    if (lista->len==0) return 0; // no hay registros que se puedan quitar de la lista

    Registro *pop_registro=lista->Final->prev;

    pop_registro->prev->next = lista->Final; // hacemos que el prev apunte al final
    lista->Final->prev = pop_registro->prev; // hacemos que el prev del final apunte al prev

    // char nombre[50] = pop_registro->nombre; // se podrían guardar los valores de pop para lectura.
    free(pop_registro);
    lista->len-=1;

    return 1;
}

// misma funcionalidad que push_lista
int enqueue_lista (Lista *lista, const char *nombre, const char *direccion, int edad){
    Registro *N = crear();
    if (N == NULL) return 0; // Si falla el alojamiento

    strncpy(N->nombre, nombre, sizeof(N->nombre) - 1);
    N->nombre[sizeof(N->nombre) - 1] = '\0';

    strncpy(N->direccion, direccion, sizeof(N->direccion) - 1);
    N->direccion[sizeof(N->direccion) - 1] = '\0';

    N->edad = edad;

    N->prev = lista->Final->prev;
    N->next = lista->Final;

    lista->Final->prev->next = N;
    lista->Final->prev = N;

    lista->len++;
    return 1;
}

// Aqui agregamos los elementos (registros) en el inicio (justo despues de lista->Inicial)
int dequeue_lista (Lista *lista){

    if (lista->len==0) return 0; // no hay registros que se puedan quitar de la lista

    Registro *pop_registro=lista->Inicial->next;

    lista->Inicial->next = pop_registro->next; // hacemos que el next del inicio apunte al prev
    pop_registro->next->prev = lista->Inicial; // hacemos que el prev del registro apunte al inicial

    // char nombre[50] = pop_registro->nombre; // se podrían guardar los valores de pop para lectura.
    free(pop_registro);
    lista->len-=1;

    return 1;
}

int main() {
    Lista lista;
    crear_lista(&lista);

    agregar(&lista, "Algo", "Otra Cosa", 99);
    agregar(&lista, "Maria", "Calle 999", 22);

    printf("Contenido inicial de la lista:\n");
    print_list(&lista);

    printf("\nElemento buscado:\n");
    find_in_list(&lista, "Algo");

    borrar(&lista, "Algo");
    printf("\nLista despues de borrado:\n");
    print_list(&lista);

    // Prueba de push / pop:
    printf("\nPush:\n");
    push_lista(&lista,"JJ","Casa",23);
    print_list(&lista);
    printf("\nPop:\n");
    pop_lista(&lista);
    print_list(&lista);

    // Prueba de enqueue / dequeue:
    printf("\nEnqueue:\n");
    enqueue_lista(&lista,"CC","Cuarto",28);
    print_list(&lista);
    printf("\nDequeue:\n");
    dequeue_lista(&lista);
    print_list(&lista);
    

    destruir_lista(&lista);
    return 0;
}