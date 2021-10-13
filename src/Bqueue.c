#include <stdlib.h>
#include <pthread.h>
#include <Bqueue.h>
#include <stdio.h>

#define PTHREAD_CALL(p,s)\
                if (p != 0){\
                perror(s);\
                exit(EXIT_FAILURE);}

#define CHECK_PTR(p,s)\
                if (p == NULL){\
                perror(s);\
                exit(EXIT_FAILURE);}

/* ====================================================================== */

BQueue_t* init (size_t dim){
    BQueue_t* Q = (BQueue_t*) malloc(sizeof(BQueue_t));
    CHECK_PTR(Q,"cannot init BQueue_t")
    Q->buf = malloc(sizeof(void*)*dim);
    CHECK_PTR(Q->buf, "cannot malloc buf")
    Q->head = Q->tail = 0;
    Q->len = 0;
    Q->size = dim;
    PTHREAD_CALL(pthread_mutex_init(&(Q->mtx), NULL), "mutex_init")
    PTHREAD_CALL(pthread_cond_init(&(Q->empty), NULL), "cond empty init")
    PTHREAD_CALL(pthread_cond_init(&(Q->full), NULL), "cond full init")
    return Q;
}

/* ---------------------------------------------------------------------- */

void deinit(BQueue_t* Q){
    free(Q->buf);
    PTHREAD_CALL(pthread_mutex_destroy(&Q->mtx), "mutex destroy")
    PTHREAD_CALL(pthread_cond_destroy(&Q->empty), "empty destroy")
    PTHREAD_CALL(pthread_cond_destroy(&Q->full), "full destroy")
    free(Q);
}

/* ---------------------------------------------------------------------- */

void enqueue (BQueue_t *Q, void* dato){
    PTHREAD_CALL(pthread_mutex_lock(&Q->mtx), "lock Q mtx")
    while (Q->len == Q->size) PTHREAD_CALL(pthread_cond_wait(&Q->full, &Q->mtx), "wait full")
    Q->buf[Q->tail] = dato;
    Q->tail +=  (Q->tail+1 >= Q->size) ? (1-Q->size) : 1;
    Q->len++;
    PTHREAD_CALL(pthread_cond_signal(&Q->empty), "signal empty")
    PTHREAD_CALL(pthread_mutex_unlock(&Q->mtx), "unlock Q mtx")
}

/* ---------------------------------------------------------------------- */

void* dequeue(BQueue_t *Q){
  PTHREAD_CALL(pthread_mutex_lock(&Q->mtx), "lock Q mtx")
    if (Q->len == 0){
      PTHREAD_CALL(pthread_mutex_unlock(&Q->mtx), "unlock Q mtx")
      return NULL;
    }
    void* dato = Q->buf[Q->head];
    Q->buf[Q->head] = NULL;
    Q->head += (Q->head+1 >= Q->size) ? (1-Q->size) : 1;
    Q->len--;
    PTHREAD_CALL(pthread_cond_signal(&Q->full), "signal full")
    PTHREAD_CALL(pthread_mutex_unlock(&Q->mtx), "unlock Q mtx")
    return dato;
}

/* ---------------------------------------------------------------------- */

int getSize(BQueue_t* Q){
  PTHREAD_CALL(pthread_mutex_lock(&Q->mtx), "lock Q mtx")
  int size = Q->len;
  PTHREAD_CALL(pthread_mutex_unlock(&Q->mtx), "unlock Q mtx")
  return size;
}

/* ====================================================================== */
