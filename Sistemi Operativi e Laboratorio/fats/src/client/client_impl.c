/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* client_impl.c -- funzioni del client fats */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "../com/com_defs.h"
#include "../com/sol_com.h"
#include "../fat/fat_defs.h"
#include "../fat/fat_error.h"
#include "../common/common.h"

#include "client_impl.h"

/** do_ handshake
 * fase iniziale del protocollo di comunicazione
 * il client invia un messaggio di richiesta MSG_CONNECT
 * il server risponde con MSG_OK se tutto e' andato a buon fine, oppure
 *     con MSG_ERROR ed allegando l'errore riscontrato
 * 
 * restituisce true se l'handshake ha successo, false altrimenti
 * se *dnameptr == NULL (cioe' se l'utente non ha specificato un device)
 * assegna a *dnameptr il nome del device selezionato dal server
 */
int do_handshake( char *usrname, char **dnameptr )
{
	int n;
	size_t len;
	message_t msg;
	char *devname;

	extern channel_t com;
	
	devname = *dnameptr;

	memset( &msg, 0x00, sizeof( message_t ) );
	len = strlen( usrname );

	/* richiesta di connessione */
	msg.type = MSG_CONNECT;

	msg.length = len + 1;
	if ( devname ) /* specifica il device */
		msg.length += strlen( devname ) + 1;

	msg.buffer = malloc( msg.length );
	if ( !msg.buffer ) { 
		perror( "malloc (do_handshake)" );
		exit( EXIT_FAILURE );
	}
	strncpy( msg.buffer, usrname, msg.length );
	if ( devname )
		strncpy( msg.buffer+len+1, devname, msg.length-(len+1) );
	
	n = sendMessage( com, &msg );
	if ( n < 0 ) {
		fprintf( stderr, "Handshake fallito: " );
		fatserror( n );
		return n;
	}

	free( msg.buffer );

	/* ricezione della risposta */
	n = receiveMessage( com, &msg );

	if ( n == SEOF ) {
		printf( "Handshake interrotto dal server\n" );
		return 1;
	}
	else if ( n < 0 )  {
		fprintf( stderr, "Handshake fallito: " );
		fatserror( n );
		return n;
	}

	/* controllo della risposta */
	if ( msg.type == MSG_OK ) {
		printf("Handshake completato\n");
		if ( !( *dnameptr ) )
			*dnameptr = msg.buffer;
		return 0;
	}
	else if ( msg.type == MSG_ERROR ) {
		/* errore lato server */
		fprintf( stderr, "Handshake fallito.\n" );
		if ( msg.buffer ) {
			fatserror( *((int*) msg.buffer) );
			free( msg.buffer );
		}
	}
	else {
		fprintf( stderr, "Ricevuto un messaggio inatteso\n" );
		if ( msg.length )
			free( msg.buffer );
	}

	return 1;
}

/** server_listener
 * thread dedicato alla ricezione dei messaggi in ingresso
 * arg e' un puntatore al canale di comunicazione con il server, ma non e'
 *     allocato nell'heap - e' una variabile del main
 * se la conversazione e' interrotta dal server invoca exit, altrimenti legge
 *     le risposte fino a quando l'utente non decide di uscire
 *     in questo caso al thread viene inviato un segnale per sbloccare la
 *     ricezione e disconnected==true
 */
void *server_listener( void *arg )
{
	message_t in_msg;        /* messaggio in ingresso */
	int n, state;

	extern channel_t com;    /* socket connesso al server */

	memset( &in_msg, 0, sizeof( message_t ) );

	/* legge fino a quando il server non viene terminato */
	while ( (n = receiveMessage( com, &in_msg )) != SEOF ) {

		/* disabilita la cancellazione se ritorna da read(), in questo
		   modo l'eventuale buffer del messaggio e' sempre deallocato */
		pthread_setcancelstate( PTHREAD_CANCEL_DISABLE, &state );

		if ( n < 0 ) {
			fprintf( stderr, "receiveMessage (server_listener): " );
			fatserror( n );
			exit( EXIT_FAILURE );
		}

		/* output del messaggio */
		switch ( in_msg.type ) {
		case MSG_OK:
			if ( in_msg.length )
				fprintf( stdout, "%s\n", in_msg.buffer );
			break;
		case MSG_ERROR:
			fatserror( *((int*) in_msg.buffer) );
			break;
		default:
			fprintf( stderr, "Ricevuto un messaggio inatteso\n" );
		}

		if ( in_msg.length )
			free( in_msg.buffer );

		pthread_setcancelstate( PTHREAD_CANCEL_ENABLE, &state );
	}

	/* se il client termina per il comando quit, il thread viene cancellato
	   dentro al loop - se invece il server chiude la connessione, read()
	   restituisce SEOF, causando l'uscita dal loop: in quest'ultimo caso il
	   processo viene immediatamente chiuso da exit() */
	printf( "\nConnessione con il server interrotta\n" );

	exit( EXIT_FAILURE ); /* non esattamente un errore... */
}

/** handle_input
 * invio dei comandi al server
 * QUIT definita in client_fn.h
 */
void handle_input()
{
	char input[MAXLINE];  /* User input */
	int err, n;
	message_t msg;        /* Messaggio inviato al server */

	extern channel_t com; /* socket connesso al server */
	
	memset( &msg, 0x00, sizeof( message_t ) );

	for (;;) {
		/* azzera l'input */
		memset( input, 0x00, MAXLINE );
		msg.buffer = NULL;
		n = 0;

		/* legge il comando inserito */
		fflush( stdout );
		fflush( stderr );
		get_line( input, MAXLINE );

		if ( strcmp( input, QUIT ) == 0 ) {
			return;
		}
		else { /* prepara il messaggio e lo invia */
			err = parse_cmd( input, &msg );
			if ( !err ) {
				n = sendMessage( com, &msg );
			}

			if ( msg.buffer )
				free( msg.buffer );

			if ( n < 0 ) { /* invio del messaggio fallito */
				fprintf( stderr, "sendMessage (handle_input): " );
				fatserror( n );
				return;
			}
		}
	} /* forever */
}

/** get_line
 * legge al piu' limit caratteri della prossima riga in ingresso, e li salva
 * nel buffer puntato da line
 * la stringa salvata e' sempre terminata dal carattere nullo \0
 */
void get_line( char *line, int limit )
{
	int i;
	char c; /* ultimo carattere letto */

	i = 0;
	while ( i < limit-1 && ( c = getchar() ) != '\n' && c != EOF )
		*( line + i++ ) = c;

	*( line + i ) = '\0';
}

/** parse_cmd
 * effettua il parsing del comando specificato da input
 *  - restituisce 0 e compila il messaggio msg come specificato dal protocollo
 *    se il comando e' sintatticamente valido
 *  - restituisce -1 se il comando non e' valido
 */
int parse_cmd( char *input, message_t *msg )
{
	char *cmd, *arg[3], *saveptr;
	unsigned int n0, n1;
	int code, offset, len, e;

	arg[0] = arg[1] = arg[2] = NULL;

	cmd = strtok_r( input, " ", &saveptr );
	if ( !cmd )
		return -1;

	/* identifica il comando */
	if ( strcmp( cmd, CMD_MKDIR ) == 0 )
		code = MSG_MKDIR;	
	else if ( strcmp( cmd, CMD_MKFILE ) == 0 )
		code = MSG_MKFILE;
	else if ( strcmp( cmd, CMD_LS ) == 0 )
		code = MSG_LS;
	else if ( strcmp( cmd, CMD_FWRITE ) == 0 )
		code = MSG_FWRITE;
	else if ( strcmp( cmd, CMD_CP ) == 0 )
		code = MSG_CP;
	else if ( strcmp( cmd, CMD_FREAD ) == 0 ) 
		code = MSG_FREAD;
	else {
		fprintf( stderr, "%s: comando sconosciuto\n", cmd );
		return -1;
	}

	/* parsing degli argomenti */
	arg[0] = strtok_r( NULL, " ", &saveptr );

	switch ( code ) {
	case MSG_FWRITE:
		/* la stringa da appendere va racchiusa tra virgolette */
		while( *saveptr == ' ' ) saveptr++; /* mangia gli spazi iniziali */
		arg[1] = strtok_r( NULL, "\"", &saveptr );
		break;
	default:
		arg[1] = strtok_r( NULL, " ", &saveptr );
		arg[2] = strtok_r( NULL, " ", &saveptr ); /* per MSG_FREAD */

	}
	
	switch ( code ) {
	case MSG_MKDIR:
		if ( !arg[0] ) {
			fprintf( stderr, "Uso: %s dir\n", CMD_MKDIR );
			return -1;
		}
		break;
	case MSG_MKFILE:
		if ( !arg[0] ) {
			fprintf( stderr, "Uso: %s file\n", CMD_MKFILE );
			return -1;
		}
		break;
	case MSG_LS:
		if ( !arg[0] ) {
			fprintf( stderr, "Uso: %s dir\n", CMD_LS );
			return -1;
		}
		break;
	case MSG_FWRITE:
		if ( !arg[0] || !arg[1] ) {
			fprintf( stderr, "Uso: %s file \"<stringa>\"\n", CMD_FWRITE );
			return -1;
		}
		break;
	case MSG_CP:
		if ( !arg[0] || !arg[1] ) {
			fprintf( stderr, "Uso: %s origine destinazione\n", CMD_CP );
			return -1;
		}
		break;
	case MSG_FREAD:
		if ( !arg[0] || !arg[1] || !arg[1] ) {
			fprintf( stderr, "Uso: %s file offset dimensione\n", CMD_FREAD );
			return -1;
		}
		break;
	}

	/* il primo argomento e' sempre un percorso assoluto */
	if ( ( e = check_path( arg[0] ) ) ) {
		fprintf( stderr, "%s: ", arg[0] );
		fatserror( e );
		return -1;
	}

	n0 = strlen( arg[0] );
	
	/* compilazione del messaggio */
	msg->type = code;

	switch ( code ) {
	case MSG_MKDIR:
	case MSG_MKFILE:
	case MSG_LS:
		msg->length = n0 + 1;
		msg->buffer = malloc( msg->length );
		memcpy( msg->buffer, arg[0], msg->length );
		break;
	case MSG_FWRITE:
		n1 = strlen( arg[1] );
		msg->length = n0 + n1 + 1;
		msg->buffer = malloc( msg->length );
		memcpy( msg->buffer, arg[0], n0+1 );
		memcpy( msg->buffer + n0+1, arg[1], n1 );
		break;
	case MSG_CP:
		/* il buffer contiene <path_source>\0<path_destination>\0 */ 
		if ( ( e = check_path( arg[1] ) ) ) {
			fprintf( stderr, "%s: ", arg[1] );
			fatserror( e );
			return 1;
		}
		n1 = strlen( arg[1] );
		msg->length = n0 + n1 + 2;
		msg->buffer = (char *) malloc( msg->length );
		memcpy( msg->buffer, arg[0], n0+1 );
		memcpy( msg->buffer + n0+1, arg[1], n1+1 );
		break;
	case MSG_FREAD:
		offset = atoi( arg[1] );
		len = atoi( arg[2] );
		/* printf( "offset=%d length=%d\n", offset, len ); */
		msg->length = 2*sizeof(int) + n0;
		msg->buffer = malloc( msg->length );
		memcpy( msg->buffer, &offset, sizeof( int ) );
		memcpy( msg->buffer + sizeof( int ), &len, sizeof( int ) );
		memcpy( msg->buffer + 2*sizeof( int ), arg[0], n0 );
		break;
	}

	if ( !msg->buffer ) {
		perror( "malloc (parse_cmd)" );
		exit( EXIT_FAILURE );
	}

	return 0;
}
