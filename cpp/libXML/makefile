CC = g++
CFLAGS = -fPIC -Wall -g -std=c++17

# Nome della libreria
LIBNAME = libXML.so

# Percorsi interni al container
PROJECT_ROOT = /app
DIR = $(PROJECT_ROOT)/cpp
OUT_LIB = /usr/lib

# Include relativi alla struttura /app/cpp/...
INCLUDE = -I./include \
          -I$(DIR)/Utils

# Sorgenti mappati sulla struttura del container
xml    = ./source/xml.cpp
parser = ./source/parser.cpp
util   = $(DIR)/Utils/Utils.cpp
field  = $(DIR)/Utils/field.cpp

SRCFILES = $(xml) $(parser) $(util) $(field)

# Target principale: genera la lib direttamente in /usr/lib
lib: $(SRCFILES)
	$(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname,$(LIBNAME) -o $(OUT_LIB)/$(LIBNAME)

clean:
	@echo "PULIZIA DEL SISTEMA"
	rm -f *.o *~ Log.txt

# Aggiorna il database del linker dinamico
install:
	ldconfig

.PHONY: lib clean install
