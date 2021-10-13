#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <util.h>


/*TAKE_NUMBER e TAKE_LOG
 * rendono il codice di readConfig più leggibile */
#define TAKE_NUMBER(n,t,s)\
            if (strcmp(token, n) == 0) {\
                token = strtok_r(NULL, " ", &savePtr);\
                token[strlen(token)-1] = '\0';\
                if ((t = isNumber(token)) == -1) { \
                    fprintf(stderr, s); \
                    exit(EXIT_FAILURE); }\
                token = NULL;\
                continue; }
#define TAKE_LOG(n,t)\
            if (strcmp(token, "log") == 0) {\
                token = strtok_r(NULL, " ", &savePtr);\
                unsigned int length = strlen(token);\
                strncpy(conf->logName, token, length);\
                conf->logName[length-1] = '\0';\
                token = NULL;\
                continue;}

void use (const char* eseguibile){
    fprintf(stderr,"use: %s [-c] [config name]\n", eseguibile);
    exit(EXIT_FAILURE);
}

char* parse_cmd (int argc, char* argv[]){
    char* string;
    /*se argc == 3, allora c'è un'opzione
     *che indica il path di config */
    if (argc == 3) {
        switch (getopt(argc, argv, "c:")) {
            case 'c':
                if (optarg == 0){
                    return NULL;
                } else{
                    string = optarg;
                    return string;

                }
            default:
                return NULL;
        }
    //altrimenti ritorno il file di config predefinito
    } else if (argc == 1 ){
        string = "config.txt";
        return string;
    } else
        return NULL;
}

int isNumber(const char* s) {
    char* e = NULL;
    int val = (int) strtol(s, &e, 0);
    if (e != NULL && (*e == '\0') && val >0) return val;
    return -1;
}

config* readConfig (const char* filename){
    FILE* fPtr;
    if ((fPtr = fopen(filename, "r")) == NULL){ //apro il file di configurazione
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    char buffer[MAX_BUF_SIZE+1];
    config* conf =malloc(sizeof(config));
    memset(conf->logName, '\0', MAX_BUF_SIZE);

    while (fgets(buffer, MAX_BUF_SIZE, fPtr) != NULL) { //leggo tutto il file
        char* savePtr = NULL;
        char *token = strtok_r(buffer, "=", &savePtr);

        TAKE_NUMBER("K", conf->K,"K must be a positive not null number\n" )
        TAKE_NUMBER("C", conf->C,"C must be a positive not null number\n" )
        TAKE_NUMBER("E", conf->E,"E must be a positive not null number\n" )
        TAKE_NUMBER("T", conf->T,"T must be a positive not null number\n" )
        TAKE_NUMBER("P", conf->P,"P must be a positive not null number\n" )
        TAKE_NUMBER("S1", conf->S1,"S1 must be a positive not null number\n" )
        TAKE_NUMBER("S2", conf->S2,"S2 must be a positive not null number\n" )
        TAKE_NUMBER("TP", conf->TP,"TP must be a positive not null number\n" )
        TAKE_NUMBER("Q", conf->Q,"Q must be a positive not null number\n" )
        TAKE_NUMBER("R", conf->R,"R must be a positive not null number\n" )

        TAKE_LOG("log", conf->logName);
    }
    fclose(fPtr);

    //ulteriori controlli sui valori

    if (conf->E >= conf->C){
      fprintf(stderr, "E deve essere minore di C\n" );
      exit(EXIT_FAILURE);
    }
    if (conf->S1 > conf->K){
      fprintf(stderr, "La soglia S1 deve essere minore del numero di casse totali\n" );
      exit(EXIT_FAILURE);
    }
    if (conf->S2 > conf->C){
      fprintf(stderr, "La soglia S2 deve essere minore del massimo numero di clienti\n" );
      exit(EXIT_FAILURE);
    }
    if (conf->Q > conf->K){
      fprintf(stderr, "Il numero di casse aperte inizialmente deve essere minore del numero di casse totali\n");
      exit(EXIT_FAILURE);

    }
    return conf;
}

void write_pid (pid_t pid){
  FILE *fPtr;
  if ((fPtr = fopen("./pid.txt", "w")) == NULL){
    perror("fopen");
    exit(EXIT_FAILURE);
  }
  fprintf(fPtr, "%d\n", pid);

  fclose(fPtr);
  return;
}

void write_logName(const char* logName){
  FILE *fPtr;
  if ((fPtr = fopen("./filenameLog.txt", "w")) == NULL){
    perror("fopen");
    exit(EXIT_FAILURE);
  }
  fprintf(fPtr, "%s\n", logName);

  fclose(fPtr);
  return;
}

void printCliente (Cliente* cliente, const char* filename){
  FILE *fPtr;
  if ((fPtr = fopen(filename, "a")) == NULL){
    fprintf(stderr, "impossibile aprire file di log\n");
    exit(EXIT_FAILURE);
  }
  fprintf(fPtr, "CLIENTE %4d\t%.3lf\t%.3lf\t", cliente->clienteId, cliente->TotalTime, cliente->WaitTime);

  if (cliente->CodeCambiate <= 0) fprintf(fPtr, "n 0\t");
  else fprintf(fPtr, "y %d\t", cliente->CodeCambiate);

  fprintf(fPtr, "%3d\n",cliente->products);
  fclose(fPtr);
}

void printCassiere (Cassiere* cassiere, const char* filename){
  FILE *fPtr;
  if ((fPtr = fopen(filename, "a")) == NULL){
    fprintf(stderr, "impossibile aprire file di log\n");
    exit(EXIT_FAILURE);
  }

  fprintf(fPtr, "CASSA %2d\t%3d\t%5d\t%3d\n", cassiere->cassaId, cassiere->ClientiServiti, cassiere->ProdottiElaborati, cassiere->Chiusura);
  while(cassiere->TotalTime_head != NULL){
    double t_apertura = pop(&(cassiere->TotalTime_head), &(cassiere->TotalTime_tail));
    fprintf(fPtr, "APERTURA\t%.3lf\n", t_apertura);
  }
  while(cassiere->ServiceTime_head != NULL){
    double t_servizio = pop(&(cassiere->ServiceTime_head), &(cassiere->ServiceTime_tail));
    fprintf(fPtr, "SERVIZIO\t%.3lf\n", t_servizio);
  }
  fprintf(fPtr, "FINE\n");
  fclose(fPtr);
}

void printSupermercato (const char* filename, size_t numeroClienti, long int ProdottiTotali){
  FILE *fPtr;
  if ((fPtr = fopen(filename, "a")) == NULL){
    fprintf(stderr, "impossibile aprire file di log\n");
    exit(EXIT_FAILURE);
  }
  fprintf(fPtr, "SUPERMERCATO\t%ld\t%ld\n", numeroClienti, ProdottiTotali );
  fclose(fPtr);
}
