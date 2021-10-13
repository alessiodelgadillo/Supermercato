#ifndef BQUEUE_H
#define BQUEUE_H
#include <pthread.h>

typedef struct Bqueue{
    void** buf;
    size_t head;
    size_t tail;
    size_t len;
    size_t size;
    pthread_mutex_t mtx;
    pthread_cond_t empty;
    pthread_cond_t full;

}BQueue_t;

/* alloca e inzializza una coda
 * param dim: dimensione della coda
 * restituisce il puntatore alla coda allocata */
BQueue_t* init (size_t dim);

/* cancella una coda allocata con initQueue
 * param Q: puntatore alla coda da cancellare */
void deinit(BQueue_t* Q);

/* inserisce un dato nella coda Q
 * param dato: puntatore al dato da inserire */
void enqueue (BQueue_t *Q, void* dato);

/* estrae un dato dalla coda
 * restituisce un puntatore al dato estratto */
void* dequeue(BQueue_t *Q);

/* ritorna il numero di elementi presenti nella coda */
int getSize(BQueue_t* Q);

#endif
