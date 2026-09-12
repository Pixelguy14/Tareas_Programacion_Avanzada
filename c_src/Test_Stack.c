/*
 * Test_Stack.c — Demostración de una pila (LIFO) usando un arreglo.
 *
 * Emite líneas con el formato:
 *   PUSH <val> | STACK: <e0> <e1> ... <eN>
 *   POP <val>  | STACK: <e0> <e1> ... <eN>
 * que el visualizador de la GUI puede parsear y animar.
 *
 * Relación temática: Sección 2.1 — Pilas
 *
 * Compilar standalone:
 *   gcc -o Test_Stack.bin c_src/Test_Stack.c
 */

#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 10

typedef struct {
    int data[MAX_SIZE];
    int top;
} Stack;

void stack_init(Stack *s)  { s->top = -1; }
int  is_empty(Stack *s)    { return s->top == -1; }
int  is_full(Stack *s)     { return s->top == MAX_SIZE - 1; }

/* Imprime el estado actual de la pila (bottom → top) */
static void print_state(Stack *s) {
    printf("STACK:");
    for (int i = 0; i <= s->top; i++)
        printf(" %d", s->data[i]);
    printf("\n");
}

void push(Stack *s, int val) {
    if (is_full(s)) { printf("OVERFLOW\n"); return; }
    s->data[++s->top] = val;
    printf("PUSH %d | ", val);
    print_state(s);
}

int pop(Stack *s) {
    if (is_empty(s)) { printf("UNDERFLOW\n"); return -1; }
    int val = s->data[s->top--];
    printf("POP %d | ", val);
    print_state(s);
    return val;
}

int main() {
    Stack s;
    stack_init(&s);

    push(&s, 10);
    push(&s, 20);
    push(&s, 30);
    push(&s, 40);
    pop(&s);
    push(&s, 50);
    pop(&s);
    pop(&s);
    pop(&s);

    return 0;
}
