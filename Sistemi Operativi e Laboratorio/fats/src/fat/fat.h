/**
 * \file 
 * \brief Strutture di base per libfat
 * \author sol
 */

#ifndef FAT_H
#define FAT_H

#include "fat_defs.h"

/**
 * <H3>Boot Sector</H3>
 * La struttura \c boot_sector ha il compito di memorizzare le informazioni 
 * contenute nel boot sector. Tale informazioni sono:
 * - \c fs_type: un byte che indica il tipo di file system con cui il
 *   device e' stato formattato. Per il filesystem FAT il valore del byte e' FAT_TYPE.
 * - \c  block_size: rappresenta la dimensione di un blocco in byte.
 * - \c num_block: rappresenta il numero di blocchi disponibili sul device. Esiste una entry nella FAT
 *   per ciascun blocco presente nella Data Region.
 *
 * 
 * <HR>
 *
 */
struct boot_sector {
    char fs_type;              /**< tipo del File system */
    int block_size;            /**< Block Size (bytes) */
    unsigned int num_block;    /**< Numero di blocchi */
    char usr[FAT_MAXUSRLEN];   /* owner */
};

/**
 * <H3>Directry Table Entry</H3> 
 * La struttura \c dir_entry rappresenta le informazioni contenute in ciascuna entry
 * della Directory Table. In particolare:
 *
 * - \c used: indica se la entry contiene dati sogificativi. Questo byte contiene  DIR_ENTRY_FREE se 
 *   la entry e' libera,  DIR_ENTRY_BUSY altrimenti.
 * - \c name: indica il nome del file o della directory. Il nome del file puo' essere
 *   lungo al piu' MAX_LEN_NAME caratteri.
 * - \c attr: indica se la entry e' relativa ad un file o una directory. In particolare se il valore 
 *   di questo campo e' FILE_ENTRY si tratta di un file, se il valore e' SUB_ENTRY si tratta di una directory.
 * - \c index: rappreenta l'indice del blocco a partire da cui il file o la directory e' memorizzata.
 * - \c len: rappresenta la lunghezza del file o della directory. 
 * 
 * <HR>
 *
 */
struct dir_entry {
    char used;                    /**< DIR_ENTRY_FREE se disponibile, DIR_ENTRY_BUSY altrimenti */
    char name[MAX_LEN_NAME + 1];  /**< File/Directory name. Lunghezza massima MAX_LEN_NAME */
    char attr;                    /**< Una directory entry puo' essere un file (FILE_ENTRY) o una directory (SUB_ENTRY) */ 
    unsigned int index;           /**< Blocco da cui inizia il file/directory cuisi riferisce la entry */
    unsigned int len;             /**< File/directory size (in byte)*/
};

/**
 * <H3>Struttura di controllo principale di un file system </H3> 
 * La struttura dati \c fat_ctrl, contiene tutte le
 * informazioni necessarie a gestire il file system
 * FAT. In particolare:
 *
 * - \c b_sector: contiene le informazioni del boot sector.
 * - \c fat_ptr: puntatore alla File Allocation Table. 
 * - \c blk_base: indica il byte in cui e' memorizzato il 
 *                blocco 0 (zero), 
 */
struct fat_ctrl {
    struct boot_sector b_sector;   /**< Informazioni del boot sector. */
    unsigned int *fat_ptr;         /**< File Allocation Table (FAT) array. */
    unsigned int blk_base;         /**< Offset del primo blocco dati. */
};

#endif /* FAT_H */
