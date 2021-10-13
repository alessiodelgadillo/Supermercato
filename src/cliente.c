#define _POSIX_C_SOURCE 200112L
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>
#include <cliente.h>
#include <cassiere.h>
#include <Bqueue.h>
#include <stdio.h>
#include <signal.h>

#define PTHREAD_CALL(p,s)\
                if (p != 0){\
                perror(s);\
                exit(EXIT_FAILURE);}

//Prototipi delle funzioni usate dal Cliente durante la propria esecuzione

/* ============================================================ */

//simula il tempo necessario per fare la spesa
static void spesa (int time);

/* sceglie una coda in maniera random e vi si mette in fila
 * restituisce 1 in caso di successo, 0 altrimenti */
static int getQueue (Cliente* cliente);

//mette il cliente in attesa passiva fino al cambiamento del suo stato
static void attendoTurno (Cliente* cliente);

//imposta lo stato del cliente a newState
static void setClientState (Cliente* cliente, Stato_Cliente newState);

//restituisce lo stato corrente del cliente
static Stato_Cliente getState (Cliente* cliente);

//chiede l'autoorizzazione per uscire al direttore
static void askAuthorization (Cliente* cliente);

//informa il supermercato dell'uscita
static void uscita (Cliente* cliente);

/* ============================================================ */

extern volatile char need_auth;


void* cliente (void* arg) {

  //imposto la maschera dei segnali
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGQUIT);
  PTHREAD_CALL(pthread_sigmask(SIG_SETMASK, &mask, NULL), "sigmask cliente")

  Cliente *cliente = (Cliente *) arg;

  /* inizializzo le variabili e le strutture necessarie
     alla creazione del file di log */

  struct timespec startPermanenza= {0,0};
  struct timespec endPermanenza= {0,0};
  struct timespec startWait= {0,0};
  struct timespec endWait= {0,0};
  clock_gettime (CLOCK_MONOTONIC, &startPermanenza);


  //faccio la spesa
  spesa(cliente->timeToShop);

  cliente->CodeCambiate = -1;
  cliente-> WaitTime = 0;

  if (cliente->products > 0) {
    //se ho preso qualche prodotto scelgo una fila e mi metto in attesa
    clock_gettime (CLOCK_MONOTONIC, &startWait);

      while(1){
          if (!getQueue(cliente)) break;
          cliente->CodeCambiate++;

          attendoTurno(cliente);

          //se sono stato servito posso uscire
          if (getState(cliente) == SERVITO){
            clock_gettime (CLOCK_MONOTONIC, &endWait);
            cliente->WaitTime = ((double)endWait.tv_sec + 1.0e-9 * endWait.tv_nsec) -
                                  ((double)startWait.tv_sec + 1.0e-9 * startWait.tv_nsec);
            break;
          }

          //se la cassa ha chiuso, cambio fila e mi rimetto in attesa
      }
  }
  else {
    //se ho 0 prodotti chiedo al direttore di uscire
    askAuthorization(cliente);

  }

  if (getState(cliente) == CAMBIA_CASSA ){
    clock_gettime (CLOCK_MONOTONIC, &endWait);
    cliente->WaitTime = ((double)endWait.tv_sec + 1.0e-9 * endWait.tv_nsec) -
                          ((double)startWait.tv_sec + 1.0e-9 * startWait.tv_nsec);
  }

  clock_gettime(CLOCK_MONOTONIC, &endPermanenza);
  cliente->TotalTime = ((double)endPermanenza.tv_sec + 1.0e-9 * endPermanenza.tv_nsec) -
                        ((double)startPermanenza.tv_sec + 1.0e-9 * startPermanenza.tv_nsec);

  //informo il supermercato di essere uscito
  uscita(cliente);

  return NULL;
}

/* ====================================================================== */

static void spesa (int time) {
  struct timespec t = {time/1000, (time %1000)*1000000};
  while (nanosleep(&t, &t) && errno == EINTR);
}

/* ---------------------------------------------------------------------- */

static int getQueue (Cliente* cliente){
  PTHREAD_CALL(pthread_mutex_lock(cliente->casse_mtx), "lock casse_mtx")

  //se ci sono ancora casse aperte ne scelgo una in modo random
  if (*(cliente->casse_aperte) > 0){
      int numero_cassa = rand_r(&(cliente->seed)) % *(cliente->casse_aperte);
      for (int i = 0; i < cliente->casseTot; i++) {
          if (cliente->statoCassa[i] == APERTA){
              if (numero_cassa == 0){
                  enqueue(cliente->casse[i], cliente);
                  setClientState(cliente, IN_FILA);
                  break;
              } else --numero_cassa;
          }
      }
      PTHREAD_CALL(pthread_mutex_unlock(cliente->casse_mtx), "unlock casse_mtx")
      return 1;
  }
  PTHREAD_CALL(pthread_mutex_unlock(cliente->casse_mtx), "unlock casse_mtx")
  return 0;
}

/* ---------------------------------------------------------------------- */

static void attendoTurno (Cliente* cliente){
  PTHREAD_CALL(pthread_mutex_lock(cliente->mtx), "lock cliente mtx")
  while (cliente->stato == IN_FILA) {
    PTHREAD_CALL(pthread_cond_wait(cliente->cond_cliente, cliente->mtx), "wait cond_cliente")
  }
  PTHREAD_CALL(pthread_mutex_unlock(cliente->mtx), "unlock cliente mtx")
}

/* ---------------------------------------------------------------------- */

static void setClientState (Cliente* cliente, Stato_Cliente newState){
  PTHREAD_CALL(pthread_mutex_lock(cliente->mtx), "lock cliente mtx")
  cliente->stato = newState;
  PTHREAD_CALL(pthread_mutex_unlock(cliente->mtx), "unlock cliente mtx")
}

/* ---------------------------------------------------------------------- */

static Stato_Cliente getState (Cliente* cliente){
  Stato_Cliente state;
  PTHREAD_CALL(pthread_mutex_lock(cliente->mtx), "lock cliente mtx")
  state = cliente->stato;
  PTHREAD_CALL(pthread_mutex_unlock(cliente->mtx), "unlock cliente mtx")
  return state;
}

/* ---------------------------------------------------------------------- */

static void askAuthorization (Cliente* cliente){
  PTHREAD_CALL(pthread_mutex_lock(cliente->auth_mtx), "lock auth_mtx")
  *(cliente->authorization) = 0;
  //mi metto in attesa dell'autorizzazione del direttore
  while (*(cliente->authorization) == 0 && need_auth){
    PTHREAD_CALL(pthread_cond_wait (cliente->auth_cond, cliente->auth_mtx), "wait auth_cond")
  }
  PTHREAD_CALL(pthread_mutex_unlock(cliente->auth_mtx), "unlock auth_mtx")
}

/* ---------------------------------------------------------------------- */

static void uscita (Cliente* cliente){
  PTHREAD_CALL(pthread_mutex_lock(cliente->exit_mtx), "lock exit_mtx")

  cliente->isExited = 1;
  *(cliente->exited_clients)+=1;
  PTHREAD_CALL(pthread_cond_signal(cliente->exit_cond), "signal exit_cond")

  PTHREAD_CALL(pthread_mutex_unlock(cliente->exit_mtx), "unlock exit_mtx")

}

/* ====================================================================== */
