CC 				= gcc
CFLAGS 		= -std=c99 -Wall -pedantic
DBGFLAGS	= -g


INCDIR 	 = ./include
LIBDIR 	 = ./lib
SRCDIR 	 = ./src
BINDIR 	 = ./bin

INCLUDES = -I $(INCDIR) -pthread
LIBS 		 = -lmyfun
LDFLAGS  = -Wl,-rpath,$(LIBDIR)/ -L $(LIBDIR)/ $(LIBS) -pthread



TARGET 		= $(BINDIR)/supermercato

.PHONY 		= all clean cleanall test testquit

# pattern da .c ad eseguibile # pattern da .c ad eseguibile
$(BINDIR)/%: $(SRCDIR)/%.c
				$(CC) $(CFLAGS) $(DBGFLAGS)  $(INCLUDES) $< -o $@ $(LDFLAGS)

# pattern per la generazione di un .o da un .c nella directory SRCDIR
$(SRCDIR)/%.o: $(SRCDIR)/%.c
				$(CC) -fPIC $(CFLAGS) $(INCLUDES) $(DBGFLAGS)  -c -o $@ $<

# prima regola
all: $(TARGET)

# dipendenze dell'eseguibile che voglio generare

$(TARGET): $(SRCDIR)/supermercato.c $(LIBDIR)/libmyfun.so

# dipendenze della libreria
$(LIBDIR)/libmyfun.so: 	$(SRCDIR)/Bqueue.o $(SRCDIR)/util.o $(SRCDIR)/Queue.o\
												$(SRCDIR)/signal_handler.o\
												$(SRCDIR)/cassiere.o $(SRCDIR)/cliente.o  $(SRCDIR)/direttore.o

				$(CC) -shared -o $@ $^


clean:
					rm -f $(TARGET) pid.txt log.txt filenameLog.txt

cleanall: clean
					find . \( -name "*~" -o -name "*.o" \) -exec rm -f {} \;
					rm -f  $(LIBDIR)/libmyfun.so

test:
					$(TARGET) &
					sleep 25
	 				kill -1 `cat ./pid.txt`
					chmod +x analisi.sh
					./analisi.sh pid.txt filenameLog.txt

testquit:
					$(TARGET) &
					sleep 25
	 				kill -3 `cat ./pid.txt`
					chmod +x analisi.sh
					./analisi.sh pid.txt filenameLog.txt
