#ifndef CASSIERE_H
#define CASSIERE_H

#include <pthread.h>
#include <Bqueue.h>
#include <supermercato.h>
#include <Queue.h>


#define NOTIFY_NOT_EXISTS -1

#define MAX_TS 80
#define MIN_TS 20


typedef enum {
    APERTA,
    CHIUSA,
    TERMINATA,
}Stato_Cassa;

typedef struct {
    int cassaId;              //identificatore della cassa

    BQueue_t* cassa;          //coda della cassa
    Stato_Cassa* statoCassa;  //stato della cassa
    pthread_mutex_t* mtx;     //mutex della cassa
    pthread_cond_t* close;    //variabile di condizione della cassa

    pthread_cond_t* notifica; //variabile di condizione con cui avviso il direttore della notifica
    int* dim_cassa;           //puntatore per "mandare" la lunghezza della coda al direttore

    int time;                 //tempo di servizio base per ogni cliente
    int time_products;        //tempo di servizio per ogni prodotto
    int time_notifica;        //intervallo di tempo per ogni notifica

    int ClientiServiti;       //numero di clienti serviti
    int ProdottiElaborati;    //numero di prodotti elaborati
    int Chiusura;             //numero di chiusure
    NodoPtr ServiceTime_head; //coda in cui vengono salvati i tempi di servizio
    NodoPtr ServiceTime_tail;
    NodoPtr TotalTime_head;   //coda in cui vengono salvati i tempi di apertura
    NodoPtr TotalTime_tail;


    //aggiungere lo stato del supermercato per i controlli
    //mancano variabili per il file di log

}Cassiere;

void* cassiere (void* arg);

#endif
