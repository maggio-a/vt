/** 
 *  \file
 *  \author sol
 *  \brief Stampa degli errori. 
*/

#ifndef FAT_ERROR_H
#define FAT_ERROR_H

/**
 * Stampa su stderr un messaggio di errore 
 * \param err codice di errore da stampare: 
 *            se err==STDLIBERR, errno e' stato settato 
 *            invoca perror, altrimenti stampa messaggi di errore per gli
 *            errori definiti in fat_defs.h
*/
void fatserror( int err );

#endif /* FAT_ERROR_H */
