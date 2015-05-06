/**
 * \file
 * \brief Operazioni eseguibili dal client.
 * \author sol
 */

#ifndef OPS_FAT_H
#define OPS_FAT_H

#include "fat.h"

/**
 * Questa funzione effettua l'operazione di \e listing di una directory.  In
 * particolare ha il compito di generare la lista dei file e subdirectory
 * contenute all'interno di una directory. La lista e' rappresentata da una
 * stringa in cui i nomi dei file sono separati dal carattere \c '\\t'.  La
 * lista non viene esplicitamente ordinata, ma i nomi appaiono nell'ordine in
 * cui sono letti dalla directory table delle directory su cui si sta
 * effettuando l'operazione di listing.
 *
 * \param fs il FILE pointer che consente di acceddere al
 *           file dove il filesystem e' memorizzato.
 * \param f_ctrl puntatore alla struttura che mantiene tutte 
 *               le informazioni di controllo del filesystem.
 * \param path il path assoluto della directory di cui dobbiamo
 *             effettuare il listing.
 * \param list [OUT] indirizzo di un puntatore. Dopo l'elaborazione conterra' 
 *             riferimento a una stringa contenente tutti
 *             i nomi dei files e delle subdirectory contenuti
 *             nella directory definita da \a path. I nomi 
 *             sono separati dal carattere \\t (tab). 
 * \returns 
 *         - EBDP (Error Bad Directory Path) se il path specificato
 *           non e' assoluto, ossia non inizia con il carattere '/'
 *         - ERFCD (Error Reading Fat Control Data) se si verifica 
 *           un errore leggendo le strutture di controllo del
 *           file system FATS.
 *         - ENSFD (Error Not Such File or Directory) se un elemento del
 *           pah non siste o non e' una directory.
 *         - ERDB (Error Reading Block Data) se si verifica un errore
 *            di lettura di un blocco.
 *         - 0 in caso di sucesso.
 */
int fat_ls(FILE *fs, struct fat_ctrl *f_ctrl, char *path, char** list);

/**
 * Questa funzione ha il compito di creare una subdirectory \e SUBDIR
 * all'interno di una directory \e DIR. Il pathname con cui si effettua
 * la creazione della directory e' formato da \e DIR/SUBDIR. Ad esempio
 * il pathname \c /this/is/a/path/name/example e' formato dal pathname
 * assoluto della directory \e DIR= \c /this/is/a/path/name/ in cui si
 * vuole creare una nuova subdirectory, ed il nome \e SUBDIR= \c example
 * della subdirectory che si vuole creare. Ogni singola componente del
 * pathname \e DIR (e.g. \c this \c is \c a ecc.) viene riferito con il
 * termine \e path \e element.  
 * In particolare questa funzione ha il compito di
 *
 * - Controllare che il pathname \e DIR/SUBDIR sia valido, ossia
 *   -# \e DIR sia un path assoluto.
 *   -# Ogni path element in \e DIR non ecceda la lunghezza consentita dal
 *      file system FATS (MAX_LEN_NAME).
 * - Navigare all'interno del file system seguendo i singoli path element. 
 *   Se uno di tali elementi non esiste oppure non e' una directory 
 *   l'operazione deve terminare con fallimento.
 * - Controllare che il nome della nuova subdirectory \e SUBD non sia
 *   gia' presente all'interno della directory \e DIR. In tal caso 
 *   l'operazione deve terminare con fallimento.
 * - Allocare un nuovo blocco per contenere la directory table per \e SUBD
 *   ed inserire le entries \e "." e \e ".."
 * - Preparare la nuova entry da inserire all'interno della directory
 *   table di \e DIR.
 * - Effettuare l'inserimento della entry creata al passo precedente 
 *   all'interno della directory table di \e DIR.
 *
 *
 * \param fs il FILE pointer che consente di accedere al
 *           file dove il filesystem e' memorizzato.
 * \param f_ctrl puntatore alla struttura che mantiene tutte 
 *               le informazioni di controllo del file system.
 * \param path il path assoluto in cui creare la nuova directory.
 *             L'ultimo elemento del path e' il nome della nuova
 *             directory. Ad esempio se <em>path = /home/pippo </em>
 *             allora la directory che deve essere creata ha nome \e 
 *             pippo e deve essere creata come subdirectory di \e /home.
 * \returns 
 *         - EBDP (Error Bad Directory Path) se il path specificato
 *           non e' assoluto, ossia non inizia con il carattere '/'
 *         - ENTL (Error Name Too Long) uno degli elementi del pathname
 *           supera la lunghezza consentita (MAX_LEN_NAME).
 *         - ENSFD (Error Not Such File or Directory) se un elemento del
 *           pah non siste o non e' una directory.
 *         - ENMSD (Error No More Space on Device) se non ci 
 *           sono piu' blocchi disponibili e quindi ci troviamo
 *           in una condizione di file system full.
 *         - ERFCD (Error Reading Fat Control Data) se si verifica 
 *           un errore leggendo le strutture di controllo del
 *           file system FATS.
 *         - EWFCD (Error Writing Fat Control Data) se si verifica 
 *           un errore durante la scrittura delle strutture di 
 *           controllo del file system FATS.
 *         - 0 (zero) in caso di successo.
 */
int fat_mkdir (FILE *fs, struct fat_ctrl *f_ctrl, char *path);

/**
 * Questa funzione ha il compito di creare un nuovo file all'interno del
 * filesystem FATS. Il nome del file \e F viene espresso tramite un pathname
 * assoluto come ad esempio \c /dir/name/fileName in cui il nome del file \e F
 * e' specificato dall'ultimo elemento del pathname (\c fileNmae) e la
 * directrory \e D in cui deve essere creato il file e' specificata dalla
 * prima parte del pathname (\c /dir/name ). In particolare questa funzione
 * deve
 *
 * - Separare il nome del file dal path che specifica la directory \e D dove il
 *   file deve essere generato.
 * - Controllare che la directory \e D esista, ossia che il pathname sia corretto.
 * - Controllare che il file \e F non sia gia' esistente.
 * - Determinare la directory table di \e D. 
 * - Determinare in \e D una entry libera in cui inserire le informazioni
 *   relative al file \e F che deve essere generato. Puo' risultare necessario
 *   aggiungere un nuovo blocco alla directory table \e D.
 * - Selezionare un blocco libero \e B che conterra' il primo byte del file \e F.
 * - Preparare i dati da inserire nella entry che includono:
 *   - \c used valorizzata a DIR_ENTRY_BUSY.
 *   - \c name valorizzato al nome del file \e F.
 *   - \c attr valorizzato a FILE_ENTRY.
 *   - \c index valorizzato all'indice del blocco \e B che contiene
 *     il primo byte del file.
 *   - \c len valorizzato a 0 (alla creazione il file e' vuoto).
 *
 * @param fs il FILE pointer che consente di acceddere al
 *           file dove il file system e' memorizzato.
 * @param f_ctrl puntatore alla struttura che mantiene tutte 
 *               le informazioni di controllo del filesystem.
 * @param path la stringa che contiene il path assoluto del
 *              file che deve essere creato.
 * @returns
 *         - EBDP (Error Bad Directory Path) se il path specificato
 *           non e' assoluto, ossia non inizia con il carattere '/'
 *         - ENTL (Error Name Too Long) uno degli elementi del pathname
 *           supera la lunghezza consentita (MAX_LEN_NAME).
 *         - ENSFD (Error Not Such File or Directory) se un elemento del
 *           pah non esiste o non e' una directory.
 *         - ENMSD (Error No More Space on Device) se non ci 
 *           sono piu' blocchi disponibili e quindi ci troviamo
 *           in una condizione di file system full.
 *         - ERFCD (Error Reading Fat Control Data) se si verifica 
 *           un errore leggendo le strutture di controllo del
 *           file system FATS.
 *         - EWFCD (Error Writing Fat Control Data) se si verifica 
 *           un errore durante la scrittura delle strutture di 
 *           controllo del filesystem FATS.
 *         - 0 (zero) in caso di successo
 *
 */
int fat_open (FILE *fs, struct fat_ctrl *f_ctrl, char *path);

/**
 * Questa funzione ha il compito di leggere una sequenza di byte di un file
 * memorizzato all'interno di un file system FATS. La sequenza viene
 * specificata tramite due parametri:
 *
 * - La posizione \e P del primo byte che si vuole leggere. Il primo byte del 
 *   file ha posizione 0 (zero), il secondo posizione 1 e cosi' via.
 * - Il numero \e NB di byte che si vogliono leggere a 
 *   partire dalla posizione \e P.
 *
 * La funzione deve
 * - Estrarre le informazioni relative al file \e F che deve essere 
 *   acceduto in lettura.
 * - Determinare il blocco \e B che contiene il primo byte del
 *   file che deve essere letto.
 * - Determinare l'offset all'interno del blocco \e B da cui iniziare 
 *   la lettura dei dati.
 * - Seguire la catena dei blocchi utilizzati per memorizzare il
 *   file \e F sino a che sono stati acceduti \e NB bytes o
 *   si e' raggiunta la fine del file.
 *
 * @param fs il FILE pointer che consente di acceddre al
 *           file dove il file system e' memorizzato.
 * @param f_ctrl puntatore alla struttura che mantiene tutte 
 *               le informazioni di controllo del file system.
 * @param path la stringa che contiene il path assoluto del
 *             file che deve essere letto.
 * @param start la posizione del byte da cui cominciare la lettura.
 * @param data_len il numero di byte da leggere a partire dal byte
 *                 in posizone \c start.
 * @param data il buffer in cui copiare i dati letti dal file.
 *             Il buffer deve essere allocato opportunamente.
 * @returns
 *         - EBDP (Error Bad Directory Path) se il path specificato
 *           non e' assoluto, ossia non inizia con il carattere '/'
 *          - In caso di successo la funzione torna un numero non
 *           negativo che indica il numero di bytes letti. Tale 
 *           numero puo'risultare inferiore a \c data_len nel
 *           caso in cui si incontri la fine del file prima di
 *           leggere \c data_len bytes.
 * 
 */
int fat_read(FILE *fs, struct fat_ctrl *f_ctrl, char *path, int start, 
			 int data_len, char *data);

/**
 * Questa funzione ha il compito di scrivere una sequenza di byte all'interno
 * di un file memorizzato in un filesystem FATS. Osserviamo che
 *
 * - Il file su cui si vuole effettuare la scrittura deve essere 
 *   gia' stato creato all'interno del file system FATS.
 * - I dati che debbono essere scritti sono visti da questa funzione 
 *   come una sequenza di byte.
 * - I dati vengono sempre scritti alla fine del file.
 *
 * Questa funzione deve realizzare le seguenti operazioni:
 * - Controllare che il file esista.
 * - Determinare il blocco e l'offset all'interno del blocco
 *   in cui effettuare la scrittura dei nuovi dati.
 * - Determinare se sia necessario allocare uno o piu' 
 *   blocchi in cui memorizzare i nuovi dati.
 * - Scrivere i nuovi dati eventualmente distribuendoli tra
 *   i vari blocchi allocati al passo precedente.
 * - Aggiornare in modo consistente le varie strutture dati
 *   come ad esempio la lunghezza del file e la File Allocation
 *   Table.
 *
 * @param fs il FILE pointer che consente di acceddre al
 *           file dove il filesystem e' memorizzato.
 * @param f_ctrl puntatore alla struttura che mantiene tutte 
 *               le informazioni di controllo del file system.
 * @param path la stringa che contiene il path assoluto del
 *              file su cui effettuare la scrittura.
 * @param data il buffer contenente i nuovi dati da scrivere
 *             all'interno del file.
 * @param data_len il numero di bytes contenuti nel buffer \c data 
 *                 che debbono essere scritti nel file.
 * @returns
 *         - EBDP (Error Bad Directory Path) se il path specificato
 *           non e' assoluto, ossia non inizia con il carattere '/'
 *         - ENTL (Error Name Too Long) uno degli elementi del pathname
 *           supera la lunghezza consentita (MAX_LEN_NAME).
 *         - ENSFD (Error Not Such File or Directory) se un elemento del
 *           pah non esiste o non e' una directory.
 *         - 0 se l'operazione termina con successo.
 */
int fat_write(FILE *fs, struct fat_ctrl *f_ctrl, char *path, char* data, int data_len);

/** TODO controllare i codici di uscita etc..
 * Copia
 *
 * @param from percorso del file sorgente
 * @param to percorso del file destinazione (da creare)
 *
 * @returns
 *		- EDAEX se il file destinazione esiste gia'
 *		- ENSFD se il file sorgente non esiste
 *		- EBDP (Error Bad Directory Path) se il path specificato
 *		  non e' assoluto, ossia non inizia con il carattere '/'
 *		- ENTL (Error Name Too Long) uno degli elementi del pathname
 *        supera la lunghezza consentita (MAX_LEN_NAME).
 *		- 0 OK
 */
int fat_cp( FILE *fs, struct fat_ctrl *f_ctrl, char *from, char *to );

#endif /* OPS_FAT_H */
