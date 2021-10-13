#ifndef QUEUE_H
#define QUEUE_H

typedef struct _nodo {
  double dato;
  struct _nodo *next;
}Nodo;

typedef Nodo* NodoPtr;

/* inserisce un nuovo nodo alla fine della coda
 * param testaPtr: puntatore alla testa della coda
 * param codaPtr: puntatore alla fine della coda
 * param val: valore da inserire nel campo dato del nuovo nodo */
void push (NodoPtr* testaPtr, NodoPtr *codaPtr, double val);

/* estrae un elemento dalla testa della coda
 * param testaPtr: puntatore alla testa della coda
 * param codaPtr: puntatore alla fine della coda
 * restituisce il dato presente nel nodo della testa della coda */
double pop (NodoPtr* testaPtr, NodoPtr* codaPtr);

#endif
