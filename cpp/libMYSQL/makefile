# Compilatore e flag
CC := g++
CFLAGS := -fPIC -Wall -Wextra -pedantic -std=c++17 -Wzero-as-null-pointer-constant

# Nome libreria
LIBNAME := libMYSQL.so

# In Docker la root del progetto è /app
PROJECT_ROOT := /app
# Directory di destinazione per le librerie di sistema nel container
OUT_LIB := /usr/lib

# Include (Aggiungiamo mariadb path che è standard su Debian/Ubuntu)
INCLUDE := \
    -I./ \
    -I/usr/include/mysql \
    -I/usr/include/mariadb

# Linker: usiamo mariadb
LIB := -lmariadb

# Sorgenti
SRCFILES := tmysql.cpp 

# Target principale: compila la libreria direttamente in /usr/lib
$(LIBNAME): $(SRCFILES)
	$(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) \
		-Wno-deprecated -shared -Wl,-soname,$(LIBNAME) \
		-o $(OUT_LIB)/$(LIBNAME)

# In Docker l'installazione è solo un aggiornamento della cache del linker
install:
	ldconfig

# Pulizia
clean:
	rm -f $(OUT_LIB)/$(LIBNAME)

# Compila un programma di test (adattato per percorsi di sistema)
test: $(LIBNAME) test_mysql.cpp
	$(CC) -Wall -std=c++17 -I./ test_mysql.cpp -lMYSQL -o test_mysql
	./test_mysql

# Compilazione con debug
debug: CFLAGS += -g -O0
debug: $(LIBNAME)

.PHONY: clean install test debug
