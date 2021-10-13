#define _POSIX_C_SOURCE 200112L
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <signal_handler.h>

#define PTHREAD_CALL(p,s)\
                if (p != 0){\
                perror(s);\
                exit(EXIT_FAILURE);}

void* signalHandler (void* arg){

  //imposto la maschera dei segnali
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGHUP);
  sigaddset(&mask, SIGQUIT);
  PTHREAD_CALL(pthread_sigmask(SIG_SETMASK, &mask, NULL), "sigmask handler")
  int sig;

  //mi metto in attesa del segnale
  sigwait(&mask, &sig);

  if (sig == SIGHUP){
    PTHREAD_CALL(pthread_mutex_lock(&supermercato_mtx), "lock handler")
    StatoSupermercato = IN_CHIUSURA;
    PTHREAD_CALL(pthread_mutex_unlock(&supermercato_mtx), "unlock handler")
  }
  else {
    PTHREAD_CALL(pthread_mutex_lock(&supermercato_mtx), "lock handler")
    StatoSupermercato = CHIUSO;
    PTHREAD_CALL(pthread_mutex_unlock(&supermercato_mtx), "unlock handler")
  }

  return NULL;

}
