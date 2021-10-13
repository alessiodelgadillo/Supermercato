#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <util.h>
#include <Bqueue.h>
#include <cassiere.h>
#include <cliente.h>
#include <pthread.h>
#include <direttore.h>
#include <time.h>
#include <supermercato.h>
#include <signal.h>
#include <signal_handler.h>
#include <errno.h>

#define PTHREAD_CALL(p,s)\
                if (p != 0){\
                perror(s);\
                exit(EXIT_FAILURE);}

Stato_Supermercato StatoSupermercato = APERTO;
pthread_mutex_t supermercato_mtx = PTHREAD_MUTEX_INITIALIZER;   //mutex per accedere allo stato del supermercato

int main (int argc, char* argv[]) {

    //imposto la maschera dei segnali
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGQUIT);
    PTHREAD_CALL(pthread_sigmask(SIG_SETMASK, &mask, NULL), "sigmask supermercato")

    char* config_name;
    //parsing della linea di comando
    if ((config_name = parse_cmd(argc, argv)) == NULL){
        use(argv[0]);
    }

    //leggo il file di configurazione
    config* conf = readConfig(config_name);

    //scrivo su file il pid del processo e il nome del file di log
    pid_t pid = getpid();
    write_pid(pid);
    write_logName(conf->logName);

    //se il file log esiste, lo elimino
    if (unlink(conf->logName) == -1 && errno != ENOENT){
      printf("%d\n", errno);
      exit(EXIT_FAILURE);
    }

    //dichiarazione delle strutture
    Cliente clienti[conf->C];
    Cassiere cassieri[conf->K];
    Direttore direttore[1];

    //dichiarazione degli identificatori dei thread
    pthread_t thClienti[conf->C];
    pthread_t thCassieri[conf->K];
    pthread_t thDirettore;
    pthread_t thSignal_handler;

    //dichiarazione e inizializzazione delle variabili mutex e di condizione
    pthread_mutex_t casse_mtx;
    pthread_cond_t casse_close;
    pthread_cond_t casse_notifica;

    pthread_mutex_t auth_mtx;
    pthread_cond_t auth_cond;

    pthread_mutex_t cliente_mtx[conf->C];
    pthread_cond_t cliente_cond[conf->C];

    pthread_cond_t exit_cond;


    PTHREAD_CALL(pthread_mutex_init(&casse_mtx, NULL), "init casse_mtx")
    PTHREAD_CALL(pthread_cond_init(&casse_close, NULL), "init casse_close")
    PTHREAD_CALL(pthread_cond_init(&casse_notifica, NULL), "init casse_notifica")

    PTHREAD_CALL(pthread_mutex_init(&auth_mtx, NULL), "init auth_mtx")
    PTHREAD_CALL(pthread_cond_init(&auth_cond, NULL), "init auth_cond")

    for (size_t i = 0; i < conf->C; i++) {
      PTHREAD_CALL(pthread_mutex_init(&cliente_mtx[i], NULL), "init cliente_mtx")
      PTHREAD_CALL(pthread_cond_init(&cliente_cond[i], NULL), "init cliente_cond")
    }

    PTHREAD_CALL(pthread_cond_init(&exit_cond, NULL), "init exit_cond")

    //creazione del signal handler
    pthread_attr_t attr_handler;
    PTHREAD_CALL(pthread_attr_init(&attr_handler), "pthread_attr_init failed")
    PTHREAD_CALL(pthread_attr_setdetachstate(&attr_handler, PTHREAD_CREATE_DETACHED), "set detach state")
    PTHREAD_CALL(pthread_create(&thSignal_handler, &attr_handler, signalHandler, NULL ), "pthread_create handler")

    //dichiarazione e inizializzazione dell'insieme delle casse
    BQueue_t *casse[conf->K];
    Stato_Cassa statoCasse[conf->K];
    int CassaLen[conf->K];

    for (size_t i = 0; i < conf->K; i++) {
      casse[i] = init(1000);
      statoCasse[i] = i < conf->Q ? APERTA : CHIUSA;
      CassaLen[i] = NOTIFY_NOT_EXISTS;
    }

    for (size_t i = 0; i < conf->K; i++) {
      cassieri[i].cassaId = i;
      cassieri[i].cassa = casse[i];
      cassieri[i].statoCassa = &statoCasse[i];
      cassieri[i].mtx = &casse_mtx;
      cassieri[i].close = &casse_close;
      cassieri[i].notifica = &casse_notifica;
      cassieri[i].dim_cassa = &CassaLen[i];
      unsigned seed = (i+1) * time(NULL);
      cassieri[i].time = rand_r(&seed) % (MAX_TS - MIN_TS + 1) + MIN_TS;
      cassieri[i].time_products = conf->TP;
      cassieri[i].time_notifica = conf->R;
    }

    //inizializzazione e creazione del direttore
    int casseAperte = conf->Q;
    char auth[conf->C];
    memset(auth, 0 , conf->C);

    int exited_clients = 0;

    direttore->casse_mtx = &casse_mtx;
    direttore->casse_cond = &casse_close;
    direttore->statoCasse = statoCasse;
    direttore->casse_aperte = &casseAperte;
    direttore->casseTot = conf->K;
    direttore->dim_casse = CassaLen;
    direttore->notifiche = &casse_notifica;
    direttore->auth_mtx = &auth_mtx;
    direttore->auth_cond = &auth_cond;
    direttore->authorization = auth;
    direttore->clientiMax = conf->C;
    direttore->exited_clients = &exited_clients;
    direttore->S1 = conf->S1;
    direttore->S2 = conf->S2;

    PTHREAD_CALL(pthread_create(&thDirettore, NULL, direttore_fun, &direttore), "pthread_create direttore")

    //creazione dei cassieri
    for (size_t i = 0; i < conf->K; i++) {
      PTHREAD_CALL(pthread_create(&thCassieri[i], NULL, cassiere, &cassieri[i]), "pthread_create cassieri" )
    }

    //inizializzazione e creazione dei clienti
    size_t c = 0;
    for (c = 0; c < conf->C; c++) {
      clienti[c].clienteId = c;
      clienti[c].stato = SPESA;
      clienti[c].seed = (c+1) * time(NULL);
      clienti[c].products = rand_r(&clienti[c].seed) % (conf->P + 1);
      clienti[c].timeToShop = rand_r(&clienti[c].seed) % (conf->T - 10 + 1) + 10;
      clienti[c].casseTot = conf->K;
      clienti[c].casse_aperte = &casseAperte;
      clienti[c].statoCassa = statoCasse;
      clienti[c].casse = casse;
      clienti[c].casse_mtx = &casse_mtx;
      clienti[c].mtx = &cliente_mtx[c];
      clienti[c].cond_cliente = &cliente_cond[c];
      clienti[c].auth_mtx = &auth_mtx;
      clienti[c].auth_cond = &auth_cond;
      clienti[c].authorization = &auth[c];
      clienti[c].exit_mtx = &supermercato_mtx;
      clienti[c].exit_cond = &exit_cond;
      clienti[c].exited_clients = &exited_clients;
      clienti[c].isExited = 0;

      PTHREAD_CALL(pthread_create(&thClienti[c], NULL, cliente, &clienti[c]), "pthread_create clienti")
    }

    while (1) {

      //controllo lo stato del supermercato
      PTHREAD_CALL(pthread_mutex_lock(&supermercato_mtx), "supermercato lock")
      if (StatoSupermercato != APERTO){
        PTHREAD_CALL(pthread_mutex_unlock(&supermercato_mtx), "supermercato unlock")
        break;
      }

      //finché non sono usciti almeno E clienti mi metto in attesa
      while (exited_clients < conf->E) PTHREAD_CALL(pthread_cond_wait(&exit_cond, &supermercato_mtx), "wait exit")

      /* scrivo sul file di log le statistiche dei clienti usciti,
         li rinizializzo e li faccio rientrare */
      for (size_t i = 0; i < conf->C; i++) {
        if (clienti[i].isExited){

          PTHREAD_CALL(pthread_join(thClienti[i], NULL), "pthread_join clienti")

          printCliente(&clienti[i], conf->logName);

          clienti[i].clienteId = c++;
          clienti[i].stato = SPESA;
          clienti[i].seed = c * time(NULL);
          clienti[i].products = rand_r(&clienti[i].seed) % (conf->P + 1);
          clienti[i].timeToShop = rand_r(&clienti[i].seed) % (conf->T - 10 + 1) + 10;
          clienti[i].isExited = 0;

          PTHREAD_CALL(pthread_create(&thClienti[i], NULL, cliente, &clienti[i]), "pthread_create clienti")

          if (--exited_clients == 0) break;
        }
      }
      PTHREAD_CALL(pthread_mutex_unlock(&supermercato_mtx), "supermercato unlock")
    }

    //i clienti terminano
    for (size_t i = 0; i < conf->C; i++) {
      PTHREAD_CALL(pthread_join(thClienti[i], NULL), "pthread_join clienti")
      //scrivo sul file di log le statistiche dei clienti
      printCliente(&clienti[i], conf->logName);

    }
    //i cassieri terminano
    int ProdottiAcquistati = 0;
    int ClientiServiti = 0;
    for (size_t i = 0; i < conf->K; i++) {
      PTHREAD_CALL(pthread_join(thCassieri[i], NULL), "pthread_join cassieri")

      //conto i prodotti acquistati e i clienti serviti
      ProdottiAcquistati += cassieri[i].ProdottiElaborati;
      ClientiServiti += cassieri[i].ClientiServiti;

      //scrivo sul file di log le statistiche dei cassieri
      printCassiere(&cassieri[i], conf->logName);
    }

    //il direttore termina
    PTHREAD_CALL(pthread_join(thDirettore, NULL), "pthread_join direttore")

    //dealloco le code delle casse
    for (size_t i = 0; i < conf->K; i++) {
      deinit(casse[i]);
    }

    //scrivo sul file di log le statistiche del supermercato
    printSupermercato(conf->logName, ClientiServiti, ProdottiAcquistati);


    free(conf);

    return 0;
}
