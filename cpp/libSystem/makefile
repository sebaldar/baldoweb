CC = g++

# Manteniamo c++11 come da tuo originale, ma fPIC è vitale per la .so
CFLAGS=-fPIC -Wall -g -std=c++17

APPNAME = system
LIBNAME = lib$(APPNAME).so

# Percorsi interni al container
PROJECT_ROOT = /app
DIR = $(PROJECT_ROOT)/cpp
OUT_LIB = /usr/lib

# Librerie: cerca in /usr/lib (dove abbiamo messo libXML.so)
LIBDIRS = -L$(OUT_LIB) -L/usr/local/lib
LIB = -lXML

INCLUDE = -I./ \
          -I$(DIR)/CMat/Hash \
          -I$(DIR)/CMat/cgi \
          -I$(DIR)/Utils \
          -I$(DIR)/CMat \
          -I$(DIR)/CMat/interprete/include \
          -I$(DIR)/CMat/system/include \
          -I$(DIR)/CMat/Curva/include \
          -I$(DIR)/libXML/include

SRCFILES = $(DIR)/CMat/Hash/Hash.cpp \
           $(DIR)/CMat/cgi/cgi_base.cpp \
           $(DIR)/CMat/system/source/system.cpp \
           $(DIR)/CMat/interprete/source/Interprete.cpp \
           $(DIR)/CMat/interprete/source/Espressione.cpp \
           $(DIR)/CMat/interprete/source/ITRNode.cpp \
           $(DIR)/CMat/interprete/source/ext_funz.cpp \
           $(DIR)/CMat/funzione.cpp \
           $(DIR)/CMat/variabile.cpp \
           $(DIR)/CMat/matrix.cpp \
           $(DIR)/Utils/Utils.cpp \
           $(DIR)/Utils/field.cpp \
           $(DIR)/CMat/Curva/source/Curva.cpp \
           $(DIR)/CMat/Curva/source/matrice.cpp \
           sistema.cpp

# Target principale
lib : $(SRCFILES)
	$(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname,$(LIBNAME) -o $(OUT_LIB)/$(LIBNAME) $(LIBDIRS) $(LIB)

# Aggiorna il linker
install:
	ldconfig

clean:
	rm -f $(OUT_LIB)/$(LIBNAME)

.PHONY: lib install clean
