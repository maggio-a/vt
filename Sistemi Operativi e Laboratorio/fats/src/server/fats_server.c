/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* fats_server.c -- processo server */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include "../fat/fat.h"
#include "../fat/fat_error.h"
#include "../com/com_defs.h"
#include "../com/sol_com.h"
#include "server_impl.h"
#include "thlist.h"

#define LISTEN_QUEUE 20

/* -- SOCKET ---------------------------------------------------------------- */
char *server_name = TMP SKTNAME;
serverChannel_t server_c;

/* -- TABELLA DEI FILE SYSTEM ----------------------------------------------- */
struct fstab_st fstab;

/* -- GESTIONE THREADS ------------------------------------------------------ */
/* lista dei threads attivati dal main */
thread_list tl;
/* mutex per gli accessi alla lista */
pthread_mutex_t list_mutex = PTHREAD_MUTEX_INITIALIZER;
/* tid del main thread */
pthread_t main_thread;

/* -- CLEANUP --------------------------------------------------------------- */
void close_sv( void *arg ) {
	int err;
	extern serverChannel_t server_c;

	err = closeSocket( server_c);
	if ( err ) {
		fatserror( err );
	}

	unlink( server_name );
}

/* -- MAIN ------------------------------------------------------------------ */
int main( int argc, char **argv )
{
	extern struct fstab_st fstab;

	int err, state, i;          /* Misc */
	channel_t client_c;         /* Canale di comunicazione client-server */
	pthread_t tid;
	sigset_t set;

	if ( argc != 2 ) {
		fprintf(stderr, "Uso: %s <dir>\n", argv[0] );
		fprintf(stderr, "  dir\tcartella di cui indicizzare i file system\n" );
		exit( EXIT_FAILURE );
	}
	
	/* inizializza la tabella dei file */
	fstab.numfiles = 0;
	fstab.tabsize = 5;
	fstab.fsystem = malloc( fstab.tabsize*sizeof(struct file_handle) );

	if ( !fstab.fsystem ) {
		perror( "malloc (main)" );
		exit( EXIT_FAILURE );
	}

	/* indicizza i fs contenuti nella cartella specificata */
	printf( "Indicizzazione dei device in %s in corso...\n", argv[1] );
	index_devices( argv[1] );

	if ( fstab.numfiles == 0 ) {
		printf( "Nessun file system trovato - uscita.\n" );
		free( fstab.fsystem );
		exit( 0 );
	}

	/* stampa qualche info sui device indicizzati */
	printf( "==========================================================\n" );
	printf( " %-15s %-16s %-16s%s\n",
		    "NOME", "PROPRIETARIO", "BLOCCHI", "BLKSIZE" );
	for ( i = 0; i < fstab.numfiles; i++ )
		printf( " %-12.12s%3s %-16s %-16u%d B\n",
		        fstab.fsystem[i].name,
		        (strlen(fstab.fsystem[i].name) > 12 ) ? "..." : "",
		        fstab.fsystem[i].fsctrl.b_sector.usr,
		        fstab.fsystem[i].fsctrl.b_sector.num_block,
		        fstab.fsystem[i].fsctrl.b_sector.block_size );
	printf( "==========================================================\n" );

	/* salva l'ID del main */
	main_thread = pthread_self();

	/* inizializza la lista dei threads */
	list_init( &tl );

	/* apre il socket di ascolto */
	fprintf( stdout, "Apertura del socket %s\n", server_name );
	server_c = createServerChannel( server_name );
	if ( server_c < 0 ) {
		perror( "createServerChannel (main)" );
		return EXIT_FAILURE;
	}

	pthread_cleanup_push( close_sv, NULL );

	/* handling dei segnali - lancia un thread per gestire SIGINT e SIGTERM */
	sigemptyset( &set );
	sigaddset( &set, SIGINT );
	sigaddset( &set, SIGTERM );

	/* blocca SIGINT e SIGTERM in tutti i thread */
	err = pthread_sigmask( SIG_SETMASK, &set, NULL );
	if ( err ) {
		errno = err;
		perror( "pthread_sigmask" );
		exit( EXIT_FAILURE );
	}

	/* avvia il servizio di cancellazione che intercetta SIGINT e SIGTERM */
	err = pthread_create( &tid, NULL, &sig_handler, NULL );
	if ( err ) {
		errno = err;
		perror( "pthread_create (main)" );
		exit( EXIT_FAILURE );
	} else
		if ( pthread_detach( tid ) )
			fprintf(stderr, "Warning: detach di sig_handler fallita\n");

	if ( listen( server_c, LISTEN_QUEUE ) ) {
		perror( "listen (main)" );
		return EXIT_FAILURE;
	}
	
	fprintf( stdout, " In ascolto... \n" );

	/* Loop - gestione delle connessioni in ingresso */
	for (;;) {

		int i;
		th_info *info = NULL;

		client_c = acceptConnection( server_c );

		/* il thread non puo' essere cancellato mentre aggiorna la lista */
		pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &state );

		if ( client_c < 0 ) {
			fprintf( stderr, "acceptConnection (main):" );
			fatserror( client_c );
			break; /* interrompe l'ascolto */
		}
		
		printf( "Richiesta di connessione accettata\n" );

		/* effettua l'handshake */
		i = server_handshake( client_c );

		if ( i < 0 ) { /* richiesta non soddisfattibile */
			printf( "Handshake fallito\n" );
			closeConnection( client_c );
		}
		else { /* fstab.fsystem[i] e' il file system richiesto dall'utente */
			info = malloc( sizeof(th_info) );

			if ( !info ) {
				perror( "malloc (main)" );
				if ( closeConnection( client_c ) )
					perror( "closeConnection (main):" );
				break;
			}

			info->client_c = client_c;
			info->handle = &( fstab.fsystem[i] );

			pthread_mutex_lock( &list_mutex ); /* blocca la lista */
		
			err = pthread_create( &tid, NULL, &client_handler, (void *) info );
			if ( err ) {
				errno = err;
				perror( "main (pthread_create)" );
				free( info );
				break;
			}
			else {
			   /* nota: nonostante l'inserimento sia posticipato, le operazioni
			      sulla lista sono effettuate acquisendo la lock che a questo
			      punto non e' stata rilasciata, quindi nessuno puo' "vedere" un
			      tid non inizializzato scorrendo la lista usando i mutex */
				list_push( &tl, info );
				info->tid = tid;
			}
			
			pthread_mutex_unlock( &list_mutex );
		}

		pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &state );

	}

	pthread_cleanup_pop( 0 );

	return 0;
}
