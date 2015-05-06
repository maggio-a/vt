/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* server_impl.c -- funzioni del server */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>

#include "../fat/fat.h"
#include "../fat/fat_defs.h"
#include "../fat/fat_error.h"
#include "../fat/load_fat.h"
#include "../fat/ops_fat.h"
#include "../com/sol_com.h"
#include "../com/com_defs.h"
#include "thlist.h"

#include "server_impl.h"

/** server_handshake
 * Handshake a 2 vie tra client e server
 *   - il client invia MSG_CONNECT specificando nome utente [e device]
 *   - il server cerca nella tabella un file system che risponda alla query
 *     ( nome utente [, device] )
 *   - se trova un file system risponde MSG_OK e notifica il nome del
 *     file system selezionato
 *   - altrimenti risponde MSG_ERROR e notifica il codice EDNF
 *
 * restituisce l'indice della tabella dei files che corrisponde alla
 * entry richiesta, -1 se ci sono problemi (entry non trovata o errore)
 */
int server_handshake( channel_t com )
{
	message_t msg;
	int status;
	int i;

	extern struct fstab_st fstab;

	memset( &msg, 0x00, sizeof( message_t ) );
	i = -1;

	status = receiveMessage( com, &msg );

	/* calcola la risposta */
	if ( status == SEOF || status < 0 ) {
		return -1;
	}
	else if ( msg.type != MSG_CONNECT) { /* inatteso */
		fprintf( stderr, "Ricevuto un messaggio inatteso\n" );
		if ( msg.buffer )
			free( msg.buffer );
		return -1;
	}
	else {
		/* effettua la ricerca del fs richiesto e scrive la risposta in msg */
		i = lookup_request( &msg );

		if ( msg.buffer )
			free( msg.buffer );

		if ( i < 0 ) {
			/* device non trovato */
			i = EDNF;
			msg.type = MSG_ERROR;
			msg.length = sizeof( int );
			msg.buffer = malloc( sizeof( int ) );
			if ( !msg.buffer ) {
				perror( "malloc (server_handshake)" );
				return -1;
			}
			memcpy( msg.buffer, &i, sizeof( int ) );
		}
		else { /* OK, invia il nome del device su cui il client opera */
			msg.type = MSG_OK;
			msg.length = strlen( fstab.fsystem[i].name ) + 1;
			msg.buffer = strdup( fstab.fsystem[i].name );
			if ( !msg.buffer ) {
				perror( "malloc (server_handshake)" );
				return -1;
			}
		}
	}

	status = sendMessage( com, &msg );
	free( msg.buffer );

	return i;
}

/** lookup_request
 * ricerca il device corrispondente alla richiesta se esiste
 * restituisce l'indice della tabella se lo trova, -1 se non trovato
 */
int lookup_request( message_t *msg )
{
	size_t dim;
	int index, i;
	char *usr_req, *device_req;

	extern struct fstab_st fstab;

	usr_req = msg->buffer;
	dim = strlen( usr_req ) + 1;

	if ( msg->length > dim )
		device_req = msg->buffer + dim; /* punta alla seconda stringa */
	else
		device_req = NULL;

	index = -1;
	/* cerca il device specificato */
	for ( i = 0; i < fstab.numfiles; i++ ) {
		if ( (device_req ? !strcmp( fstab.fsystem[i].name, device_req ) : 1) &&
		     !strcmp( fstab.fsystem[i].fsctrl.b_sector.usr, usr_req ) )
		{
			index = i;
			break;
		}
	}

	return index;
}

/* -- TABELLA FS ------------------------------------------------------------ */
/* -------------------------------------------------------------------------- */

/** add_to_index
 * prova a montare il file fs_path, se ha successo compila il file handle
 * per la nuova entry
 * name contiene il nome del device, fs_path il percorso completo
 */
void add_to_index( const char *fs_path, const char *name )
{
	int i;
	int e;

	FILE *fs;
	struct fat_ctrl fsctrl;

	extern struct fstab_st fstab;

	fs = fopen( fs_path, "r+" );
	if ( !fs ) {
		fprintf( stderr, "%s tralasciato - ", fs_path );
		perror( NULL );
		return;
	}

	/* mounting */
	fsctrl.fat_ptr = NULL;
	e = mount_fat( fs, &fsctrl );
	if ( e ) { /* non ha caricato il file system */
		fclose( fs ); /* chiude il vecchio puntatore */
		if ( fsctrl.fat_ptr )
			free( fsctrl.fat_ptr );
		return;
	}
	
	/* se alla fine della tabella, alloca nuovo spazio */
	if ( fstab.numfiles == fstab.tabsize ) {
		fstab.tabsize += 5;
		fstab.fsystem = realloc( fstab.fsystem,
			                     fstab.tabsize*sizeof(struct file_handle) );
		if ( !fstab.fsystem ) {
			perror( "malloc (add_to_index)" );
			exit( EXIT_FAILURE );
		}
	}

	/* compila la entry nella tabella */
	i = fstab.numfiles++;
	fstab.fsystem[i].fs = fs;
	fstab.fsystem[i].fsctrl = fsctrl;

	/* copia il nome del device */
	fstab.fsystem[i].name = strdup( name );
	if ( !fstab.fsystem[i].name ) {
		perror( "strdup ( add_to_index)" );
		exit( EXIT_FAILURE );
	}

	/* inizializza le strutture dati per il supporto alla gestione della
	   concorrenza */
	fstab.fsystem[i].num_readers = 0;
	pthread_mutex_init( &( fstab.fsystem[i].num_mutex ), NULL );
	pthread_mutex_init( &( fstab.fsystem[i].write_mutex ), NULL );
}

/** index_devices
 * Prova ad eseguire il mounting di ogni file contenuto in dir_path
 */
int index_devices( const char *dir_path )
{
	size_t length;
	DIR *dev_dir;
	struct dirent *entry;
	struct stat stat_buffer;
	char full_name[256];

	dev_dir = opendir( dir_path );
	if ( !dev_dir ) {
		perror( "opendir (index_devices)" );
		exit( EXIT_FAILURE );
	}

	length = strlen( dir_path );
	while ( ( entry = readdir( dev_dir ) ) != NULL ) {
		if ( !strcmp( entry->d_name, "." ) || !strcmp( entry->d_name, ".." ) )
			continue;

		/* crea il percorso completo */
		if ( length + strlen( entry->d_name ) + 2 > 256 )
			continue; /* tralascia se il nome e' troppo lungo */

		/* sicuramente non si puo' uscire dall'array */
		strcpy( full_name, dir_path );
		if ( full_name[length-1] != '/' )
			strcat( full_name, "/" );
		strcat( full_name, entry->d_name );

		/* stat */
		if ( stat( full_name, &stat_buffer ) ) {
			perror( "stat (index_devices)" );
			exit( EXIT_FAILURE );
		}

		/* se e' un file regolare lo aggiunge all'indice */
		if ( S_ISREG( stat_buffer.st_mode ) )
			add_to_index( full_name, entry->d_name ); /* non fa nulla se non e'
			un fs valido */
	}

	return closedir( dev_dir );
}

/* -- THREADS --------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/** client_handler
 * gestione dell'interazione con il client
 * sostanzialmente un ciclo infinito
 *   while (richiesta in ingresso )
 *     gestisci la richiesta
 *     
 * interrompe unilateralmente la conversazione se:
 *   - il thread viene cancellato (il processo riceve SIGINT o SIGTERM)
 *   - non riesce a comunicare con il client (fallisce l'invio di una risposta
 *     o si verifica un errore nelle chiamate effettuate che impedisce di
 *     compilare la risposta
 */
void *client_handler( void *arg )
{
	int io, err;   /* misc */
	channel_t com;
	message_t msg;

	int state;     /* stato cancellazione */
	th_info *info; /* parametri del thread: socket, file system da usare etc. */

	info = arg;

	/* installa un cleanup handler che si occupa di chiudere la connessione con
	   il client e deallocare la entry del thread dalla lista.
	   lo standard posix prescrive che ad ogni invocazione di pthread_exit i
	   cleanup handler siano estratti dallo stack ed eseguiti, eseguendo
	   implicitamente le operazioni di cleanup */
	pthread_cleanup_push( cl_cleanup, info );
	
	com = info->client_c;
	memset( &msg, 0x00, sizeof( message_t ) );

	while ( (io = receiveMessage( com, &msg )) != SEOF ) {

		if ( io < 0 ) {
			fprintf( stderr, "client_handler (receiveMessage): " );
			fatserror( io );
			pthread_detach( pthread_self() );
			pthread_exit( NULL );
		}

		/* il thread non puo' essere cancellato mentre lavora sul file system */
		pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &state );

		/* Esecuzione del comando */
		err = execute( info->handle, com, &msg );

		if ( msg.length )
			free( msg.buffer );

		pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &state );

		/* se qualcosa e' andato storto durante l'esecuzione del comando
		   e non e' stato possibile notificare al client la risposta
		   il thread interrompe la conversazione */
		if ( err )
			break;
		
	}
	
	/* estrae l'handler installato e lo esegue - rimuove info dalla lista */
	pthread_cleanup_pop( 1 );

	/* uscita - se e' stato cancellato non chiama detach (sig_handler invoca
	   join su questo tid ) */
	pthread_testcancel();

	pthread_detach( pthread_self() );
	pthread_exit( NULL );
}

/** sig_handler
 * thread dedicato alla gestione della terminazione asincrona del server.
 * quando SIGINT o SIGTERM vengono intercettati da sigwait la procedura di
 * cancellazione e' avviata:
 *  - viene cancellato per primo il thread main, in modo che nessuna ulteriore
 *    richiesta di connessione venga accettata
 *  - quando join su main ritorna, la procedura cancella ogni client_handler
 *    attivo
 */
void *sig_handler( void *arg )
{
	extern pthread_t main_thread;
	extern pthread_mutex_t list_mutex;
	extern thread_list tl;
	extern struct fstab_st fstab;

	sigset_t set;
	int err, sig;
	unsigned int i;
	pthread_t *threads;
	th_info *ptr;

	printf( "Servizio di cancellazione avviato\n" );

	sigemptyset( &set );
	sigaddset( &set, SIGINT );
	sigaddset( &set, SIGTERM );

	/* attende SIGINT o SIGTERM */
	err = sigwait( &set, &sig );
	if ( err ) {
		perror( "sigwait" );
		exit( EXIT_FAILURE );
	}

	printf( "Inizio procedura di cancellazione\n" );

	/* attende la terminazione del main per avere la garanzia che nessun nuovo
	   thread venga attivato mentre invia le richieste di cancellazione */
	pthread_cancel( main_thread );
	pthread_join( main_thread, NULL );

	/* memorizza ogni tid per poter invocare join sui client_handler */
	pthread_mutex_lock( &list_mutex );

	threads = malloc( (tl.n_items)*sizeof(pthread_t) );
	if ( !threads ) {
		perror( "malloc (sig_handler)" );
		exit( EXIT_FAILURE );
	}
	i = 0, ptr = tl.head;
	while ( ptr ) {
		threads[i] = ptr->tid;
		i++, ptr = ptr->next;
	}

	/* cancella i client_handler */
	for ( i = 0; i < tl.n_items; i++ ) {
		err = pthread_cancel( threads[i] );
		if ( err ) {
			errno = err;
			fprintf( stderr, "pthread_cancel (sig_handler): " );
			fatserror( STDLIBERR );
		}
	}

	pthread_mutex_unlock( &list_mutex );
	
	/* attende la fine delle operazioni prima di deallocare fstab */
	for ( i = 0; i < tl.n_items; i++ )
		pthread_join( threads[i], NULL );

	free ( threads );

	/* cancella la tabella dei file aperti */
	for ( i = 0; i < fstab.numfiles; i++ ) {
		fclose( fstab.fsystem[i].fs );
		free( fstab.fsystem[i].name );
		free( fstab.fsystem[i].fsctrl.fat_ptr );
	}

	free( fstab.fsystem ); /* dealloca la tabella dei file */

	pthread_detach( pthread_self() );
	pthread_exit( NULL );
}

/** cl_cleanup
 * cleanup handler per client_handler
 * arg e' un puntatore alla entry th_info relativa al thread in esecuzione
 * chiude la connessione con il client e dealloca la entry dalla lista
 */
void cl_cleanup( void *arg )
{
	/* lista dei thread globale */
	extern pthread_mutex_t list_mutex;
	extern thread_list tl;

	th_info *info, *returned;

	info = arg;

	/* chiude la connessione con il client */
	if ( closeConnection( info->client_c ) ) {
		fprintf( stderr, "client_handler (closeConnection): " );
		fatserror( STDLIBERR );
	}

	/* rimuove la entry dalla lista dei thread e dealloca la memoria */
	pthread_mutex_lock( &list_mutex );

	returned = list_remove( &tl, pthread_self() );

	if ( info != returned ) /* sanity check */
		fprintf( stderr, "cl_cleanup: error retrieving thread info\n" );
	if ( returned )
		free( returned );

	pthread_mutex_unlock( &list_mutex );
}


/* -- LOCKS LETTORI-SCRITTORI ----------------------------------------------- */
/* Nota: con questa implementazione gli scrittori sono soggetti a starvation  */
/* in breve, permettono molte letture contemporanee ma uno scrittore          */
/* acquisisce la lock di scrittura solo se nessun lettore e' attivo           */

void read_lock( struct file_handle *fh )
{
	pthread_mutex_lock( &( fh->num_mutex ) );
	fh->num_readers++;
	
	if ( fh->num_readers == 1 )
	/* primo lettore - impedisce le scritture bloccando il mutex di scrittura
	   il mutex di scrittura viene poi rilasciato quando termina la lettura
	   l'ultimo lettore in esecuzione */
		pthread_mutex_lock( &( fh->write_mutex ) );
	
	pthread_mutex_unlock( &( fh->num_mutex ) );
}

void read_unlock( struct file_handle *fh )
{

	pthread_mutex_lock( &( fh->num_mutex ) );
	fh->num_readers--;
	
	if ( fh->num_readers == 0 )
	/* nessun altro lettore in esecuzione - rilascia il mutex di scrittura
	   per permettere ad ecentuali scrittori in attesa di eseguire il task */
		pthread_mutex_unlock( &( fh->write_mutex ) );
	
	pthread_mutex_unlock( &( fh->num_mutex ) );
}

void write_lock( struct file_handle *fh )
{
	pthread_mutex_lock( &( fh->write_mutex ) );
}

void write_unlock( struct file_handle *fh )
{
	pthread_mutex_unlock( &( fh->write_mutex ) );
}

/* -- FUNZIONI -------------------------------------------------------------- */
/* -------------------------------------------------------------------------- */

/** execute
 * individua il tipo di richiesta del client e passa il controllo al un gestore
 * specializzato
 *
 * restituisce 0 se riesce a comunicare al client l'esito della richiesta
 * indipendentemente dal fatto che questa abbia generato errore
 * restituisce -1 se si e' verificato un errore che ha impedito di comunicare
 * con il client (fallimento nella compilazione del messaggio di risposta o
 * nell'invio del messaggio stesso)
 */
int execute( struct file_handle *f_handle, channel_t com, message_t *in_msg )
{
	message_t out_msg;
	int err;
	
	memset( &out_msg, 0, sizeof( message_t ) );

	/* Invoca dei wrapper che prelevano gli argomenti dai messaggi ed
	   invocano le procedure vere e proprie sul file system */
	switch ( in_msg->type ) {
	case MSG_MKDIR:
		err = handle_mkdir_request( f_handle, in_msg, &out_msg );
		break;
	case MSG_LS:
		err = handle_ls_request( f_handle, in_msg, &out_msg );
		break;
	case MSG_MKFILE:
		err = handle_mkfile_request( f_handle, in_msg, &out_msg );
		break;
	case MSG_FREAD:
		err = handle_fread_request( f_handle, in_msg, &out_msg );
		break;
	case MSG_FWRITE:
		err = handle_fwrite_request( f_handle, in_msg, &out_msg );
		break;
	case MSG_CP:
		err = handle_cp_request( f_handle, in_msg, &out_msg );
		break;
	default:
		fprintf( stderr, "Ricevuto un messaggio inatteso\n");
		return -1;
	}

	if ( err )
		return -1;
	else if ( sendMessage( com, &out_msg ) < 0 ) {
		fprintf( stderr, "execute (sendMessage): " );
		fatserror( STDLIBERR );
		return -1;
	}

	if ( out_msg.length )
		free( out_msg.buffer );

	return 0;
}


/** handle_*_request - invocazione delle operazioni sul file system
 * Queste funzioni si occupano di prelevare i parametri delle operazioni
 * dal messaggio di richiesta, invocare le operazioni sul file system e
 * compilare l'opportuno messaggio di risposta da inviare al client
 *
 * restituiscono 0 se riescono a codificare l'esito dell'operazione in un
 * messaggio, -1 se qualche errore lo ha impedito.
 *
 * NOTA in caso di errori della libreria standard non codificati da fats
 * (ad esempio malloc), al client si cerca di notificare un messaggio di
 * errore interno al server (MSG_ERROR con buffer INTSVERR)
 */

int handle_mkdir_request( struct file_handle *f_handle,
                          message_t          *in_msg,
                          message_t          *out_msg )
{
	int e;
	
	write_lock( f_handle );
	e = fat_mkdir( f_handle->fs, &( f_handle->fsctrl ), in_msg->buffer );
	write_unlock( f_handle );

	if ( e == STDLIBERR )
		e = INTSVERR;
	
	if ( e ) {
		out_msg->type = MSG_ERROR;
		out_msg->length = sizeof( int );
		out_msg->buffer = malloc( sizeof( int ) );
		if ( !out_msg->buffer ) {
			perror( "malloc (handle_mkdir_request)" );
			return -1;
		}
		memcpy( out_msg->buffer, &e, sizeof( int ) );
	}
	else {
		out_msg->type = MSG_OK;
		out_msg->length = 0;
	}

	return 0;
}

int handle_mkfile_request( struct file_handle *f_handle,
                           message_t          *in_msg,
                           message_t          *out_msg )
{
	int e;

	write_lock( f_handle );
	e = fat_open( f_handle->fs, &( f_handle->fsctrl ), in_msg->buffer );
	write_unlock( f_handle );

	if ( e == STDLIBERR )
		e = INTSVERR;
	
	if ( e ) {
		out_msg->type = MSG_ERROR;
		out_msg->length = sizeof( int );
		out_msg->buffer = malloc( sizeof( int ) );
		if ( !out_msg->buffer ) {
			perror( "malloc (handle_mkfile_request)" );
			return -1;
		}
		memcpy( out_msg->buffer, &e, sizeof( int ) );
	}
	else {
		out_msg->type = MSG_OK;
		out_msg->length = 0;
	}

	return 0;
}

int handle_ls_request( struct file_handle *f_handle,
                       message_t          *in_msg,
                       message_t          *out_msg )
{
	char *list; /* al ritorno contiene la stringa da inviare */
	int e;

	list = NULL;

	read_lock( f_handle );
	e = fat_ls( f_handle->fs, &( f_handle->fsctrl ), in_msg->buffer, &list );
	read_unlock( f_handle );

	if ( e == STDLIBERR )
		e = INTSVERR;
	
	if ( e ) {
		/* in questo caso list non punta a memoria allocata (vd fat_ls) */
		out_msg->type = MSG_ERROR;
		out_msg->length = sizeof( int );
		out_msg->buffer	= malloc( sizeof( int ) );
		if ( !out_msg->buffer ) {
			perror( "malloc (handle_ls_request)" );
			return -1;
		}
		memcpy( out_msg->buffer, &e, sizeof( int ) );
	}
	else {
		out_msg->type = MSG_OK;
		out_msg->length = strlen( list ) + 1;
		out_msg->buffer = list;
	}

	return 0;
}

int handle_fwrite_request( struct file_handle *f_handle,
                           message_t          *in_msg,
                           message_t          *out_msg )
{
	int e, i;
	char *path, *data, *buffer;
	
	buffer = in_msg->buffer;
	
	/* buffer contiene <path>\0<data> */
	
	/* identificazione di <path> */
	for ( i = 0; i < in_msg->length && buffer[i] != '\0'; i++ )
		;
		
	if ( i == in_msg->length )
		/* non dovrebbe mai succedere, messaggi di questo tipo
		   dovrebbero essere rifiutati dal client */
		return -1; 
		
	path = malloc( i+1 );
	if ( !path ) {
		perror( "malloc (handle_fwrite_request)" );
		return -1;
	}
	memcpy( path, buffer, i+1 );
	
	/* puntatore ai dati da scrivere */
	data = buffer+i+1;
	
	write_lock( f_handle );
	e = fat_write( f_handle->fs,
	               &( f_handle->fsctrl ),
	               path,
	               data,
	               in_msg->length - (i+1) );
	write_unlock( f_handle );
	
	free( path );

	if ( e == STDLIBERR )
		e = INTSVERR;
	
	if ( e ) {
		out_msg->type = MSG_ERROR;
		out_msg->length = sizeof( int );
		out_msg->buffer = malloc( sizeof( int ) );
		if ( !out_msg->buffer ) {
			perror( "malloc (handle_fwrite_request)" );
			return -1;
		}
		memcpy( out_msg->buffer, &e, sizeof( int ) );
	}
	else {
		out_msg->type = MSG_OK;
		out_msg->length = 0;
	}
	
	return 0;
}

int handle_fread_request( struct file_handle *f_handle,
                          message_t          *in_msg,
                          message_t          *out_msg )
{
	int read, offset, data_len, path_len;
	char *path, *buffer, *data;
	
	if ( in_msg->length < 2*sizeof(int)+1 )
		return -1;
	
	/* buffer contiene <offset><size><path> */
	buffer = in_msg->buffer;
	
	memcpy( &offset, buffer, sizeof( int ) );
	memcpy( &data_len, buffer+sizeof( int ), sizeof( int ) );

	if ( offset < 0 )
		offset *= -1;
	if ( data_len < 0 )
		data_len *= -1;

	
	/* crea la stringa <path> */
	path_len = in_msg->length - 2*sizeof( int );
	path = malloc( path_len + 1 );
	if ( !path ) {
		perror( "malloc (handle_fread_request)" );
		return -1;
	}
	memcpy( path, buffer+2*sizeof(int), path_len );
	path[path_len] = '\0';
	
	/* restituisce il numero di caratteri letti */
	data = malloc( data_len+1 );
	if ( !data ) {
		perror( "malloc (handle_fread_request)" );
		return -1;
	}
	memset( data, 0x00, data_len+1 );
	
	read_lock( f_handle );
	read = fat_read( f_handle->fs,
	                 &( f_handle->fsctrl ),
	                 path,
	                 offset,
	                 data_len,
	                 data );	
	read_unlock( f_handle );
	
	free( path );

	if ( read == STDLIBERR )
		read = INTSVERR;
	
	if ( read < 0) {
		free( data );
		out_msg->type = MSG_ERROR;
		out_msg->length = sizeof( int );
		out_msg->buffer = malloc( sizeof( int ) );
		if ( !out_msg->buffer ) {
			perror( "malloc (handle_fread_request)" );
			return -1;
		}
		memcpy( out_msg->buffer, &read, sizeof( int ) );
	}
	else {
		out_msg->type = MSG_OK;
		out_msg->length = read+1;
		out_msg->buffer = data;
	}
	
	return 0;
}

int handle_cp_request( struct file_handle *f_handle,
                       message_t          *in_msg,
                       message_t          *out_msg )
{
	char *from, *to;
	int e;

	/* buffer contiene <from>\0<to>\0 - usa direttamente il contenuto di buffer
	   per passare i percorsi a fats_cp */
	from = in_msg->buffer;
	for ( to = from; *to != '\0'; to++ )
		;
	to++;

	write_lock( f_handle );
	e = fat_cp( f_handle->fs, &( f_handle->fsctrl ), from, to );
	write_unlock( f_handle );

	if ( e == STDLIBERR )
		e = INTSVERR;

	if ( e ) {
		out_msg->type = MSG_ERROR;
		out_msg->length = sizeof( int );
		out_msg->buffer = malloc( sizeof( int ) );
		if ( !out_msg->buffer ) {
			perror( "malloc (handle_cp_request)" );
			return -1;
		}
		memcpy( out_msg->buffer, &e, sizeof( int ) );
	}
	else {
		out_msg->type = MSG_OK;
		out_msg->length = 0;
	}

	return 0;

}
