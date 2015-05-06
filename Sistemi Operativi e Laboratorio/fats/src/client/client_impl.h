/*
================================================================================
Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
Universita' di Pisa - Informatica Applicata
Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
2013

Il contenuto di questo file e' interamente ad opera dell'autore.
================================================================================
*/

#ifndef CLIENT_IMPL_H
#define CLIENT_IMPL_H

#include "../com/com_defs.h"

#define MAXLINE      65536   /* massima lunghezza dell'input */
#define QUIT         "quit" /* comando di uscita */

#define CMD_MKDIR    "mkdir"
#define CMD_LS       "ls"
#define CMD_MKFILE   "mkfile" 
#define CMD_FREAD    "fread" 
#define CMD_FWRITE   "append" 
#define CMD_CP       "cp"

int   do_handshake( char *usrname, char **devname );
void *server_listener( void *arg );
void  handle_input();
void  get_line( char *input, int limit );
int   parse_cmd( char *input, message_t *msg );

#endif /* CLIENT_IMPL_H */
