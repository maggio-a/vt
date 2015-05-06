/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* ops_fat.c -- implementazione delle operazioni offerte dal file system      */

/* -- Nota sul calcolo degli offset ----------------------------------------- */
/*
 * Un device fats e' diviso in 3 sezioni:
 *
 *   [BOOT SECTOR][FAT][DATA REGION]
 *
 * Quando e' necessario accedere ad un elemento della FAT, oppure un blocco
 * della data region, l'offset e' calcolato come segue:
 *
 *   - nel caso della FAT, supponiamo di voler individuare l'elemento FAT[i];
 *     occorre posizionare il puntatore del file al byte
 *
 *       offset = sizeof(struct boot_sector) + i*sizeof(unsigned int)
 *         (FAT e' un array di unsigned int)
 *
 *     ed eseguire la lettura di un unsigned int
 *
 *   - nel caso di un blocco, la struttura di controllo compilata da fat_mount
 *     contiene gia' l'offset del primo blocco dati (base), e la dimensione del
 *     blocco e' salvata nel boot_sector relativo, quindi per posizionarsi
 *     all'inizio dell'i-esimo blocco l'offset e'
 *
 *       base = sizeof(struct boot_sector) + num_block*sizeof(unsigned int)
 *       offset = base + i*block_size                                         */
/* -------------------------------------------------------------------------- */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../common/common.h"
#include "ops_fat.h"

#define MIN(a,b) ((a) < (b) ? (a) : (b))

/* -- NAVIGAZIONE DEL FILE SYSTEM ------------------------------------------- */
/* Funzioni di utilita' visibili solo all'interno di questo file              */

static int get_dt(
	FILE *fs, struct fat_ctrl *f_ctrl, struct dir_entry *entry_info,
	struct dir_entry **dt );

static int get_dir_entry(
	FILE *fs, struct fat_ctrl *f_ctrl, const char *path,
	struct dir_entry *retval );

static char *get_parent_path( const char *path );

static unsigned int get_block( unsigned int *fat, unsigned int size );

static int write_dir_table(
		FILE *fs, struct fat_ctrl *f_ctrl, struct dir_entry *dt );

static int update_entry(
		FILE *fs, struct fat_ctrl *f_ctrl, const char *path,
		struct dir_entry parent_info, unsigned int len );

/* -- IMPLEMENTAZIONE DELLE OPERAZIONI -------------------------------------- */
/* -------------------------------------------------------------------------- */

/** fat_mkdir
 * path = <parent>/<dir_name>
 * Aggiunge alla directory parent la directory dir_name
 *
 * Algoritmo utilizzato
 *   - Recupera la directory table di parent (o della radice se parent == "")
 *   - Scandisce le entry per assicurarsi dir_name non esista
 *   - Aggiunge la entry alla directory table di parent (eventualmente alloca
 *     un nuovo blocco) e crea la nuova directory table per dir_name
 *   - Trascrive le modifiche alla FAT e alla data region sul device
 *   - Aggiorna i riferimenti alla directory modificata:
 *       - se parent ha un antenato, la entry relativa deve essere aggiornata
 *       - se parent contiene delle sub directory, le loro entry .. devono
 *         essere aggiornate
 */
int fat_mkdir( FILE *fs, struct fat_ctrl *f_ctrl, char *path )
{
	int e, i, n;
	char *parent;                 /* percorso directory che ospita <dir_name> */
	char *dir_name;               /* nome della directory da creare */
	struct dir_entry *dt;         /* directory table di <parent> */
	struct dir_entry parent_info;
	unsigned int new_block, extended_block;

	e = check_path( path );
	if ( e )
		return e;
	
	for ( i = strlen( path )-1; i >= 0 && path[i] != '/'; i-- )
		;
	dir_name = &path[i+1];

	if ( !(*dir_name) ) /* dir_name == "" => path === "/" */
		return EDAEX;

	parent = get_parent_path( path );
	if ( !parent )
		return STDLIBERR;

	e = get_dir_entry( fs, f_ctrl, parent, &parent_info );
	if ( e ) {
		free( parent );
		return e;
	}

	e = get_dt( fs, f_ctrl, &parent_info, &dt );
	if ( e ) {
		free( parent );
		return e;
	}

	n = parent_info.len / sizeof( struct dir_entry );
	for ( i = 0, e = 0; i < n && !e; i++ )
		if ( strcmp(dt[i].name, dir_name ) == 0 )
			e = EDAEX;

	if ( e ) {
		/* la cartella esiste gia' */
		free ( dt );
		free( parent );
		return e;
	}
	else {
		/* la entry non esiste, crea la subdir */
		struct dir_entry *dt_, *new_dir;
		int b_size, bytesleft;
		unsigned int *fat;
		unsigned int idx;
		/* se aggiunge una entry alla root */
		int root = !strcmp( parent, "/" );

		fat = f_ctrl->fat_ptr;
		b_size = f_ctrl->b_sector.block_size;
		bytesleft = parent_info.len%b_size ? b_size - (parent_info.len%b_size) : 0;

		new_block = extended_block = LAST_BLOCK;
		if ( bytesleft < sizeof(struct dir_entry) ) {
			extended_block = get_block( fat, f_ctrl->b_sector.num_block );
			if ( extended_block == LAST_BLOCK ) {
				/* spazio esaurito */
				free ( dt );
				free( parent );
				return ENMSD;
			}
		}
		/* alloca spazio per la nuova directory table */
		new_block = get_block( fat, f_ctrl->b_sector.num_block );
		if ( new_block == LAST_BLOCK ) {
			/* spazio esaurito */
			free ( dt );
			free( parent );
			fat[extended_block] = BLOCK_FREE;
			return ENMSD;
		}

		/* nuova dimensione della directory table */
		dt[0].len += sizeof(struct dir_entry);
		/* se alla radice ".." e' uaguale a "." */
		if ( root )
			dt[1].len = dt[0].len;

		dt_ = dt;
		dt = realloc( dt, dt[0].len );
		if ( !dt ) {
			free( dt_ );
			free( parent );
			return STDLIBERR;
		}

		memset( &dt[n], 0x00, sizeof(struct dir_entry) );
		dt[n].used = DIR_ENTRY_BUSY;
		memcpy( dt[n].name, dir_name, strlen(dir_name)+1 );
		dt[n].attr = SUB_ENTRY;
		dt[n].index = new_block;
		dt[n].len = 2*sizeof(struct dir_entry);

		new_dir = calloc( 2, sizeof(struct dir_entry) );
		if ( !new_dir ) {
			free( dt );
			free( parent );
			return STDLIBERR;
		}
		memcpy( &new_dir[0], &dt[n], sizeof(struct dir_entry) );
		memcpy( new_dir[0].name, ".", 2 );
		memcpy( &new_dir[1], &dt[0], sizeof(struct dir_entry) );
		memcpy( new_dir[1].name, "..", 3 );

		/* aggiorna la FAT */
		fat[new_block] = LAST_BLOCK;
		fseek( fs, sizeof(struct boot_sector)+new_block*sizeof(unsigned int), SEEK_SET );
		fwrite( fat+new_block, sizeof(unsigned int), 1, fs );

		/* se allocato un nuovo blocco anche per la parent directory table */
		if ( extended_block != LAST_BLOCK ) {
			for ( idx = dt[0].index; ; idx = fat[idx] )
				if ( fat[idx] == LAST_BLOCK )
					break;
			
			fat[idx] = extended_block;
			fseek( fs, sizeof(struct boot_sector)+idx*sizeof(unsigned int), SEEK_SET );
			fwrite( &extended_block, sizeof(unsigned int), 1, fs );

			fat[extended_block] = LAST_BLOCK;
			fseek( fs, sizeof(struct boot_sector)+extended_block*sizeof(unsigned int), SEEK_SET );
			fwrite( f_ctrl->fat_ptr+extended_block, sizeof(unsigned int), 1, fs );
		}

		if ( ferror( fs ) ) {
			clearerr( fs );
			free( dt );
			free( parent );
			free( new_dir );
			return EWFCD;
		}

		/* scrive le directory table */
		if ( (e = write_dir_table( fs, f_ctrl, dt )) ||
		     (e = write_dir_table( fs, f_ctrl, new_dir )) ||
		/* aggiorna la entry relativa alla parent directory
		   la dimensione e' cambiata ( una entry in piu' ) solo se la entry e'
		   inserita a profondita' > 1 */
		     ((!root) && (e = update_entry( fs, f_ctrl, parent, dt[1], dt[0].len ))) )
		{
			free( dt );
			free( parent );
			free( new_dir );
			return e;
		}

		free( parent );
		free( new_dir );

		/* aggiorna le entry ".." relative alle subdir di parent, visto che ora
		   parent ha una entry in piu' */
		for ( i = 2; i < n; i++ ) {
			struct dir_entry einfo;
			size_t dd_offt;

			if ( dt[i].attr != SUB_ENTRY )
				continue;

			/* offset della entry .. relativa a dt[i] */
			dd_offt = f_ctrl->blk_base + ( dt[i].index * b_size ) + sizeof( struct dir_entry );
			/* aggiorna il campo len */
			fseek( fs, dd_offt, SEEK_SET );
			fread( &einfo, sizeof( struct dir_entry ), 1, fs );

			if ( ferror( fs ) ) {
				clearerr( fs );
				free( dt );
				return ERBD;
			}

			einfo.len += sizeof( struct dir_entry );
			fseek( fs, - sizeof( struct dir_entry ), SEEK_CUR );
			fwrite( &einfo, sizeof( struct dir_entry ), 1, fs );

			if ( ferror( fs ) ) {
				clearerr( fs );
				free( dt );
				return EWBD;
			}
		}

		free( dt );
	}

	return 0;

}

/** fat_open
 * path = <parent>/<file_name>
 * Aggiunge alla directory parent il file file_name
 *
 * Algoritmo utilizzato
 *   - Recupera la directory table di parent (o della radice se parent == "")
 *   - Scandisce le entry per assicurarsi file_name non esista
 *   - Aggiunge la entry alla directory table di parent (eventualmente alloca
 *     un nuovo blocco) ed alloca un nuovo blocco per file_name
 *   - Trascrive le modifiche alla FAT e alla data region sul device
 *   - Aggiorna i riferimenti alla directory modificata:
 *       - se parent ha un antenato, la entry relativa deve essere aggiornata
 *       - se parent contiene delle sub directory, le loro entry .. devono
 *         essere aggiornate
 */
int fat_open( FILE *fs, struct fat_ctrl *f_ctrl, char *path )
{
	int e, i, n;
	struct dir_entry *dt, parent_info;
	unsigned int new_block, extended_block;

	char *parent, *file_name;

	e = check_path( path );
	if ( e )
		return e;
	
	/* nome del file da creare */
	for ( i = strlen( path )-1; path[i] != '/'; i-- )
		;
	file_name = &path[i+1];

	if ( !(*file_name) ) /* path == "/" */
		return EDAEX;

	parent = get_parent_path( path );
	if ( !parent )
		return STDLIBERR;

	e = get_dir_entry( fs, f_ctrl, parent, &parent_info );
	if ( e ) {
		free( parent );
		return e;
	}

	/* directory table parent */
	e = get_dt( fs, f_ctrl, &parent_info, &dt );
	if ( e ) {
		free( parent );
		return e;
	}

	n = parent_info.len / sizeof(struct dir_entry);
	for ( i = 0, e = 0; i < n && !e; i++ ) 
		if ( strcmp( dt[i].name, file_name ) == 0 )
			e = EDAEX; /* entry esistente (anche se non directory) */

	if ( e ) {
		free( dt );
		free( parent );
		return e;
	}
	else {
		/* crea il file vuoto - alloca comunque un blocco */
		struct dir_entry *dt_;
		int b_size, bytesleft;
		unsigned int *fat, idx;

		int root = !strcmp( parent, "/" );

		fat = f_ctrl->fat_ptr;
		b_size = f_ctrl->b_sector.block_size;
		bytesleft = parent_info.len%b_size ? b_size - (parent_info.len%b_size) : 0;

		new_block = extended_block = LAST_BLOCK;
		if ( bytesleft < sizeof(struct dir_entry) ) {
			extended_block = get_block( fat, f_ctrl->b_sector.num_block );
			if ( extended_block == LAST_BLOCK ) {
				/* spazio esaurito */
				free( parent );
				free ( dt );
				return ENMSD;
			}
		}
		/* alloca il blocco per il nuovo file vuoto */
		new_block = get_block( fat, f_ctrl->b_sector.num_block );
		if ( new_block == LAST_BLOCK ) {
			/* spazio esaurito */
			free( parent );
			free ( dt );
			fat[extended_block] = BLOCK_FREE;
			return ENMSD;
		}

		/* aggiorna la dimensione della dt */
		dt[0].len += sizeof(struct dir_entry);
		if ( root )
			dt[1].len = dt[0].len;

		dt_ = dt;
		dt = realloc( dt, dt[0].len );
		if ( !dt ){
			free( dt_ );
			free( parent );
			return STDLIBERR;
		}

		memset( &dt[n], 0x00, sizeof(struct dir_entry) );
		dt[n].used = DIR_ENTRY_BUSY;
		memcpy( dt[n].name, file_name, strlen(file_name)+1 );
		dt[n].attr = FILE_ENTRY;
		dt[n].index = new_block;
		dt[n].len = 0; /* file vuoto */

		/* aggiorna la fat */
		fat[new_block] = LAST_BLOCK;
		fseek( fs, sizeof(struct boot_sector)+new_block*sizeof(unsigned int), SEEK_SET );
		fwrite( fat+new_block, sizeof(unsigned int), 1, fs );

		/* se allocato un nuovo blocco alla dt parent */
		if ( extended_block != LAST_BLOCK ) {
			for ( idx = dt[0].index; ; idx = fat[idx] )
				if ( fat[idx] == LAST_BLOCK )
					break;

			fat[idx] = extended_block; /* punta al blocco appena assegnato */
			fseek( fs, sizeof(struct boot_sector)+idx*sizeof(unsigned int), SEEK_SET );
			fwrite( fat+idx, sizeof(unsigned int), 1, fs );

			fat[extended_block] = LAST_BLOCK; /* il blocco assegnato e' l'ultimo */
			fseek( fs, sizeof(struct boot_sector)+extended_block*sizeof(unsigned int), SEEK_SET );
			fwrite( fat+extended_block, sizeof(unsigned int), 1, fs );
		}

		if ( ferror( fs ) ) {
			clearerr( fs );
			free( dt );
			free( parent );
			return EWFCD;
		}

		/* writeback della directory table */
		if ( (e = write_dir_table( fs, f_ctrl, dt )) ||
		/* non serve scrivere il blocco del file visto che e' vuoto
		   ma bisogna aggiornare la entry della cartella che ospita il
		   file - ora ha una entry in piu' */
		     ((!root) && (e = update_entry( fs, f_ctrl, parent, dt[1], dt[0].len ))) )
		{
			free( dt );
			free( parent );
			return e;
		}

		free( parent );

		/* aggiorna le entry ".." relative alle subdir di parent, visto che ora
		   parent ha una entry in piu' */
		for ( i = 2; i < n; i++ ) {
			struct dir_entry einfo;
			size_t dd_offt;

			if ( dt[i].attr != SUB_ENTRY )
				continue;

			dd_offt = f_ctrl->blk_base + ( dt[i].index * b_size ) + sizeof( struct dir_entry );

			fseek( fs, dd_offt, SEEK_SET );
			fread( &einfo, sizeof( struct dir_entry ), 1, fs );

			if ( ferror( fs ) ) {
				clearerr( fs );
				free( dt );
				return ERBD;
			}

			einfo.len += sizeof( struct dir_entry );
			fseek( fs, - sizeof( struct dir_entry ), SEEK_CUR );
			fwrite( &einfo, sizeof( struct dir_entry ), 1, fs );

			if ( ferror( fs ) ) {
				clearerr( fs );
				free( dt );
				return EWBD;
			}
		}

		free( dt );
	}

	return 0;
}

/** fat_ls 
 * Esegue il listing di path, e lo salva in *list
 * Nota: se si verifica un errore *list non e' mai allocato
 */
int fat_ls( FILE *fs, struct fat_ctrl *f_ctrl, char *path, char** list )
{
	int e, n, i, length;
	struct dir_entry *dt, dir_info;

	e = check_path( path );
	if ( e )
		return EBDP;

	/* carica la directory table di path */
	e = get_dir_entry( fs, f_ctrl, path, &dir_info );
	if ( e )
		return e;

	e = get_dt( fs, f_ctrl, &dir_info, &dt );
	if ( e )
		return e;

	n = dt[0].len / sizeof(struct dir_entry);

	/* primo passaggio, calcola il numero di caratteri */
	for ( i = 0, length = 0; i < n; i++ ) 
		length += strlen( dt[i].name ) + 1;

	*list = malloc( length );
	if ( !( *list ) ) {
		free( dt );
		return STDLIBERR;
	}

	memset( *list, 0x00, length );

	/* secondo passaggio, copia le stringhe */
	for ( i = 0; i < n-1; i++ ) {
		strcat( *list, dt[i].name );
		strcat( *list, "\t" );
	}
	strcat( *list, dt[n-1].name );

	free( dt );

	return 0;

}

/** fat_write
 * path     = <parent>/<file_name>
 * data     = caratteri da scrivere 
 * data_len = numero di caratteri da scrivere
 * Appende la stringa di caratteri data in coda a file_name
 *
 * Sequenza delle operazioni
 *   - Carica la directory table di parent (o della radice se parent == "") ed
 *     individua la entry relativa a file_name (se esiste)
 *   - Alloca se necessario nuovi blocchi per il file
 *   - Trascrive il contenuto di data sul device procedendo per blocchi
 */
int fat_write( FILE *fs, struct fat_ctrl *f_ctrl, char *path, char* data, int data_len )
{
	int e, i, file_idx, n, available, nblocks, block_size;
	unsigned int *iblocks, *fat, curr_block;
	struct dir_entry file_info, parent_info, *dt;

	char *parent, *file_name;
	
	int data_offset, nwrite, bytesleft;
	unsigned int base;

	block_size = f_ctrl->b_sector.block_size;
	
	/* controlla il percorso */
	e = check_path( path );
	if ( e )
		return e;

	/* identifica il nome del file */
	for ( i = strlen(path)-1; i >= 0 && path[i] != '/'; i--)
		;
	file_name = &path[i+1];

	/* carica la directory table dove cercare il file */
	parent = get_parent_path( path );
	if ( !parent )
		return STDLIBERR;

	e = get_dir_entry( fs, f_ctrl, parent, &parent_info );

	free( parent );

	if ( e )
		return e;

	e = get_dt( fs, f_ctrl, &parent_info, &dt );
	if ( e )
		return e;

	n = dt[0].len/sizeof(struct dir_entry);
	for ( i = 2; i < n; i++ ) {
		if ( !strcmp( dt[i].name, file_name ) && dt[i].attr == FILE_ENTRY ) {
			file_info = dt[i];
			break;
		}
	}

	if ( i == n ) { /* file non trovato */
		free( dt );
		return ENSFD;
	}

	/* salva l'indice della entry nella directory table */
	file_idx = i;
	
	/* calcola il numero di blocchi da allocare (se necessario) */
	if ( file_info.len == 0 )
		available = block_size;
	else 
		available = file_info.len%block_size ?
	                block_size - (file_info.len%block_size) : 0;

	if ( available >= data_len ) 
		nblocks = 0; /* non occorre allocare altri blocchi */
	else {
		int additional; /* bytes che richiedono nuovi blocchi */
		additional = data_len - available;
		nblocks = additional / block_size;
		if ( additional % block_size != 0 )
			nblocks++; /* serve un ulteriore blocco per il resto */
	}
	
	fat = f_ctrl->fat_ptr;
	
	/* alloca i blocchi per memorizzare il file */
	if ( nblocks > 0 )
		iblocks = malloc( nblocks*sizeof(unsigned int) );
	else
		iblocks = NULL;
	
	/* se non c'e' spazio per tutto il file non scrive niente */
	for ( i = 0; iblocks && i < nblocks; i++ ) {
		iblocks[i] = get_block( fat, f_ctrl->b_sector.num_block );
		if ( iblocks[i] == LAST_BLOCK ) {
			int j;
			for ( j = 0; j < i; j++ )
				fat[ iblocks[j] ] = BLOCK_FREE;
			free( iblocks );
			free( dt );
			return ENMSD;
		}
	}
	
	/* scrive sul blocco già in uso */
	for ( curr_block = file_info.index; ; curr_block = fat[curr_block] )
		if ( fat[curr_block] == LAST_BLOCK )
			break;
	
	data_offset = 0;
	bytesleft = data_len;
	base = f_ctrl->blk_base;
	
	/* calcola quanto scrivere sul resto del blocco */
	nwrite = MIN( available, data_len );
	/* l'offset sul file deve tener conto dei dati gia' presenti */
	fseek( fs, base+curr_block*block_size+(file_info.len%block_size), SEEK_SET );
	fwrite( data+data_offset, 1, nwrite, fs );
	if ( ferror( fs ) || feof( fs ) ) {
		free( iblocks );
		return EWBD;
	}
	
	/* aggiorna l'offset per data */
	data_offset += nwrite;
	bytesleft -= nwrite;
	
	/* se ci sono altri blocchi da scrivere */
	if ( nblocks > 0 ) {
		fat[curr_block] = iblocks[0]; /* alla prima iterazione curr_block sara'
		il primo dei nuovi blocchi allocati */
		fseek( fs, sizeof(struct boot_sector)+curr_block*sizeof(unsigned int), SEEK_SET );
		fwrite( fat+curr_block, sizeof( unsigned int ), 1, fs );

		if ( ferror( fs ) || feof( fs ) ) {
			free( iblocks );
			return EWFCD;
		}
	
		for ( i = 0; i < nblocks; i++ ) {
			/* rileva il blocco corrente */
			curr_block = fat[curr_block];

			/* scrive i dati sul file system */
			nwrite = MIN( bytesleft, block_size );
			fseek( fs, base+curr_block*block_size, SEEK_SET );
			fwrite( data+data_offset, 1, nwrite, fs );

			if ( ferror( fs ) || feof( fs ) ) {
				free( iblocks );
				return EWBD;
			}
			
			/* aggiorna offset */
			data_offset += nwrite;
			bytesleft -= nwrite;
			
			/* aggiorna la entry della fat, se e' l'ultimo blocco la entry deve
			   essere LAST_BLOCK */
			fat[curr_block] = ( i == nblocks-1 ) ? LAST_BLOCK : iblocks[i+1];
			fseek( fs, sizeof( struct boot_sector )+curr_block*sizeof(unsigned int), SEEK_SET );
			fwrite( fat+curr_block, sizeof( unsigned int ), 1, fs );

			if ( ferror( fs ) || feof( fs ) ) {
				free( iblocks );
				return EWFCD;
			}
		}
		
		free( iblocks );
	}
	
	/* aggiorna la entry del file (il campo len va modificato) */
	dt[file_idx].len += data_len;

	e = write_dir_table( fs, f_ctrl, dt );
	free( dt );
	
	return e;
}


/** fat_read
 * path =     <parent>/<file_name>
 * start =    offset iniziale
 * data_len = numero di caratteri da leggere
 * data =     buffer di lettura
 * Legge data_len caratteri del file file_name a partire da start e li trascrive
 * nel buffer data (gia' allocato)
 *
 * Sequenza delle operazioni
 *   - Carica la directory entry di path
 *   - Calcola il numero di caratteri da leggere effettivo (non legge oltre la
 *     fine del file)
 *   - Legge il contenuto del file procedendo per blocchi
 */
int fat_read( FILE            *fs,
              struct fat_ctrl *f_ctrl,
              char            *path,
              int              start, 
              int              data_len,
              char            *data )
{
	int e, i;
	int totbytes, bytesleft, block_size; /* contatori i/o */
	int data_offset, first_offset, first_bytes;
	int to_read;
	
	unsigned int *fat, curr_block, base;
	struct dir_entry file_info;
	char *buffer;
	
	e = check_path( path );
	if ( e )
		return e;
	
	e = get_dir_entry( fs, f_ctrl, path, &file_info );
	if ( e )
		return e;
		
	if ( start > file_info.len || data_len == 0 )
		/* legge 0 bytes */
		return 0;
	
	/* numero di bytes effettivi da leggere
	   se start+data_len va oltre la fine del file legge solo i bytes 
	   inizializzati, altrimenti legge tutti i bytes richiesti */
	totbytes = bytesleft = ( start+data_len > file_info.len ) ? file_info.len-start : 
	          data_len;
	
	fat = f_ctrl->fat_ptr;
	base = f_ctrl->blk_base;
	block_size = f_ctrl->b_sector.block_size;
	
	/* alloca lo spazio per l'io - trascrive un blocco alla volta */
	buffer = malloc( block_size );
	if ( !buffer )
		return STDLIBERR;
	
	/* assegna a curr_block l'i-esimo blocco da cui iniziare la lettura */
	i = start / block_size;
	for ( curr_block = file_info.index; i > 0 ; curr_block = fat[curr_block] )
		i--;
	
	/* lettura dal primo blocco */

	data_offset = 0;
	first_offset = start % block_size;

	/* bytes nel blocco partendo da first_offset */
	first_bytes = block_size - ( first_offset );
	/* se per caso non e' necessario leggere tutto il blocco
	   legge solo bytes_left bytes */
	to_read = MIN( first_bytes, bytesleft);

	fseek( fs, base + curr_block*block_size + first_offset, SEEK_SET );
	fread( buffer, 1, to_read, fs );
	if ( ferror( fs ) || feof( fs ) ) {
		clearerr( fs );
		free( buffer );
		return ERBD;
	}
	memcpy( data+data_offset, buffer, to_read );
	data_offset += to_read;
	bytesleft -= to_read;

	/* lettura dagli eventuali blocchi successivi */

	while ( bytesleft > 0 && (curr_block = fat[curr_block]) != LAST_BLOCK ) {
		to_read = MIN( block_size, bytesleft );
		fseek( fs, base+curr_block*block_size, SEEK_SET );
		fread( buffer, 1, to_read, fs );
		if ( ferror( fs ) || feof( fs ) ) {
			clearerr( fs );
			free( buffer );
			return ERBD;
		}
		memcpy( data+data_offset, buffer, to_read );
		data_offset += to_read;
		bytesleft -= to_read;
	}
	
	free( buffer );
	
	return totbytes - bytesleft;
}

/** fat_cp
 * from = file di origine
 * to   = <to_parent>/<to_name> copia da creare
 * Crea il file to e vi copia il contenuto di from
 *
 * Sequenza delle operazioni
 *   - Recupera la directory entry di from
 *   - Alloca spazio per un file delle stesse dimensioni
 *   - Carica la directory table di to_parent (o della radice se to_parent=="")
 *   - Aggiunge la entry to_name alla directory table
 *   - Copia il contenuto di from in to blocco per blocco
 *   - Trascrive la directory table di parent
 *   - Aggiorna i riferimenti a to_parent, che ora ha una entry in piu'
 */
int fat_cp( FILE *fs, struct fat_ctrl *f_ctrl, char *from, char *to )
{
	int i, e, nblocks, block_size, n;
	struct dir_entry from_info, to_parent_info, *dt;
	unsigned int *iblocks, *fat;
	unsigned int extended_block;
	char *to_parent, *to_name;
	
	if ( (e = check_path( from )) ||
	     (e = check_path( to )) ||
	     /* recupera le informazioni del file sorgente */
	     (e = get_dir_entry( fs, f_ctrl, from, &from_info )) )
		return e;
	
	block_size = f_ctrl->b_sector.block_size;
	fat = f_ctrl->fat_ptr;
	
	/* alloca i blocchi per la copia - alloca un blocco
	   anche se il file e' vuoto */
	nblocks = from_info.len / block_size;
	if ( from_info.len % block_size || !nblocks )
		nblocks++;
		
	iblocks = malloc( nblocks*sizeof(unsigned int) );
	
	for ( i = 0; i < nblocks; i++ ) {
		iblocks[i] = get_block( fat, f_ctrl->b_sector.num_block );
		if ( iblocks[i] == LAST_BLOCK ) {
			int j;
			for ( j = 0; j < i; j++ )
				fat[ iblocks[j] ] = BLOCK_FREE;
			free( iblocks );
			return ENMSD;
		}
	}
	
	/* carica la dt che ospita il nuovo file */
	to_parent = get_parent_path( to );
	if ( !to_parent ) {
		free( iblocks );
		return STDLIBERR;
	}

	e = get_dir_entry( fs, f_ctrl, to_parent, &to_parent_info );
	if ( e ) {
		free( to_parent );
		free( iblocks );
		return e;
	}

	e = get_dt( fs, f_ctrl, &to_parent_info, &dt );
	if ( e ) {
		free( to_parent );
		free( iblocks );
		return e;
	}
	
	/* controlla che non esista una entry con lo stesso nome */
	for ( i = strlen( to )-1; i >= 0 && to[i] != '/'; i-- )
		;
	to_name = &to[i+1];
	
	n = to_parent_info.len / sizeof(struct dir_entry);
	e = !(*to_name) ? EDAEX : 0;
	for ( i = 0; i < n && !e; i++ ) {
		if ( strcmp( dt[i].name, to_name ) == 0 )
			e = EDAEX;
	}
	
	if ( e ) {
		free( dt );
		free( iblocks );
		free( to_parent );
		return e;
	}
	else { /* crea la nuova entry */
		struct dir_entry *dt_;
		int bytesleft;
		int root;
		unsigned int f_idx, t_idx, base, idx;
		char *buffer;
		
		root = !strcmp( to_parent, "/" );
		
		bytesleft = to_parent_info.len%block_size ?
		            block_size - (to_parent_info.len%block_size) : 0;
		
		/* se serve un nuovo blocco per la directory table to_parent (dt) */
		if ( bytesleft < sizeof(struct dir_entry) ) {
			extended_block = get_block( fat, f_ctrl->b_sector.num_block );
			if ( extended_block == LAST_BLOCK ) {
				for ( i = 0; i < nblocks; i++ )
					fat[ iblocks[i] ] = BLOCK_FREE;
				free( dt );
				free( iblocks );
				free( to_parent );
				return ENMSD;
			}
		}
		else
			extended_block = LAST_BLOCK;
		
		/* aggiorna la dimensione della dt */
		dt[0].len += sizeof(struct dir_entry);
		if ( root )
			dt[1].len = dt[0].len;
		
		dt_ = dt;
		dt = realloc( dt, dt[0].len );
		if ( !dt ) {
			free( dt_ );
			free( iblocks );
			free( to_parent );
			return STDLIBERR;
		}

		memset( &dt[n], 0x00, sizeof(struct dir_entry) );
		dt[n].used = DIR_ENTRY_BUSY;
		memcpy( dt[n].name, to_name, strlen(to_name)+1 );
		dt[n].attr = FILE_ENTRY;
		dt[n].index = iblocks[0];
		dt[n].len = from_info.len; /* file vuoto */
		
		
		/* se e' stato allocato un nuovo blocco per la directory
		   table che ospita il file, la fat va aggiornata */
		if ( extended_block != LAST_BLOCK ) {
			for ( idx = dt[0].index; ; idx = fat[idx] )
				if ( fat[idx] == LAST_BLOCK )
					break;

			fat[idx] = extended_block; /* punta al blocco appena assegnato */
			fseek( fs, sizeof(struct boot_sector)+idx*sizeof(unsigned int), SEEK_SET );
			fwrite( fat+idx, sizeof(unsigned int), 1, fs );

			fat[extended_block] = LAST_BLOCK; /* il blocco assegnato e' l'ultimo */
			fseek( fs, sizeof(struct boot_sector)+extended_block*sizeof(unsigned int), SEEK_SET );
			fwrite( fat+extended_block, sizeof(unsigned int), 1, fs );
		}

		if ( ferror( fs ) || feof( fs ) ) {
			clearerr( fs );
			free( dt );
			free( iblocks );
			free( to_parent );
			return EWFCD;
		}
		
		/* copia il file blocco per blocco */
		
		/* indici iniziali */
		base = f_ctrl->blk_base;
		f_idx = from_info.index; /* indice FAT from */
		t_idx = dt[n].index;     /* indice FAT to */
		
		buffer = malloc( block_size );
		if ( !buffer ) {
			free( to_parent );
			free( dt );
			free( iblocks );
			return STDLIBERR;
		}
		
		for ( i = 0; i < nblocks; i++ ) {			
			/* legge il blocco corrente */
			fseek( fs, base+f_idx*block_size, SEEK_SET );
			fread( buffer, 1, block_size, fs );
			
			if ( ferror( fs ) || feof( fs ) ) {
				clearerr( fs );
				free( buffer );
				free( to_parent );
				free( dt );
				free( iblocks );
				return ERBD;
			}
			
			/* scrive il blocco corrente */
			fseek( fs, base+t_idx*block_size, SEEK_SET );
			fwrite( buffer, 1, block_size, fs );
			
			if ( ferror( fs ) || feof( fs ) ) {
				clearerr( fs );
				free( buffer );
				free( to_parent );
				free( dt );
				free( iblocks );
				return EWBD;
			}
			
			/* aggiorna la fat */
			fat[t_idx] = ( i == nblocks-1 ) ? LAST_BLOCK : iblocks[i+1];
			fseek( fs, sizeof( struct boot_sector )+t_idx*sizeof(unsigned int), SEEK_SET );
			fwrite( fat+t_idx, sizeof( unsigned int ), 1, fs );

			if ( ferror( fs ) || feof( fs ) ) {
				clearerr( fs );
				free( buffer );
				free( dt );
				free( iblocks );
				free( to_parent );
				return EWFCD;
			}
			
			/* percorre la FAT */
			f_idx = fat[f_idx];
			t_idx = fat[t_idx];
			
		}
		
		free( iblocks );
		free( buffer );
		
		/* la directory table che ospita il nuovo file va trascritta sul file
		   system */
		if ( (e = write_dir_table( fs, f_ctrl, dt )) ||
		/* e va aggiornata la sua dimensione nella relativa entry al
		   livello superiore */
		     ((!root) && (e = update_entry( fs, f_ctrl, to_parent, dt[1], dt[0].len))) )
		{
			free( dt );
			free( to_parent );
			return e;
		}

		free( to_parent );

		/* aggiorna le entry ".." relative alle subdir di to_parent, visto che
		   ora to_parent ha una entry in piu' */
		for ( i = 2; i < n; i++ ) {
			struct dir_entry einfo;
			size_t dd_offt;

			if ( dt[i].attr != SUB_ENTRY )
				continue;

			dd_offt = f_ctrl->blk_base + ( dt[i].index * block_size ) + sizeof( struct dir_entry );

			fseek( fs, dd_offt, SEEK_SET );
			fread( &einfo, sizeof( struct dir_entry ), 1, fs );

			if ( ferror( fs ) || feof( fs ) ) {
				free( dt );
				return ERBD;
			}

			einfo.len += sizeof( struct dir_entry );
			fseek( fs, - sizeof( struct dir_entry ), SEEK_CUR );
			fwrite( &einfo, sizeof( struct dir_entry ), 1, fs );

			if ( ferror( fs ) || feof( fs ) ) {
				free( dt );
				return EWBD;
			}
		}

		free( dt );
		
	}
	
	return 0;
}

/* -- IMPLEMENTAZIONE DELLE FUNZIONI DI NAVIGAZIONE ------------------------- */
/* -------------------------------------------------------------------------- */

/** get_dt
 * Recupera la directory table della cartella denotata da entry_info e 
 * ne assegna l'indirizzo a *dt
 * la directory table e' un array di struct dir_entry allocato sullo heap
 * Restituisce 0 se ha successo, oppure un codice opportuno in caso di errore
 */
static int get_dt( FILE             *fs,
	               struct fat_ctrl  *f_ctrl,
	               struct dir_entry *entry_info,
	               struct dir_entry **dt )
{
	struct dir_entry *dir_table;
	unsigned int *fat, curr_block, offt, to_read;
	int i, block_size;

	fat = f_ctrl->fat_ptr;
	block_size = (f_ctrl->b_sector).block_size;
	curr_block = entry_info->index;

	/* se non e' una directory */
	if ( entry_info->attr != SUB_ENTRY )
		return ENSFD;
	
	/* alloca lo spazio per memorizzare la directory table */
	dir_table = malloc( entry_info->len );
	if ( !dir_table )
		return STDLIBERR;
	i = 0; /* offset su dir_table nelle letture successive */
	to_read = entry_info->len;
	
	offt = curr_block*block_size;

	/* legge la directory table per intero */
	if ( to_read <= block_size ) {
		/* singolo blocco */
		fseek( fs, f_ctrl->blk_base + offt, SEEK_SET );
		i = fread( dir_table, 1, entry_info->len, fs );
		if ( ferror( fs ) ) {
			free( dir_table );
			clearerr( fs );
			return ERBD;
		}
	}
	else do {
		/* DT distribuita su vari blocchi */
		unsigned int n, read;
		/* se l'ultimo blocco legge solo quanto necessario */
		n = MIN( to_read, block_size);
		fseek( fs, f_ctrl->blk_base + offt, SEEK_SET );
		/* legge i prossimi n bytes della directory table*/
		i += read = fread( ((char*) dir_table)+i, 1, n, fs );
		if ( ferror( fs ) ) {
			free( dir_table );
			clearerr( fs );
			return ERBD;
		}
		/* sposta il puntatore sul prossimo blocco */
		curr_block = fat[curr_block];
		offt = curr_block*block_size;
		to_read -= read;
	} while ( curr_block != LAST_BLOCK );

	*dt = dir_table;

	return 0;
}

/** get_dir_entry
 * Individua la directory entry puntata da path e la copia in *retval
 * Restituisce 0 se ha successo, ENSFD se un elemento di path non esiste
 * oppure un codice opportuno in caso di errore
 */
static int get_dir_entry( FILE             *fs,
	                      struct fat_ctrl  *f_ctrl,
	                      const char       *path,
	                      struct dir_entry *retval )
{
	struct dir_entry entry_info;
	struct dir_entry *dir_table;
	unsigned int block_size;
	int i, num_entries, e;
	char *copy, *entry_name, *saveptr;
	char found;
	
	if ( path[0] != '/' ) /* non dovrebbe essere possibile */
		return EBDP;
	
	copy = strdup( path );
	if ( !copy )
		return STDLIBERR;
	
	/* si inizia dalla root */
	block_size = f_ctrl->b_sector.block_size;

	entry_name = strtok_r( copy, "/", &saveptr );

	/* recupera i metadati della cartella di root */

	/* all'inizio della directory table */
	fseek( fs, f_ctrl->blk_base + ROOT_IDX*block_size, SEEK_SET );
	/* legge le info della entry "." */
	fread( &entry_info, sizeof( struct dir_entry ), 1, fs );

	/* se entry_name == NULL => path == "/" e restituisce la root entry */
	if ( entry_name == NULL ) {
		free( copy );
		*retval = entry_info;
		return 0;
	}

	/* altrimenti la sequenza delle operazioni e'
	    - Recupera la directory table di entry_info (inizialmente e' la root)
	    - Cerca tra le entry il prossimo elemento del path (next_entry)
	    - entry_info = next_entry */
	do {
		if ( entry_info.attr != SUB_ENTRY ) {
			/* errore */
			free( copy );
			return ENSFD;
		}

		e = get_dt( fs, f_ctrl, &entry_info, &dir_table );
		if ( e ) {
			free( copy );
			return e;
		}

		num_entries = entry_info.len / sizeof(struct dir_entry);
		found = 0;
		/* scorre la directory table per trovare la entry cercata */
		for ( i = 0; i < num_entries; i++ ) {
			if ( strcmp( dir_table[i].name, entry_name ) == 0 ) {
				entry_info = dir_table[i];
				found = 1;
				break;
			}
		}

		free( dir_table );

		if ( !found ) {
			/* la entry per questa iterazione non esiste */
			free( copy );
			return ENSFD;
		}

	} while ( (entry_name=strtok_r( NULL, "/", &saveptr)) != NULL );

	free( copy );
	/* copia la entry nel parametro passato per riferimento */
	*retval = entry_info;
	return 0;
		
}

/** get_parent_path
 * restituisce una stringa allocata dinamicamente contenente il percorso
 * di path troncato al penultimo livello
 */
static char *get_parent_path( const char *path )
{
	int i;
	char *parent;
	
	for( i = strlen( path )-1; i >= 0 && path[i] != '/'; i-- )
		;
	
	if ( i == 0 ) /* parent e' la root e i punta al backslash iniziale */
		i++;

	parent = malloc( i+1 );
	if ( !parent )
		return NULL;

	memcpy( parent, path, i );
	parent[i] = '\0';
	
	return parent;
}

/** get_block
 * restituisce l'indice di un blocco libero nella FAT se questo esiste,
 * LAST_BLOCK (che non e' un indice valido) altrimenti
 */
static unsigned int get_block( unsigned int *fat, unsigned int size )
{
	unsigned int i;
	for ( i = ROOT_IDX+1; i < size; i++ )
		if ( fat[i] == BLOCK_FREE ) {
			fat[i] = !BLOCK_FREE; /* segnaposto per chiamate in successione */
			return i;
		}
	return LAST_BLOCK;
}

/** write_dir_table
 * Scrive sul device la directory table dt
 * Restituisce 0 se la scrittura riesce, EWBD se errore
 */
static int write_dir_table( FILE             *fs,
	                        struct fat_ctrl  *f_ctrl,
	                        struct dir_entry *dt )
{
	int n, i, bytes_written, block_size;
	unsigned int *fat, base, block;

	n = dt[0].len;                            /* bytes totali da scrivere */
	fat = f_ctrl->fat_ptr;
	base = f_ctrl->blk_base;
	block_size = f_ctrl->b_sector.block_size;
	i = 0;

	for ( block = dt[0].index; block != LAST_BLOCK ; block = fat[block] ) {
		int to_write = ( n > block_size ) ? block_size : n;
		unsigned int offt = block*block_size;

		fseek( fs, base+offt, SEEK_SET );
		bytes_written = fwrite( ((char*) dt)+i, 1, to_write, fs );
		if ( ferror( fs ) ) {
			clearerr( fs );
			return EWBD;
		}

		n -= bytes_written;
		i += bytes_written;
	}

	return 0;
}

/* Recupera le info del penultimo livello di path, e aggiorna la relativa entry
   con il campo len del parametro info
   Restituisce 0 se ha successo, oppure un codice di errore */
static int update_entry( FILE             *fs, 
	                     struct fat_ctrl  *f_ctrl,
	                     const char       *path,
	                     struct dir_entry  parent_info,
	                     unsigned int      len )
{
	struct dir_entry *dt;
	const char *entry_name;
	int n, i, e;

	/* nome della entry (ultimo elemento di path) */
	for ( entry_name = path+strlen(path); *entry_name != '/'; entry_name-- )
		;
	entry_name++;

	/* dt del penultimo livello */
	e = get_dt( fs, f_ctrl, &parent_info, &dt );
	if ( e )
		return e;


	/* ricerca della entry - si assume che esista visto che path e' stato gia'
	   percorso da qualche chiamata a get_dir_entry precedentemente effettuata
	   dal chiamante */
	n = dt[0].len/sizeof(struct dir_entry);
	for ( i = 0; i < n && strcmp( dt[i].name, entry_name ) != 0; i++ ) 
		;

	/* update delle informazioni */
	dt[i].len = len;
	e = write_dir_table( fs, f_ctrl, dt );
	free( dt );

	return e;
}
