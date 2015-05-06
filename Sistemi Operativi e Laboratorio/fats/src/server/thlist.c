/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* thlist.c -- implementazione delle operazioni sulla lista */

#include <pthread.h>

#include "thlist.h"

/* inizializza la lista */
void list_init( thread_list *l )
{
	l->head = NULL;
	l->n_items = 0;
}

/* inserisce un nuovo thread in testa alla lista */
void list_push( thread_list *l, th_info *el )
{
	el->next = l->head;
	l->head = el;
	l->n_items++;
}

/** list_get - ricerca di un thread id
 * restituisce un puntatore al th_info cercato se esiste, oppure NULL se non
 * trova il tid specificato
 */
th_info *list_get( thread_list *l, pthread_t tid )
{
	th_info *el;

	for ( el = l->head; el != NULL; el = el->next )
		if ( pthread_equal( el->tid, tid ) )
			break;

	return el;
}

/** list_remove - estrazione di un elemento dalla lista
 * estrate il th_info cercato e restituisce un ptr ad esso se esiste, oppure
 * NULL se non trova il tid specificato
 * modifica la lista
 */
th_info *list_remove( thread_list *l, pthread_t tid )
{
	th_info *this, *prev;

	for ( prev = NULL, this = l->head; this != NULL;
		  prev = this, this = this->next )
	{
		if ( pthread_equal( this->tid, tid) ) {
			/* elemento trovato, il precedente elemento deve puntare al
			   successivo */
			prev ? ( prev->next = this->next ) : ( l->head = this->next );
			/* se prev == NULL => this == l->head e aggiorna la testa */
			l->n_items--;
			return this;
		}
	}

	return NULL;
}
