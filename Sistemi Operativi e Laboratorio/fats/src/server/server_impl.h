/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

#ifndef SERVER_IMPL_H
#define SERVER_IMPL_H

#include <stdio.h>

#include "../fat/fat.h"
#include "../com/com_defs.h"

int server_handshake( channel_t com );

/* -- TABELLA DEI FILE SYSTEM ----------------------------------------------- */
struct fstab_st {
	int numfiles;                /* numero di file system montati    */
	int tabsize;                 /* dimensione della tabella         */
	struct file_handle *fsystem; /* ptr alla tabella dei file system */
};

/* -- FILE SYSTEM HANDLING -------------------------------------------------- */
struct file_handle {
	char *name;             /* nome del device        */
	FILE *fs;               /* puntatore al device    */
	struct fat_ctrl fsctrl; /* struttura di controllo */

	/* supporto all'implementazione dei lock di lettura-scrittura */
	int num_readers;
	pthread_mutex_t num_mutex;
	pthread_mutex_t write_mutex;
};

void add_to_index( const char *fs_path, const char *name );
int  index_devices( const char *dir_path );
int  lookup_request( message_t *msg );

/* -- THREADS --------------------------------------------------------------- */
void *client_handler( void *arg );
void *sig_handler( void *arg );
void  cl_cleanup( void *arg );

/* -- LOCKS LETTORI-SCRITTORI ----------------------------------------------- */
void read_lock( struct file_handle *fh );
void read_unlock( struct file_handle *fh );
void write_lock( struct file_handle *fh );
void write_unlock( struct file_handle *fh );

/* -- FUNZIONI -------------------------------------------------------------- */
int execute( struct file_handle *f_handle, channel_t com, message_t *in_msg );

int handle_mkdir_request(
		struct file_handle *f_handle, message_t *in_msg, message_t *out_msg );

int handle_mkfile_request(
		struct file_handle *f_handle, message_t *in_msg, message_t *out_msg );

int handle_ls_request(
		struct file_handle *f_handle, message_t *in_msg, message_t *out_msg );

int handle_fwrite_request(
		struct file_handle *f_handle, message_t *in_msg, message_t *out_msg );

int handle_fread_request(
		struct file_handle *f_handle, message_t *in_msg, message_t *out_msg );

int handle_cp_request(
		struct file_handle *f_handle, message_t *in_msg, message_t *out_msg );

#endif /* SERVER_IMPL_H */