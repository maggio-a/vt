/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* fat_error.c -- stampa degli errori riscontrati */

#include <stdio.h>

#include "fat_defs.h"
#include "fat_error.h"

void fatserror( int err )
{
	if ( err == STDLIBERR )
		perror( NULL );
	else {
		char *string;

		switch ( err ) {
		case ENTL:
			string = "ENTL: Il nome di un elemento eccede MAX_LEN_NAME";
			break;
		case ERFCD:
			string = "ERFCD: Lettura dei metadati del file system fallita";
			break;
		case ERBD:
			string = "ERBD: Lettura di un blocco dati fallita";
			break;
		case ENSFD:
			string = "ENSFD: Il file o la directory specificata non esiste";
			break;
		case EDAEX:
			string = "EDAEX: Il file o la directory specificata esiste gia'";
			break;
		case ENMSD:
			string = "ENMSD: Spazio nel file system e' esaurito";
			break;
		case EWFCD:
			string = "EWFCD: Scrittura dei metadati del file system fallita";
			break;
		case EWBD:
			string = "EWBD: Scrittura di un blocco dati fallita";
			break;
		case EBDP:
			string = "EBDP: Il percorso specificato non e' valido";
			break;
		case ECCS:
			string = "ECCS: Creazione del socket di comunicazione fallita";
			break;
		case EDNF:
			string = "EDNF: Nessun device associato alle credenziali fornite";
			break;
		case INTSVERR:
			string = "INTSVERR: Errore interno del server";
			break;
		default:
			string = ": Errore sconosciuto";
		}

		fprintf( stderr, "Errore %s\n", string );
	}
}
