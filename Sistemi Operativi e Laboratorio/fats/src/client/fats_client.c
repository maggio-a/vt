/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* fats_client.c -- processo client */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/socket.h>
#include <pthread.h>

#include "../com/com_defs.h"
#include "../com/sol_com.h"
#include "../fat/fat_error.h"
#include "../fat/fat_defs.h"

#include "client_impl.h"

/* canale di comunicazione con il server */
channel_t com;

/* -- MAIN ------------------------------------------------------------------ */
int main( int argc, char **argv )
{
	int err;
	pthread_t tid;
	char *usrname, *devname;
	char *server_name = TMP SKTNAME;

	if ( argc < 2 || argc > 3 ) {
		fprintf( stderr, "Uso: %s usr [device]\n", argv[0] );
		fprintf( stderr, "  usr   \tnome utente\n" );
		fprintf( stderr, "  device\tspecifica il nome di un file system\n" );
		exit( EXIT_FAILURE );
	}

	devname = NULL;
	switch( argc ) {
	case 3:
		devname = argv[2];
	case 2:
		usrname = argv[1];
		break;
	default:
		fprintf( stderr, "argc: valore inatteso.\n" );
		exit( EXIT_FAILURE );
	}

	/* connessione al server fats */
	printf( "Connessione verso %s ", server_name );
	com = openConnection( server_name );
	if ( com < 0 ) {
		printf( "fallita\n" );
		fatserror( com );
		exit( EXIT_FAILURE );
	}
	printf( "riuscita\n" );
	
	fflush( stdout );

	/* Handshake con il server prima di avviare la conversazione */
	err = do_handshake( usrname, &devname );

	if ( err ) {
		closeConnection( com );
		exit( EXIT_FAILURE );
	}
	else {
		fprintf( stdout, "Sessione avviata per %s@%s\n", usrname, devname );
		if ( argc == 2 ) /* devname era NULL prima di chiamare do_handshake */
			free( devname );
	}

	/* crea il thread di ascolto */
	err = pthread_create( &tid, NULL, &server_listener, NULL );
	if ( err ) {
		errno = err;
		fprintf( stderr, "pthread_create (main): " );
		fatserror( STDLIBERR );
		exit( EXIT_FAILURE );
	}

	/* attiva il terminale per inviare comandi al server */ 
	handle_input(); /* ritorna solo se l'utente digita quit */

#ifdef SOL_TESTING
	/* attende la ricezione delle risposte pendenti per al massimo 2 secondi
	   evita il connection reset durante i test */
	sleep( 2 );
#endif

	/* termina server_listener */
	pthread_cancel( tid );
	pthread_join( tid, NULL );

	err = closeConnection( com );
	if ( err ) {
		fatserror( err );
		exit( EXIT_FAILURE );
	}

	exit( EXIT_SUCCESS );
}
