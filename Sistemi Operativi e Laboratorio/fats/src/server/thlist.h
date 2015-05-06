/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

#ifndef THLIST_H
#define THLIST_H

#include <stdio.h>
#include <pthread.h>

#include "../fat/fat.h"
#include "../com/com_defs.h"

/* -- LISTA DEI THREAD ATTIVI ----------------------------------------------- */
/* informazioni relative ad un client handler */
struct th_info_st {
	pthread_t tid;              /* ID del thread                           */
	channel_t client_c;         /* canale di comunicazione verso il client */
	struct file_handle *handle; /* riferimento al file system richiesto    */
	struct th_info_st *next;    /* prossima entry della lista              */
};

/* lista */
struct thread_list_st {
	unsigned int n_items;
	struct th_info_st *head;
};

typedef struct th_info_st th_info;
typedef struct thread_list_st thread_list;

void     list_init( thread_list *l );
void     list_push( thread_list *l, th_info *el );
th_info *list_get( thread_list *l, pthread_t tid );
th_info *list_remove( thread_list *l, pthread_t tid );

#endif /* THLIST_H */
