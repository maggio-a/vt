# ====================================================================
# Autore: Andrea Maggiordomo - mggndr89@gmail.com - MAT 445942
# Universita' di Pisa - Informatica Applicata
# Corso di Sistemi Operativi e Laboratorio - Progetto didattico 'fats'
# 2013
#
# Il contenuto di questo file e' interamente ad opera dell'autore.
# ====================================================================

SRC_PATH = src/client src/com src/common src/create src/fat src/server
VPATH = $(SRC_PATH)

CLIENT = fats_client
SERVER = fats_server
FCREATE = fats_create

USRTEST = test
FSTEST = fstest
TESTCLIENT = tc

CC = gcc
CFLAGS = -Wall -pedantic -g -O0

.PHONY: clean mostlyclean test tmpdir

all: $(FCREATE) $(CLIENT) $(SERVER)

build: $(FCREATE) $(CLIENT) $(SERVER) mostlyclean

# fats_create build
create: $(FCREATE)

$(FCREATE): fats_create.o format.o fat_error.o

fats_create.o: format.h

format.o:

# client build
client: $(CLIENT)

CLOBJ = fats_client.o client_impl.o sol_com.o fat_error.o common.o

$(CLIENT): $(CLOBJ)
	$(CC) -o $(CLIENT) $(CLOBJ) $(CFLAGS) -lpthread

# client test build
testclient:
	make $(TESTCLIENT) -B -s CFLAGS='-Wall -pedantic -g -DSOL_TESTING'

$(TESTCLIENT): $(CLOBJ)
	$(CC) -o $(TESTCLIENT) $(CLOBJ) $(CFLAGS) -lpthread

# server build
server: $(SERVER) tmpdir

tmpdir:
	@mkdir -p tmp

SVOBJ = fats_server.o server_impl.o sol_com.o fat_error.o thlist.o common.o \
        load_fat.o ops_fat.o

$(SERVER): $(SVOBJ)
	$(CC) -o $(SERVER) $(SVOBJ) $(CFLAGS) -lpthread

# dipendenze CLIENT
fats_client.o: fats_client.c client_impl.h sol_com.h
	$(CC) -c $< -o $@ $(CFLAGS)

client_impl.o: client_impl.c client_impl.h sol_com.h
	$(CC) -c $< -o $@ $(CFLAGS)

# dipendenze server
fats_server.o: fats_server.c server_impl.h sol_com.h
	$(CC) -c $< -o $@ $(CFLAGS)

server_impl.o: server_impl.c server_impl.h sol_com.h
	$(CC) -c $< -o $@ $(CFLAGS)

# operazioni
ops_fat.o: ops_fat.c ops_fat.h
	$(CC) -c $< -o $@ $(CFLAGS)

# lista threads
thlist.o: thlist.h

# load
load_fat.o: load_fat.h

# comunicazione
sol_com.o: sol_com.c sol_com.h com_defs.h
	$(CC) -c $< -o $@ $(CFLAGS)

# errori
fat_error.o: fat_error.h

# common
common.o: common.h

# clean
clean:
	rm -f *.o *~ src/*/*~ $(CLIENT) $(SERVER) $(FCREATE) $(TESTCLIENT) test/*.output *.log

mostlyclean:
	rm -f *.o *.log $(TESTCLIENT)

# test target
test: tmpdir test1 test2 test3 test4

test1:
	@echo "Test del file system FATS (blocchi 128 B)...";                      \
	if [ ! -f $(SERVER) ] || [ ! -f $(FCREATE) ]; then                         \
		echo "Build incompleta";                                               \
		exit;                                                                  \
	fi;                                                                        \
	if [ ! -d test ]; then                                                     \
		echo "Cartella test non trovata";                                      \
		exit;                                                                  \
	fi;                                                                        \
	rm -f test/$(FSTEST);                                                      \
	make testclient -B -s CFLAGS='-Wall -pedantic -g -DSOL_TESTING';           \
	./$(FCREATE) $(USRTEST) test/$(FSTEST) 250 1 > /dev/null || exit;          \
	./$(SERVER) test > s1.log &                                                \
	SERVER_PID=$$!;                                                            \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test1a.input > test/test1a.output;       \
	diff test/test1a.output test/test1a.ok && echo "Test 1 superato"           \
	     || echo "Test 1 fallito";                                             \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test2a.input > test/test2a.output;       \
	diff test/test2a.output test/test2.ok && echo "Test 2 superato"            \
	     || echo "Test 2 fallito";                                             \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test3a.input > test/test3a.output;       \
	diff test/test3a.output test/test3.ok && echo "Test 3 superato"            \
	     || echo "Test 3 fallito";                                             \
	                                                                           \
	kill -TERM $$SERVER_PID;                                                   \
	rm -f test/$(FSTEST);                                                      \
	make -s mostlyclean

test2:
	@echo "Test del file system FATS (blocchi 1 kB)...";                       \
	if [ ! -f $(SERVER) ] || [ ! -f $(FCREATE) ]; then                         \
		echo "Build incompleta";                                               \
		exit;                                                                  \
	fi;                                                                        \
	if [ ! -d test ]; then                                                     \
		echo "Cartella test non trovata";                                      \
		exit;                                                                  \
	fi;                                                                        \
	rm -f test/$(FSTEST);                                                      \
	make testclient -B -s CFLAGS='-Wall -pedantic -g -DSOL_TESTING';           \
	./$(FCREATE) $(USRTEST) test/$(FSTEST) 250 2 > /dev/null || exit;          \
	./$(SERVER) test > s2.log &                                                \
	SERVER_PID=$$!;                                                            \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test1b.input > test/test1b.output;       \
	diff test/test1b.output test/test1b.ok && echo "Test 1 superato"           \
	     || echo "Test 1 fallito";                                             \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test2b.input > test/test2.output;        \
	diff test/test2.output test/test2.ok && echo "Test 2 superato"             \
	     || echo "Test 2 fallito";                                             \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test3b.input > test/test3.output;        \
	diff test/test3.output test/test3.ok && echo "Test 3 superato"             \
	     || echo "Test 3 fallito";                                             \
	                                                                           \
	kill -TERM $$SERVER_PID;                                                   \
	rm -f test/$(FSTEST);                                                      \
	make -s mostlyclean

test3:
	@echo "Test del file system FATS (blocchi 2 kB)...";                       \
	if [ ! -f $(SERVER) ] || [ ! -f $(FCREATE) ]; then                         \
		echo "Build incompleta";                                               \
		exit;                                                                  \
	fi;                                                                        \
	if [ ! -d test ]; then                                                     \
		echo "Cartella test non trovata";                                      \
		exit;                                                                  \
	fi;                                                                        \
	rm -f test/$(FSTEST);                                                      \
	make testclient -B -s CFLAGS='-Wall -pedantic -g -DSOL_TESTING';           \
	./$(FCREATE) $(USRTEST) test/$(FSTEST) 250 3 > /dev/null || exit;          \
	./$(SERVER) test > s3.log &                                                \
	SERVER_PID=$$!;                                                            \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test1c.input > test/test1c.output;       \
	diff test/test1c.output test/test1c.ok && echo "Test 1 superato"           \
	     || echo "Test 1 fallito";                                             \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test2c.input > test/test2.output;        \
	diff test/test2.output test/test2.ok && echo "Test 2 superato"             \
	     || echo "Test 2 fallito";                                             \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test3c.input > test/test3.output;        \
	diff test/test3.output test/test3.ok && echo "Test 3 superato"             \
	     || echo "Test 3 fallito";                                             \
	                                                                           \
	kill -TERM $$SERVER_PID;                                                   \
	rm -f test/$(FSTEST);                                                      \
	make -s mostlyclean

test4:
	@echo "Test del file system FATS (blocchi 4 kB)...";                       \
	if [ ! -f $(SERVER) ] || [ ! -f $(FCREATE) ]; then                         \
		echo "Build incompleta";                                               \
		exit;                                                                  \
	fi;                                                                        \
	if [ ! -d test ]; then                                                     \
		echo "Cartella test non trovata";                                      \
		exit;                                                                  \
	fi;                                                                        \
	rm -f test/$(FSTEST);                                                      \
	make testclient -B -s CFLAGS='-Wall -pedantic -g -DSOL_TESTING';           \
	./$(FCREATE) $(USRTEST) test/$(FSTEST) 250 4 > /dev/null || exit;          \
	./$(SERVER) test > s4.log &                                                \
	SERVER_PID=$$!;                                                            \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test1d.input > test/test1d.output;       \
	diff test/test1d.output test/test1d.ok && echo "Test 1 superato"           \
	     || echo "Test 1 fallito";                                             \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test2d.input > test/test2.output;        \
	diff test/test2.output test/test2.ok && echo "Test 2 superato"             \
	     || echo "Test 2 fallito";                                             \
	                                                                           \
	./$(TESTCLIENT) $(USRTEST) < test/test3d.input > test/test3.output;        \
	diff test/test3.output test/test3.ok && echo "Test 3 superato"             \
	     || echo "Test 3 fallito";                                             \
	                                                                           \
	kill -TERM $$SERVER_PID;                                                   \
	rm -f test/$(FSTEST);                                                      \
	make -s mostlyclean
