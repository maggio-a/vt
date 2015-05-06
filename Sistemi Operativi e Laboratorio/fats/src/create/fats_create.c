/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* fats_create.c -- creazione di un device compatibile con il file system */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "../fat/fat.h"
#include "../fat/format.h"
#include "../fat/fat_error.h"

void print_usage( char *name ) {
	fprintf( stderr, "Uso: %s usr name nblocks nsize\n", name );
	fprintf( stderr, "  usr    \tnome utente (max %d caratteri)\n", FAT_MAXUSRLEN-1 );
	fprintf( stderr, "  name   \tnome del device\n" );
	fprintf( stderr, "  nblocks\tnumero di blocchi allocati\n" );
	fprintf( stderr, "  nsize  \tdimensione dei blocchi [1-4]\n" );
	fprintf( stderr, "         \t1 = 128 B, 2 = 1 KB, 3 = 2 KB, 4 = 4 KB\n" );
}

int main( int argc, char **argv ) {

	unsigned int nblocks; /* Numero di blocchi da allocare       */
	int blksize;   		  /* Dimensione dei blocchi              */
	int device_fd;		  /* File descriptor del device generato */
	FILE *device;		  /* Puntatore al device generato        */
	int err;
	
	if ( argc < 5 ||
		strlen( argv[4] ) > 1 ||
		strlen( argv[1] ) > FAT_MAXUSRLEN-1 ) /* lascia spazio per NULL */
	{
		print_usage( argv[0] );
		exit( EXIT_FAILURE );
	}

	switch ( **(argv+4) ) {
	case '1':
		blksize = 128;
		break;
	case '2':
		blksize = 1024;
		break;
	case '3':
		blksize = 2048;
		break;
	case '4':
		blksize = 4096;
		break;
	default:
		print_usage( argv[0] );
		return EXIT_FAILURE;
	}

	nblocks = atoi( argv[3] );

	/* crea il file system - fallisce se il file specificato esiste gia' */
	device_fd = open( argv[2], O_RDWR|O_CREAT|O_EXCL, 0755 );
	if ( device_fd < 0 ) {
		if ( errno == EEXIST )
			fprintf( stderr, "Errore: il device specificato esiste gia'\n");
		else
			perror( "open (main)" );

		return EXIT_FAILURE;
	}

	if ( close( device_fd ) < 0 ) {
		perror( "close (main)" );
		return EXIT_FAILURE;
	}

	/* formatta il file system */
	device = fopen( argv[2], "r+" );
	if ( device == NULL ) {
		perror( "fdopen (main)" );
		return EXIT_FAILURE;
	}

	printf( "Generazione del file system %s\n", argv[2] );
	printf( "Nome utente: %s\n", argv[1] );
	printf( "Numero di blocchi da allocare: %d\n", nblocks );
	printf( "Dimensione dei blocchi: %d bytes\n", blksize );

	err = fat_format( device, argv[1], blksize, nblocks );
	if ( err ) {
		fprintf( stderr, "fat_format (main): " );
		fatserror( err );
		return EXIT_FAILURE;
	}

	printf( "Fatto.\n" );

	if ( fclose( device ) == EOF ) {
		perror( "fclose (main)" );
	}

	return EXIT_SUCCESS;
}

