#include <Queue.h>
#include <stdio.h>
#include <stdlib.h>

void push (NodoPtr* testaPtr, NodoPtr *codaPtr, double val){
  NodoPtr nuovoPtr = malloc(sizeof(Nodo));
  if (nuovoPtr != NULL){
    nuovoPtr->dato = val;
    nuovoPtr->next = NULL;
    if (*testaPtr == NULL){
      *testaPtr = nuovoPtr;
      *codaPtr = *testaPtr;
    }
    else {
      (*codaPtr)->next = nuovoPtr;
      *codaPtr = nuovoPtr;
    }
  }
  else{
    fprintf(stderr, "malloc failed\n");
    exit(EXIT_FAILURE);
  }
}

double pop (NodoPtr* testaPtr, NodoPtr* codaPtr){
  double val = (*testaPtr)->dato;
  NodoPtr tmpPtr = *testaPtr;
  *testaPtr= (*testaPtr)->next;
  if (testaPtr == NULL) *codaPtr = NULL;
  free(tmpPtr);
  return val;
}
