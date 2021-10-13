# Supermercato

## Introduzione

Il progetto contiene le directory `src`, `include`, `lib` e `bin`, due file di configurazione, un Makefile e uno script Bash per effettuare il parsing del file di log prodotto e produrne un sunto sullo standard otuput.
Durante l'esecuzione vengono prodotti anche altri due file utili allo script: `pid.txt` e `filenameLog.txt` che contengono rispettivamente l'identificatore del processo supermercato e il nome del file di log.

Il codice è conforme allo standard `POSIX 2001`.

### File di configurazione

Il formato del file di configurazione è caratterizzato da un assegnamento `NOME=VALORE`, dove il primo membro identifica il parametro di configurazione.
Non sono consentiti spazi bianchi prima e dopo il carattere '='.

Di default il programma prende come file di configurazione il file di testo `config.txt`.

## Thread Supermercato

All'inizio dell'esecuzione del processo, il **thread supermercato** (thread main) *legge i parametri di configurazione* attraverso la funzione `readConfig` che restituisce un puntatore a una `struct` contenente i valori specificati; all'interno della funzione vengono anche effettuati i dovuti controlli di correttezza come la non negatività dei valori.

In seguito, il *supermercato si occupa di allocare e inizializzare le strutture necessarie* all'esecuzione dell'intero processo e di creare il **Signal Handler**[^1], il **Direttore**, i **Cassieri** e i **Clienti**; dopodiché, fino al cambiamento dello stato del supermercato causato dalla ricezione di un segnale `SIGQUIT` o `SIGHUP`, si limita ad aspettare che siano usciti *E* clienti: ne salva le statistiche, ne inizializza nuovamente i valori e li fa rientrare come nuovi clienti.
All'atto del cambiamento dello stato attende la terminazione dei vari thread per scriverne le statistiche nel file di log e libera la memoria allocata precedentemente.

[^1]: *Il signal handler è un thread detached*.

## Thread Signal Handler

All'inizio della sua esecuzione, come tutti gli altri thread, maschera i segnali `SIGHUP` e `SIGQUIT`, successivamente si sospende in attesa sincrona di uno dei due segnali tramite la funzione `sigwait`.
In alternativa si sarebbe potuto usare un gestore asincrono, ma avrebbe escluso l'uso di funzioni non signal-safe.

Se viene ricevuto il segnale `SIGHUP`, l'handler imposta lo stato del supermercato a IN_CHIUSURA, altrimenti se viene ricevuto `SIGQUIT` lo stato viene impostato a CHIUSO.

## Thread Direttore

Il **direttore** *ha due compiti*: gestire la chiusura/apertura della casse e dare l'autorizzazione per uscire ai clienti con 0 prodotti.
La gestione delle casse è molto semplice: se è necessario aprire una cassa allora viene aperta la prima cassa chiusa trovata, se invece è necessario chiuderla allora si chiude la *S1*-esima (a meno che la prima cassa non contribuisca a raggiungere la soglia *S1*, in tal caso viene chiusa quella, altrimenti non potrebbe mai chiudere).

Nel frattempo che il thread esegue questi due compiti, *controlla anche lo stato del supermercato*:

- se è IN_CHIUSURA continua a gestire le casse e rilasciare autorizzazioni finché non sono usciti tutti i clienti, a quel punto imposta lo stato delle casse a TERMINATA;
- se è CHIUSO, informa i clienti che l'autorizzazione non è più necessaria tramite la variabile *volatile* `need_auth` e imposta lo stato delle casse a TERMINATA.

## Thread Cassiere

Durante la propria esecuzione il cassiere *controlla lo stato della propria cassa*:

- se è CHIUSA libera la coda e si mette in attesa di essere riaperto dal direttore
- se è TERMINATA, libera la coda e il thread termina
- se è APERTA serve il cliente in testa alla coda e manda una notifica al direttore contenente il numero di clienti in coda.

Un aspetto interessante del thread è la gestione del tempo di servizio e ogni quanto il cassiere manda la notifica: infatti, se il tempo di servizio è maggiore dell'intervallo di notifica, allora viene "spezzato" cosicché sia rispettato sia l'intervallo di notifica che il tempo di servizio.
Un'alternativa a questa gestione sarebbe quella di usare un thread di supporto per ogni cassiere, ma ciò implicherebbe gestire altri *K* thread all'interno del processo.

## Thread Cliente

All'inizio della propria esecuzione, il cliente esegue un'attesa passiva di *t* millisecondi, 10 < *t* < *T*, che simula il tempo necessario per fare la spesa.
Al termine di tale intervallo controlla il numero di prodotti acquistati, se questo è strettamente maggiore di zero sceglie in modo casuale una cassa aperta, vi si mette in fila ed esegue un'attesa passiva dalla quale può essere risvegliato solo dal cassiere in due casi:

- è stato servito;
- la cassa è stata chiusa e quindi deve cambiare fila.

Nel primo caso il cliente termina avvertendo il supermercato della sua uscita, mentre nel secondo sceglie nuovamente una coda, si rimette in fila e di conseguenza in attesa.

Nel caso in cui non trovi nessuna cassa aperta, allora termina (se non ci sono casse aperte vuol dire che è stato ricevuto il segnale di `SIGQUIT`).

Se invece non sono stati acquistati prodotti, chiede l'autorizzazione al direttore (se è necessaria) e si mette in attesa di riceverla; una volta ricevuto il permesso, termina avvertendo il supermercato della sua uscita.

## File di Log e analisi.sh

Il formato di log è un testo semplice i cui campi sono separati da spazi e tab.
La generazione del file è un compito del thread supermercato: le statistiche del cliente e del cassiere vengono salvate sul file di log ogniqualvolta vi è una `pthread_join`, quindi anche durante il "riciclo" dei thread clienti da parte del supermercato.
La scrittura avviene in modalità *append* e per questo, dopo la `readConfig`, il thread supermercato esegue la funzione `unlink` per eliminare l'eventuale log di una precedente esecuzione.
Contemporaneamente alla fase di stampa dei cassieri vengono anche deallocate le code usate da quest'ultimi per salvare i vari tempi di apertura e di servizio.

Lo script Bash analisi attende la terminazione del processo supermercato verificando il risultato del comando `ps` ogni secondo.
Quando il processo è effettivamente terminato, legge il file prodotto usando `read -r` e quindi tokenizza ogni riga prendendo come delimitatore gli spazi e i tab.
Dopo aver letto una riga esegue un pattern matching usando la pipe `grep | wc -l` per capire di quale riga si tratta.
Tutte le operazioni in virgola mobile sono eseguite dalla calcolatrice `bc`.

## Esecuzione

Per eseguire il test è necessario eseguire `make` e dopo `make test`.
In alternativa è possibile eseguire il programma con:

```bash
$ ./bin/supermercato
$ ./bin/supermercato -c <file di configurazione>
```

Per lo script di analisi è necessario eseguire

```bash
$ ./analisi.sh pid.txt filenameLog.txt
```

## Note finali
1. Il cliente e il cassiere sono all'oscuro dello stato del supermercato: vi hanno accesso solo il direttore, il supermercato e ovviamente il Signal Handler che è l'unico a modificarlo.
2. La funzione usata dal cliente per "fare" la spesa e quella del cassiere per "servire" il cliente e aspettare di mandare la notifica è la stessa: è una `nanosleep` i cui campi `timespec` sono adattati all'uso di un tempo misurato in millisecondi invece che in secondi e nanosecondi.
3. La funzione `dequeue`, usata dal cassiere, non prevede l'attesa passiva se la coda dovesse essere vuota: in tal caso la funzione restituisce NULL e sarà il cassiere a eseguire un'attesa passiva per poi mandare la notifica al direttore; in questo modo si dà una maggiore priorità all'invio della notifica.
