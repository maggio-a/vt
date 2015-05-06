/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* common.c -- funzioni comuni a client e server */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../fat/fat_defs.h"
#include "common.h"

/** check_path
 * verifica che la sintassi del percorso specificato sia valida, cioe' che il
 * percorso sia assoluto, e che il nome di ogni elemento non ecceda la lunghezza
 * massima consentita
 * non si occupa (ovviamente) di verificare l'effettiva esistenza del file
 * restituisce 0 se il path e' valido, oppure il codice opportuno se si
 * verifica un errore
 */
int check_path( const char* path )
{
	int n, i;
	char *copy;
	char *token;
	char *saveptr;

	/* controlla che path esista e sia assoluto */
	if ( !path || path[0] != '/' ) {
		return EBDP;
	}

	n = strlen( path );

	/* controlla che non ci siano due slash contigue */
	for ( i = 0; i < n-1; i++ )
		if ( path[i] == '/' && path[i+1] == '/' )
			return EBDP;

	/* controlla che tutti i tokens abbiano lunghezza minore di MAX_LEN_NAME */
	copy = strdup( path );
	if ( !copy )
		return STDLIBERR;

	token = strtok_r( copy, "/", &saveptr );
	while ( token != NULL ) {
		if ( strlen( token ) > MAX_LEN_NAME ) {
			free( copy );
			return ENTL;
		}
		
		token = strtok_r( NULL, "/", &saveptr );
	}
	
	free( copy );

	return 0;
}