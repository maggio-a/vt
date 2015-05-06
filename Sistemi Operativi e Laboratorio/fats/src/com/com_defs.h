/**
 * \file
 * \author sol
 * \brief  Strutture dati e definizioni per lo scambio messaggi
 */

#ifndef COM_DEFS_H
#define COM_DEFS_H

/** <H3>Messaggio</H3>
 * La struttura \c message_t rappresenta un messaggio tra client e server
 * - \c type rappresenta il tipo del messaggio
 * - \c length rappresenta la lunghezza in byte del campo buffer
 * - \c buffer e' il puntatore al messaggio
 *
 * <HR>
 */

typedef struct {
    char type;           /**< tipo del messaggio */
    unsigned int length; /**< lunghezza in byte */
    char* buffer;        /**< buffer messaggio */
} message_t; 

/** fine dello stream, connessione chiusa dal peer */
#define SEOF -2

/** tipo dei messaggi di errore */
#define MSG_ERROR        'E' 
/** tipo dei messaggi di OK */
#define MSG_OK           '0' 

/** tipo dei messaggi di richiesta connessione */
#define MSG_CONNECT      'C' 
/** tipo dei messaggi di creazione directory */
#define MSG_MKDIR        'D' 
/** tipo dei messaggi di listing directory */
#define MSG_LS           'L' 
/** tipo dei messaggi di creazione file */
#define MSG_MKFILE       'F' 
/** tipo dei messaggi di creazione file */
#define MSG_FREAD        'R' 
/** tipo dei messaggi di creazione file */
#define MSG_FWRITE       'W' 

#define MSG_CP           'c'

#define FATSENAMETOOLONG -12 /**< Error Path Too Long (exceeding UNIX_PATH_MAX) */
#define FATSPRESENT -13      /**< Error Cannot create FS: already exists */

/** nome del server socket AF_UNIX */
#define SKTNAME "sfatsock" 

/** directory del server socket */
#define TMP "./tmp/" 


/** tipo descrittore canale di ascolto (server) */
typedef int serverChannel_t;

/** tipo descrittore del canale di comunicazione (server e client) */
typedef int channel_t;

#endif /* COM_DEFS_H */
