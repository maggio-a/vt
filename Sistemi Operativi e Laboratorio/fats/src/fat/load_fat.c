/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

/* load_fat.c -- montaggio di un file system */

#include <stdio.h>
#include <stdlib.h>

#include "load_fat.h"

/** mount_fat
 * mounting del file system dal device fs
 */
int mount_fat(FILE *fs, struct fat_ctrl *f_ctrl)
{
	struct boot_sector *bs;
	unsigned int n;          /* Misc */

	rewind( fs );

	bs = &( f_ctrl->b_sector );

	/* Recupera le informazioni del boot sector */
	n = fread( bs, sizeof( struct boot_sector ), 1, fs );
	if ( ferror( fs ) || n < 1 || bs->fs_type != FAT_TYPE )
		return ERFCD;

	/* Alloca la memoria per la FAT */
	f_ctrl->fat_ptr = ( unsigned int * ) malloc( ( bs->num_block )*sizeof( unsigned int ) );
	if ( f_ctrl->fat_ptr == NULL )
		return STDLIBERR;

	/* Copia la FAT nell'array allocato */
	n = fread( f_ctrl->fat_ptr, sizeof( unsigned int ), bs->num_block, fs );
	if ( ferror( fs ) || n < bs->num_block )
		return ERFCD;

	/* Calcola l'offset del primo blocco dati */
	n = sizeof( struct boot_sector );
	n += ( bs->num_block )*sizeof( unsigned int );
	f_ctrl->blk_base = n;

	return 0;
}