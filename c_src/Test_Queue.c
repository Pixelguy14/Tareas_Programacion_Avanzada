/*
 * Test_Queue.c — Demostración de una cola circular (FIFO) usando un arreglo.
 *
 * Emite líneas con el formato:
 *   ENQUEUE <val> | QUEUE: <e0> <e1> ... <eN>
 *   DEQUEUE <val> | QUEUE: <e0> <e1> ... <eN>
 * que el visualizador de la GUI puede parsear y animar.
 *
 * Relación temática: Sección 2.1 — Colas
 *
 * Compilar standalone:
 *   gcc -o Test_Queue.bin c_src/Test_Queue.c
 */

#include <stdio.h>

#define MAX_SIZE 10

typedef struct {
    int data[MAX_SIZE];
    int front;
    int rear;
    int count;
} Queue;

void queue_init(Queue *q) { q->front = 0; q->rear = -1; q->count = 0; }
int  is_empty(Queue *q)   { return q->count == 0; }
int  is_full(Queue *q)    { return q->count == MAX_SIZE; }

/* Imprime el estado actual de la cola (front → rear) */
static void print_state(Queue *q) {
    printf("QUEUE:");
    for (int i = 0; i < q->count; i++)
        printf(" %d", q->data[(q->front + i) % MAX_SIZE]);
    printf("\n");
}

void enqueue(Queue *q, int val) {
    if (is_full(q)) { printf("OVERFLOW\n"); return; }
    q->rear = (q->rear + 1) % MAX_SIZE;
    q->data[q->rear] = val;
    q->count++;
    printf("ENQUEUE %d | ", val);
    print_state(q);
}

int dequeue(Queue *q) {
    if (is_empty(q)) { printf("UNDERFLOW\n"); return -1; }
    int val = q->data[q->front];
    q->front = (q->front + 1) % MAX_SIZE;
    q->count--;
    printf("DEQUEUE %d | ", val);
    print_state(q);
    return val;
}

int main() {
    Queue q;
    queue_init(&q);

    enqueue(&q, 10);
    enqueue(&q, 20);
    enqueue(&q, 30);
    dequeue(&q);
    enqueue(&q, 40);
    enqueue(&q, 50);
    dequeue(&q);
    dequeue(&q);

    return 0;
}
