/**
 * \file
 * \brief Mounting di un file system fat 
 * \author sol
 */

#ifndef LOAD_FAT_H
#define LOAD_FAT_H

#include "fat.h"

/**
 * Questa funzione si occupa di leggere le informazioni
 * di controllo del filesystem FAT.
 * \param fs il FILE pointer che consente di acceddre al
 *           file in cui il file system e' memorizzato.
 * \param f_ctrl il puntatore alla struttura dove memorizzare
 *               le informazioni di controllo
 * \return 
 *         - ERFCD se si verifica un errore durante la 
 *           lettura delle strutture dati utilizzati 
 *           da FATS.
 *         - 0 in caso di successo
 */
int mount_fat(FILE *fs, struct fat_ctrl *f_ctrl);

#endif /* LOAD_FAT_H */
