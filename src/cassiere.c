#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <cassiere.h>
#include <cliente.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

#define PTHREAD_CALL(p,s)\
                if (p != 0){\
                perror(s);\
                exit(EXIT_FAILURE);}

//Prototipi delle funzioni usate dal Cassiere durante la propria esecuzione

/* ============================================================== */

//libera tutta la coda per la cassa avvisando i clienti di rimettersi in fila
static void freeQueue (BQueue_t* cassa);

//cambia lo stato del cliente a newState e lo avvisa
static void changeClientState (Cliente* cliente, Stato_Cliente newState);

/* simula:
          * il tempo necessario per servire un cliente;
          * l'intervallo della notifica al direttore.
*/
static void mySleep (int time);

//informa il direttore dei clienti attualmente in coda alla cassa
static void invioNotifica(Cassiere* cassiere);

/* ============================================================== */

void* cassiere (void* arg){

    //imposto la maschera dei segnali
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGQUIT);
    PTHREAD_CALL(pthread_sigmask(SIG_SETMASK, &mask, NULL), "sigmask cassiere")


    Cassiere* cassiere = (Cassiere*) arg;

    /* inizializzo le variabili e le strutture necessarie
       alla creazione del file di log */

    cassiere->ClientiServiti = 0;
    cassiere->ProdottiElaborati = 0;
    cassiere->Chiusura = 0;
    double TimeApertura = 0;
    double ServiceTime = 0;

    cassiere->ServiceTime_head = NULL;
    cassiere->ServiceTime_tail = NULL;

    cassiere->TotalTime_head = NULL;
    cassiere->TotalTime_tail = NULL;

    struct timespec StartApertura = {0,0};
    struct timespec EndApertura = {0,0};

    /* se la cassa è una di quelle aperte inizialmente
       mi salvo l'istante di apertura */
    if (*(cassiere->statoCassa) == APERTA) clock_gettime(CLOCK_MONOTONIC, &StartApertura);


    while (1) {

      //controllo lo stato della cassa

      PTHREAD_CALL(pthread_mutex_lock(cassiere->mtx), "lock cassiere mtx")

      if (*(cassiere->statoCassa) == CHIUSA){

        if (StartApertura.tv_sec != 0 && StartApertura.tv_nsec != 0 ){

          /* se la cassa era aperta, salvo l'istante di chiusura
             faccio la differenza e salvo l'intervallo */
          clock_gettime(CLOCK_MONOTONIC, &EndApertura);
          TimeApertura = ((double)EndApertura.tv_sec + 1.0e-9 * EndApertura.tv_nsec) -
                                ((double)StartApertura.tv_sec + 1.0e-9 * StartApertura.tv_nsec);
          push(&(cassiere->TotalTime_head), &(cassiere->TotalTime_tail), TimeApertura);

          StartApertura.tv_sec = 0;
          StartApertura.tv_nsec = 0;
          EndApertura.tv_sec = 0;
          EndApertura.tv_nsec = 0;
        }

        cassiere->Chiusura++;
        freeQueue(cassiere->cassa);

        //finché la cassa è chiusa mi metto in attesa
        while (*(cassiere->statoCassa) == CHIUSA){
          PTHREAD_CALL(pthread_cond_wait(cassiere->close, cassiere->mtx), "wait cassiere close")
        }

        if (*(cassiere->statoCassa) == TERMINATA){
          PTHREAD_CALL(pthread_mutex_unlock(cassiere->mtx), "unlock cassiere mtx")
          break;
        }

        /* se la cassa non è più chiusa e non è terminata,
           allora mi salvo l'istante di apertura */
        clock_gettime(CLOCK_MONOTONIC, &StartApertura);
      }

      if (*(cassiere->statoCassa) == TERMINATA){
          /* se la cassa era aperta e adesso deve terminare,
             salvo l'istante di chiusura, faccio la differenza
             e salvo l'intervallo */
          clock_gettime(CLOCK_MONOTONIC, &EndApertura);
          TimeApertura = ((double)EndApertura.tv_sec + 1.0e-9 * EndApertura.tv_nsec) -
                                ((double)StartApertura.tv_sec + 1.0e-9 * StartApertura.tv_nsec);
          push(&(cassiere->TotalTime_head), &(cassiere->TotalTime_tail), TimeApertura);

          PTHREAD_CALL(pthread_mutex_unlock(cassiere->mtx), "unlock cassiere mtx")
          break;
      }
      PTHREAD_CALL(pthread_mutex_unlock(cassiere->mtx), "unlock cassiere mtx")

      //fine del controllo sullo stato della cassa

      int notifica = cassiere->time_notifica;

      void* tmp = NULL;

      //estraggo il cliente dalla coda
      if ((tmp = dequeue(cassiere->cassa)) == NULL){
        mySleep(notifica);

        invioNotifica(cassiere);
        continue;
      }

      Cliente* cliente = (Cliente*)tmp;

      //calcolo il tempo di servizio
      PTHREAD_CALL(pthread_mutex_lock(cliente->mtx), "lock cliente mtx")
      int time_service = cassiere->time + cassiere->time_products * cliente->products;
      ServiceTime  =  (time_service * 1.0e-3);
      //salvo il tempo di servizio e aggiorno i prodotti elaborati
      push(&(cassiere->ServiceTime_head), &(cassiere->ServiceTime_tail), ServiceTime);
      cassiere->ProdottiElaborati+=cliente->products;
      PTHREAD_CALL(pthread_mutex_unlock(cliente->mtx), "unlock cliente mtx")

      //servo il cliente e mando le notifiche
      while(time_service >= notifica){
        mySleep(notifica);
        invioNotifica(cassiere);
        time_service -= notifica;
      }

      mySleep(time_service);
      changeClientState(cliente, SERVITO);
      cassiere->ClientiServiti++;

      notifica-=time_service;
      mySleep(notifica);
      invioNotifica(cassiere);

    }
    freeQueue(cassiere->cassa);


    return NULL;
}

/* ====================================================================== */

static void freeQueue (BQueue_t* cassa){
  PTHREAD_CALL(pthread_mutex_lock(&cassa->mtx), "lock BQueue_t mtx")
  while (cassa->len > 0){
    Cliente* tmp = (Cliente*) cassa->buf[cassa->head];
    changeClientState(tmp, CAMBIA_CASSA);
    cassa->buf[cassa->head] = NULL;
    cassa->head += (cassa->head+1 >= cassa->size) ? (1-cassa->size) : 1;
    cassa->len--;
  }
  PTHREAD_CALL(pthread_mutex_unlock(&cassa->mtx), "unlock BQueue_t mtx")
}

/* ---------------------------------------------------------------------- */

static void changeClientState (Cliente* cliente, Stato_Cliente newState){
  PTHREAD_CALL(pthread_mutex_lock(cliente->mtx), "lock cliente mtx")
  cliente->stato = newState;
  PTHREAD_CALL(pthread_cond_signal(cliente->cond_cliente), "signal cond_cliente")
  PTHREAD_CALL(pthread_mutex_unlock(cliente->mtx), "unlock cliente mtx")

}

/* ---------------------------------------------------------------------- */

static void mySleep (int time) {
    struct timespec t = {time/1000, (time %1000)*1000000};
    while (nanosleep(&t, &t) && errno == EINTR);
}

/* ---------------------------------------------------------------------- */

static void invioNotifica(Cassiere* cassiere){
    int size = getSize(cassiere->cassa);
    PTHREAD_CALL(pthread_mutex_lock(cassiere->mtx), "lock cassiere mtx")
    *(cassiere->dim_cassa) = size;
    PTHREAD_CALL(pthread_cond_signal(cassiere->notifica), "signal notifica")
    PTHREAD_CALL(pthread_mutex_unlock(cassiere->mtx), "unlock cassiere mtx")
}

/* ====================================================================== */
