/**
 * \file 
 * \brief definizioni di base per libfat
 * \author sol
 */

#ifndef FAT_DEFS_H
#define FAT_DEFS_H

#define FAT_TYPE 0x46          /**< Identificativo di un file system di tipo FAT, corrisponde al carattere \e F. */
#define ROOT_IDX 0             /**< Indice del blocco in cui e' memorizzata la root del file system */
#define BLOCK_FREE 0x00000000  /**< Identificativo blocco libero (nella FAT) */
#define LAST_BLOCK 0xFFFFFFFF  /**< Identificativo ultimo blocco di un file/directory (nella FAT). */
#define INIT_BLOCK 0x00        /**< Carattere di inizializzazione dei blocchi nel device (i.e ogni byte del blocco e' inizializzato a questo valore) */
#define DIR_ENTRY_FREE 0x00    /**< Directory entry disponibile (nella directory table). */
#define DIR_ENTRY_BUSY 0x01    /**< Directory entry occupata (nella directory table) */
#define MAX_LEN_NAME 8         /**< Lunghezza massima dei nomi di file/directory. */
#define FILE_ENTRY 0x02        /**< La directory entry e` relativa a un file (nella directory table). */
#define SUB_ENTRY 0x03         /**< La directory entry e` relativa a una sottodirectory (nella directory table). */

/*
 * codici di errore di SFAT 
 */

#define STDLIBERR -1 /**< Error in lib function (errno setted) */
#define ENTL   -2  /**< Error Name Too Long (exceeding MAX_LEN_NAME) */
#define ERFCD  -3  /**< Error Reading Fat Control Data */
#define ERBD   -4  /**< Error Reading Block Data */
#define ENSFD  -5  /**< Error Not Such File or Directory */
#define EDAEX  -6  /**< Error Directory Already Exists */
#define ENMSD  -7  /**< Error No More Space on Device  */
#define EWFCD  -8  /**< Error Writing Fat Control Data */
#define EWBD   -9  /**< Error Writing Block Data */
#define EBDP   -10 /**< Error Bad Directory Path */
#define ECCS   -11 /**< Error Creating Connection Socket */

#define FAT_MAXUSRLEN 16
#define EDNF     -30 /* errore device non trovato */
#define INTSVERR -40 /* errore interno del server (STDLIBERR durante un'operazione) */

#endif /* FAT_DEFS_H */
