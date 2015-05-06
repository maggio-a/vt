/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* format.c -- formattazione di un file per fats */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "fat.h"
#include "fat_defs.h"
#include "fat_error.h"

#include "format.h"

/** formattazione del file system
 * inizialmente il file system contiene solo la cartella di root con le entry
 * di default . e .. - .. (parent) punta ancora alla cartella di root
 */
int fat_format( FILE *fs,
                const char *usrname,
                int block_size,
                unsigned int num_block)
{

	int err; 
	unsigned int i;                
	struct boot_sector meta; /* metadati del file system    */
	struct dir_entry entry;  /* scrittura directory entries */
	unsigned int *table;     /* file access table           */
	char *block, *offt;      /* scrittura blocchi           */

	/* compilazione boot sector */
	meta.fs_type = FAT_TYPE;
	meta.block_size = block_size;
	meta.num_block = num_block;
	strncpy( meta.usr, usrname, FAT_MAXUSRLEN );

	rewind( fs ); /* inizio del file */

	/* trascrive il boot sector sul device */
	fwrite( &meta, sizeof( meta ), 1, fs );
	err = ferror( fs );
	if ( err ) 
		return EWFCD;
	
	/* genera la FAT - la root directory inizialmente sta in un solo blocco */
	assert( 2*sizeof( struct dir_entry ) <= block_size );

	/* alloca la FAT */
	table = (unsigned int *) malloc( num_block*sizeof( unsigned int ) );
	if ( table == NULL )
		return STDLIBERR;

	/* il primo blocco e' occupato dalla root */
	table[0] = LAST_BLOCK;
	/* tutti gli altri blocchi sono liberi */
	for ( i = 1; i < num_block; i++ )
		table[i] = BLOCK_FREE;

	/* aggiungiamo la FAT al device */
	fwrite( table, sizeof( unsigned int ), num_block, fs );
	err = ferror( fs );
	if ( err ) 
		return EWFCD;

	free( table );

	/* alloca i blocchi
	   il blocco 0 deve contenere le entries della cartella di root
	   il filesystem è formattato, nella root ci sono solo le entry . e .. */
	block = malloc( block_size );
	if ( block == NULL )
		return STDLIBERR;

	/* azzera il blocco */
	for ( i = 0; i < block_size; i++ )
		block[i] = INIT_BLOCK;

	/* scrive nel blocco la prima entry - '.' */
	entry.used = DIR_ENTRY_BUSY;
	entry.attr = SUB_ENTRY;
	entry.index = ROOT_IDX;
	entry.len = 2*sizeof( struct dir_entry );
	strncpy( entry.name, ".", 2 );

	memcpy( block, &entry, sizeof( struct dir_entry ) );

	/* scrive nel blocco la seconda entry - '..' */
	strncpy( entry.name, "..", 3 );

	offt = block + sizeof( struct dir_entry );
	memcpy( offt, &entry, sizeof( struct dir_entry ) );

	/* scrive il primo blocco */
	fwrite( block, block_size, 1, fs );
	err = ferror( fs );
	if ( err ) 
		return EWFCD;

	/* tutti gli altri blocchi sono vuoti */
	for ( i = 0; i < block_size; i++ )
		block[i] = INIT_BLOCK;

	for ( i = 0; i < num_block-1; i++ ) {
		fwrite( block, block_size, 1, fs );
		err = ferror( fs );
		if ( err )
			return EWFCD;
	}

	return 0;
}
