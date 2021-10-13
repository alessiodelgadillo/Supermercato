#define MAX_BUF_SIZE 64

#include <cliente.h>
#include <cassiere.h>

typedef struct {
    int K, C, E, T, P, S1, S2, TP, Q, R;
    char logName[MAX_BUF_SIZE];
}config;

/*ritorna il path del file di configurazione
 * param argc #elementi passati al main
 * param argv array passato al main */
char* parse_cmd (int argc, char* argv[]);

/* indica la corretta chiamata dell'esegubile
 * param esegubile nome dell'esegubile
 * termina la funzione main con un errore */
void use (const char* eseguibile);
/* converte una stringa in intero
 * param s stringa da convertire
 * return intero convertito */
int isNumber(const char* s);

/* crea una struct che contiene i parametri del del file di configurazione
 * param filename path del file di configurazione
 * retur la struct che contiene i parametri indicati nel file di configurazione */
config* readConfig (const char* filename);

// scrive su file il pid del processo
void write_pid (pid_t pid);

//scrive su file il nome del file di log
void write_logName(const char* logName);

/* stampa le informazioni del cliente sul file di log
 * param cliente: struttura contenente i dati del cliente
 * param filename: nome del file di log */
void printCliente (Cliente* cliente, const char* filename);

/* stampa le informazioni del cassiere sul file di log
 * param cassiere: struttura contenente i dati del cassiere
 * param filename: nome del file di log */
 void printCassiere (Cassiere* cassiere, const char* filename);

 /* stampa le informazioni del supermercato sul file di log
  * param filename: nome del file di log
  * param numeroClienti: numero dei clienti serviti all'interno del supermercato
  * param ProdottiTotali: numero di prodotti elaborati durante l'esecuzione */
  void printSupermercato (const char* filename, size_t numeroClienti, long int ProdottiTotali);
