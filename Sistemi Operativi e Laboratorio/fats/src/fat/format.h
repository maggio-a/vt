/** 
 *  \file 
 *  \brief Creazione e formattazione del file system FATS
 *  \author sol
 */

#ifndef FORMAT_H
#define FORMAT_H

#include <stdio.h>

/**
 * Questa funzione ha il compito di formattare un file con il formato
 * FATS. In particolare deve realizzare le seguenti operazioni:
 *
 *
 * - Scrittura dei dati nel Boot Sector.
 * - Inizializzazione della File Allocation Table.
 * - Inizializzazione della Directory Table.
 *
 * \param fs puntatore al file che conterra' il file system e che deve quindi essere formattato
 * \param usrname nome utente del proprietario
 * \param block_size size di ciascun blocco (in byte)
 * \param num_block numero di blocchi disponibili
 * \return 
 *         - EWFCD se si verifica un errore durante la scrittura
 *           delle strutture dati utilizzate dalla FATS.
 *         - 0 se la formattazione si e' conclusa con successo
 */
int fat_format( FILE *fs,
                const char *usrname,
                int block_size,
                unsigned int num_block);

#endif /* FORMAT_H */
