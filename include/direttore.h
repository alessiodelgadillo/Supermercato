#ifndef DIRETTORE_H
#define DIRETTORE_H

#include<cassiere.h>
#include <supermercato.h>

#define MIN_CLIENTI 1


typedef struct{
  pthread_mutex_t* casse_mtx;       //mutex per accedere all'insieme delle casse
  pthread_cond_t* casse_cond;       //variabile di condizione delle casse per svegliarle quando vengono aperte

  Stato_Cassa* statoCasse;          //stato delle casse
  int* casse_aperte;                //numero delle casse aperte nel supermercato
  int casseTot;                     //numero delle casse presenti nel supermercato

  int* dim_casse;                   //array per la lunghezza delle casse
  pthread_cond_t* notifiche;        //variabile di condizione sulla quale il direttore attende le notifiche

  pthread_mutex_t* auth_mtx;        //mutex per accedere all'array delle autorizzazioni
  pthread_cond_t* auth_cond;        //variabile di condizione per autorizzare i clienti
  char* authorization;              //array delle autorizzazioni
  int clientiMax;                   //numero massimo di clienti nel supermercato
  int* exited_clients;              //numero clienti usciti dal supermercato

  int S1;                           //soglia per la chiusura di una cassa
  int S2;                           //soglia per l'apertura di una cassa

}Direttore;

void* direttore_fun(void* arg);

#endif
