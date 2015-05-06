/**  
 *  \file 
 *  \author sol
 *  \brief libreria di comunicazione server client
 *
 *  Libreria che definisce l'interfaccia di comunicazione fra client e server
 *  (canali di comunicazione realizzati con socket AF_UNIX) 
*/

#ifndef SOL_COM_H
#define SOL_COM_H

/* -= TIPI =- */

#include "com_defs.h"

/* -= FUNZIONI =- */
/** Crea un channel di ascolto AF_UNIX
 *  \param  path pathname del canale di ascolto da creare
 *  \return
 *     - s    il descrittore del channel di ascolto  (s>0)
 *     - -1   in casi di errore (sets errno)
 *     - SFATENAMETOOLONG se il nome del socket eccede UNIX_PATH_MAX
 */
serverChannel_t createServerChannel(const char* path);

/** Distrugge un channel di ascolto
 *  \param s il channel di ascolto da distruggere
 *  \return
 *     -  0  se tutto ok, 
 *     - -1  se errore (sets errno)
 */
int closeSocket(serverChannel_t s);

/** Accetta una connessione da parte di un client
 *  \param  s channel di ascolto su cui si vuole ricevere la connessione
 *  \return
 *     - c il descrittore del channel di trasmissione con il client 
 *     - -1 in casi di errore (sets errno)
 */
channel_t acceptConnection(serverChannel_t s);

/** Legge un messaggio dal channel di trasmissione
 *  \param  sc  channel di trasmissione
 *  \param msg  struttura che conterra' il messagio letto 
 *		(deve essere allocata all'esterno della funzione,
 *		tranne il campo buffer)
 *  \return
 *     -  -1    se errore (errno settato),
 *     -  SEOF  se EOF sul socket 
 *     -  lung  lunghezza del buffer letto, se OK
 */
int receiveMessage(channel_t sc, message_t *msg);

/** Scrive un messaggio sul channel di trasmissione
 *   \param  sc channel di trasmissione
 *   \param msg struttura che contiene il messaggio da scrivere 
 *   \return
 *      -  -1   se errore (sets errno) 
 *      -  n    il numero di caratteri inviati (se OK)
 */
int sendMessage(channel_t sc, message_t *msg);

/** Chiude un socket
 *  \param  sc il descrittore del channel da chiudere 
 *  \return
 *     - 0  se tutto ok
 *     - -1 se errore (sets errno)
*/
int closeConnection(channel_t sc);

/** Crea un channel di trasmissione verso il server
 *  \param  path  nome del server socket
 *  \return
 *     -  c (c>0) il channel di trasmissione 
 *     - -1 in casi di errore (sets errno)
 *     - FATSENAMETOOLONG se il nome del socket eccede UNIX_PATH_MAX
 */
channel_t openConnection(const char* path);

#endif /* SOL_COM_H */
