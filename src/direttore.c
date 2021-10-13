#define _POSIX_C_SOURCE 200112L
#include <direttore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define PTHREAD_CALL(p,s)\
                if (p != 0){\
                perror(s);\
                exit(EXIT_FAILURE);}

//Prototipi delle funzioni usate dal Direttore durante la propria esecuzione

/* ====================================================================== */

//gestisce le casse in base alle notifiche ricevute
static void gestioneCasse (Direttore* direttore);

/* ---------------------------------------------------------------------- */

//autorizza i clienti con 0 prodotti ad uscire
static void giveAuthorization (Direttore* direttore);

/* ---------------------------------------------------------------------- */

//restituisce lo stato corrente del supermercato
static Stato_Supermercato getMarketState (Stato_Supermercato stato);

/* ---------------------------------------------------------------------- */

//restituisce 1 se non sono ancora usciti tutti i clienti, 0 altrimenti
static int checkExitedClients (int clientiMax, int* exited_clients);
/* ====================================================================== */

extern Stato_Supermercato StatoSupermercato;
extern pthread_mutex_t supermercato_mtx;
volatile char need_auth = 1;


void* direttore_fun(void* arg){

  //imposto la maschera dei segnali
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGQUIT);
  PTHREAD_CALL(pthread_sigmask(SIG_SETMASK, &mask, NULL), "sigmask direttore")

  Direttore* direttore = (Direttore*)arg;
  while (1) {

    //controllo che il supermercato sia aperto
    if (getMarketState(StatoSupermercato) != APERTO) break;


    //controllo le notifiche delle casse
    gestioneCasse(direttore);

    //autorizzo l'uscita dei clienti con 0 prodotti
    giveAuthorization(direttore);

  }

  if (getMarketState(StatoSupermercato) == CHIUSO){
    /*è arrivato il segnale di SIGQUIT
     i clienti non necessitano più dell'autorizzazione*/
    need_auth = 0;
  }

  else {
    /* è arrivato il segnale di SIGHUP
     * finché non sono usciti tutti i clienti continuo a gestire le casse
     *  e a rilasciare le autorizzazioni ai clienti*/

    struct timespec t = {1, 0};
    while (checkExitedClients (direttore->clientiMax, direttore->exited_clients)) {
      gestioneCasse(direttore);
      giveAuthorization(direttore);
      while (nanosleep(&t, &t) && errno == EINTR);
    }

    //quando sono usciti tutti i clienti setto lo stato del supermercato a CHIUSO
    PTHREAD_CALL(pthread_mutex_lock(&supermercato_mtx), "lock supermercato_mtx")
    StatoSupermercato = CHIUSO;
    PTHREAD_CALL(pthread_mutex_unlock(&supermercato_mtx), "unlock supermercato_mtx")

  }

  //dico alle casse di terminare e setto il contatore delle casse a 0
  PTHREAD_CALL(pthread_mutex_lock(direttore->casse_mtx), "lock casse_mtx")
  for (size_t i = 0; i < direttore->casseTot; i++) {
    direttore->statoCasse[i] = TERMINATA;
  }
  *(direttore->casse_aperte) = 0;
  PTHREAD_CALL(pthread_cond_broadcast(direttore->casse_cond), "broadcast casse_cond")
  PTHREAD_CALL(pthread_mutex_unlock(direttore->casse_mtx), "unlock casse_mtx")

  return NULL;
}

/* ====================================================================== */

static void gestioneCasse (Direttore* direttore){

  PTHREAD_CALL(pthread_mutex_lock(direttore->casse_mtx), "lock casse_mtx")
  //controllo che le casse aperte abbiano mandato la notifica
  for (size_t i = 0; i < direttore->casseTot; i++) {
      if (direttore->statoCasse[i] == APERTA){
          //se non ho ancora ricevuto la notifica dalla cassa i-esima mi metto in attesa
          while(direttore->dim_casse[i] == NOTIFY_NOT_EXISTS){
            PTHREAD_CALL(pthread_cond_wait(direttore->notifiche, direttore->casse_mtx), "wait notifiche");
          }
      }
  }
  PTHREAD_CALL(pthread_mutex_unlock(direttore->casse_mtx), "unlock casse_mtx")

  int counter = 0;
  char toOpen = 0;
  int toClose = -1;

  PTHREAD_CALL(pthread_mutex_lock(direttore->casse_mtx), "lock casse_mtx")
  //controllo se i valori delle casse non rispettano le soglie
  for (size_t i = 0; i < direttore->casseTot; i++) {
      if (direttore->statoCasse[i] == APERTA){

          if (direttore->dim_casse[i] <= MIN_CLIENTI){
              /* se ci sono S1 casse aperte con #clienti <= MIN_CLIENTI
                 chiudo la S1-esima cassa */
              if (++counter == direttore->S1){
                toClose = i;
              }

          }
          else if (direttore->dim_casse[i] >= direttore->S2){
            /* se c'è almeno una cassa aperta con più di S2 clienti
               allora devo aprire una cassa */
              toOpen = 1;
          }
      }
  }
  if (toClose != -1 && direttore->statoCasse[0] == APERTA && direttore->dim_casse[0] <= MIN_CLIENTI) toClose = 0;
  //resetto l'array delle notifiche
  memset(direttore->dim_casse, NOTIFY_NOT_EXISTS, sizeof(int));
  PTHREAD_CALL(pthread_mutex_unlock(direttore->casse_mtx), "unlock casse_mtx")

  PTHREAD_CALL(pthread_mutex_lock(direttore->casse_mtx), "lock casse_mtx")
  /*se devo aprire una cassa e c'è ancora qualche cassa chiusa
    allora apro la prima cassa cassa chiusa che trovo */
  if (toOpen == 1 && *(direttore->casse_aperte) < direttore->casseTot){
      size_t i = 0;
      while (direttore->statoCasse[i] != CHIUSA) i++;
      direttore->statoCasse[i] = APERTA;
      *(direttore->casse_aperte) += 1;
      PTHREAD_CALL(pthread_cond_broadcast(direttore->casse_cond), "broadcast casse_cond")
  }
  //se devo chiudere una cassa e non è l'unica cassa aperta, chiudo la S1-esima
  if (counter >= direttore->S1 && *(direttore->casse_aperte) > 1){
    direttore->statoCasse[toClose] = CHIUSA;
    *(direttore->casse_aperte) -= 1;

  }
  PTHREAD_CALL(pthread_mutex_unlock(direttore->casse_mtx), "unlock casse_mtx")

}

/* ---------------------------------------------------------------------- */

static void giveAuthorization (Direttore* direttore){
  PTHREAD_CALL(pthread_mutex_lock(direttore->auth_mtx), "lock auth_mtx")

  //rilascio l'autorizzazione ai clienti che l'hanno richiesta
  for (size_t i = 0; i < direttore->clientiMax; i++) {
    if (direttore->authorization[i] == 0 ) direttore->authorization[i] = 1;
  }
  //risveglio i clienti in attesa dell'autorizzazione
  PTHREAD_CALL(pthread_cond_broadcast(direttore->auth_cond), "broadcast auth_cond")

  PTHREAD_CALL(pthread_mutex_unlock(direttore->auth_mtx), "unlock auth_mtx")

}

/* ---------------------------------------------------------------------- */

static Stato_Supermercato getMarketState (Stato_Supermercato stato){
  Stato_Supermercato state;
  PTHREAD_CALL(pthread_mutex_lock(&supermercato_mtx), "lock market state")
  state = stato;
  PTHREAD_CALL(pthread_mutex_unlock(&supermercato_mtx), "unlock market state")
  return state;
}

/* ---------------------------------------------------------------------- */

static int checkExitedClients (int clientiMax, int* exited_clients){
  int x;
  PTHREAD_CALL(pthread_mutex_lock(&supermercato_mtx), "lock ExitedClients")
  x = (clientiMax != *exited_clients);
  PTHREAD_CALL(pthread_mutex_unlock(&supermercato_mtx), "unlock numeroClienti")
  return x;
}

/* ====================================================================== */
