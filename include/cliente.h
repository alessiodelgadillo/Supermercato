#ifndef CLIENTE_H
#define CLIENTE_H

#include <Bqueue.h>
#include <cassiere.h>
#include <supermercato.h>

typedef enum {
    SPESA,
    IN_FILA,
    SERVITO,
    CAMBIA_CASSA,
}Stato_Cliente;

typedef struct{
    int clienteId;                //identificatore cliente
    Stato_Cliente stato;          //stato del cliente
    int products;                 //quantità prodotti del cliente
    int timeToShop;               //tempo usato dal cliente per la spesa
    unsigned int seed;            //seed per la scelta casuale delle casse

    int casseTot;                 //numero delle casse presenti nel supermercato
    int* casse_aperte;            //numero delle casse aperte all'interno del supermercato
    Stato_Cassa* statoCassa;      //stato delle casse
    BQueue_t **casse;             //insieme delle code delle casse
    pthread_mutex_t* casse_mtx;   //mutex per accedere all'insieme delle code

    pthread_mutex_t* mtx;         //mutex per accedere ai dati del cliente
    pthread_cond_t* cond_cliente; //variabile di condizione del cliente

    pthread_mutex_t* auth_mtx;    //mutex per accedere all'array delle autorizzazioni
    pthread_cond_t* auth_cond;    //variabile di condizione per attendere l'autorizzazione
    char* authorization;          //puntatore a una posizione dell'array delle autorizzazioni

    pthread_mutex_t* exit_mtx;    //mutex per accedere alle variabili che indicano l'uscita del cliente
    pthread_cond_t* exit_cond;    //variabile di condizione per avvertire il supermercato dell'uscita
    int* exited_clients;          //numero di clienti usciti dal supermercato
    int isExited;                 //varabile che indica se il cliente è uscito

    double TotalTime;             //tempo di permanenza all'interno del supermercato
    double WaitTime;              //tempo totale speso in coda
    int CodeCambiate;             //numero di code cambiate



}Cliente;

void* cliente (void* arg);

#endif
