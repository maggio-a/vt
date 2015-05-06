/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* sol_com.c -- implementazione delle funzioni per l'IPC client-server */

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

#include "../fat/fat_defs.h"
#include "com_defs.h"
#include "sol_com.h"

/* (ri)definizione */
#ifndef UNIX_PATH_MAX
#define UNIX_PATH_MAX 108
#endif

/** createServerChannel
 * genera un socket con il nome specificato, ne restituisce il descrittore
 */ 
serverChannel_t createServerChannel( const char *path )
{
	struct sockaddr_un server_addr;
	int socket_fd;

	if ( strlen( path ) >= UNIX_PATH_MAX )
		return FATSENAMETOOLONG;

	socket_fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if ( socket_fd < 0)
		return -1;

	server_addr.sun_family = AF_UNIX;
	strncpy( server_addr.sun_path, path, strlen( path )+1 );

	unlink( path );

	if( bind( socket_fd, ( struct sockaddr * ) &server_addr, sizeof( server_addr ) ) )
		return -1;

	return ( serverChannel_t ) socket_fd;
}

/** closeSocket
 * chiude un canale di ascolto
 */
int closeSocket( serverChannel_t s )
{
	return close( s );
}

/** acceptConnection
 * accetta una richiesta di connessione dal canale di ascolto e restuisce il
 * canale di comunicazione con il client
 */
channel_t acceptConnection( serverChannel_t s )
{
	struct sockaddr_un cl_addr; /* Indirizzo del client */
	socklen_t cl_addrsize;      /* Dimensione */
	channel_t client_fd;        /* Descrittore del canale di comunicazione */

	cl_addrsize = ( socklen_t ) sizeof( cl_addr );

	client_fd = accept( s, ( struct sockaddr * ) &cl_addr, &cl_addrsize );

	if ( client_fd < 0 )
		return STDLIBERR;
	else
		return client_fd;
}

/** receiveMessage
 * restituisce SEOF se EOF sul socket, -1 se errore
 */
int receiveMessage( channel_t sc, message_t *msg )
{
	int nread;   /* Numero di bytes letti da read */
	int totread; /* Numero di bytes letti in totale */

	/* legge la struttura */
	totread = nread = read( sc, msg, sizeof( message_t ) );
	if ( nread < 0 )
		return STDLIBERR;
	else if ( nread < sizeof( message_t ) )
		return SEOF; /* socket end of file */

	if ( msg->length == 0 ) { /* nessun buffer da leggere */
		msg->buffer = NULL;
		return totread;
	}

	/* legge le informazioni supplementari */
	if ( !( msg->buffer = malloc( msg->length + 2 ) ) )
		return STDLIBERR;

	totread += ( nread = read( sc, msg->buffer, msg->length ) );
	
	if ( nread < 0 )
		return STDLIBERR;
	else if ( nread < msg->length ) {
		/* se non riesce a leggere tutto il buffer lo libera */
		free( msg->buffer );
		msg->buffer = NULL;
		return SEOF;
	}

	msg->buffer[msg->length] = 0x00;
	msg->buffer[msg->length+1] = 0x00; /* aggiunge due byte nulli finali
	per proteggere una eventuale lettura del buffer come stringa */
	return totread;
}

/** sendMessage
 * invia il messaggio sul canale di comunicazione
 */
int sendMessage( channel_t sc, message_t *msg )
{
	int nwritten;
	int totwritten;

	/* invia la struttura */
	totwritten = nwritten = write( sc, msg, sizeof( message_t ) );
	if ( nwritten < 0 )
		return STDLIBERR;

	if ( msg->length == 0 )
		return totwritten;

	/* invia il contenuto del buffer */
	totwritten += nwritten = write( sc, msg->buffer, msg->length );
	if ( nwritten < 0 )
		return STDLIBERR;

	return totwritten;
}

/** openConnection
 * apre un canale di trasmissione verso path
 */
channel_t openConnection( const char* path )
{
	struct sockaddr_un address;
	int com, e;
	int i;

	if ( strlen( path ) >= UNIX_PATH_MAX )
		return FATSENAMETOOLONG;

	com = socket( AF_UNIX, SOCK_STREAM, 0 );
	if ( com < 0 )
		return -1;

	address.sun_family = AF_UNIX;
	strncpy( address.sun_path, path, strlen( path )+1 );

	/* 5 tentativi */
	e = -1;
	for ( i = 0; i < 5 && e < 0; i++ ) {
		e = connect( com, ( struct sockaddr * ) &address, sizeof( address ) );
		sleep( 1 ); /* attende */
	}

	return ( e < 0 ) ? -1 : ( channel_t ) com;

}

/** closeConnection
 * chiude un canale di comunicazione client->server
 */
int closeConnection( channel_t sc )
{
	return close( sc );
}

