#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include <supermercato.h>

extern Stato_Supermercato StatoSupermercato;
extern pthread_mutex_t supermercato_mtx;

void* signalHandler (void* arg);

#endif
