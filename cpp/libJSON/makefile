CC = g++
# Usiamo c++17 per coerenza con le altre librerie, 
# ma c++11 va bene se rapidjson lo richiede specificamente.
CFLAGS = -fPIC -Wall -g -std=c++17 

# Nome della libreria
LIBNAME = libJSON.so

# Directory di destinazione nel container
OUT_LIB = /usr/lib

# Include relativi (assumendo di essere in /app/cpp/libJSON)
INCLUDE = -I./include \
          -I./rapidjson-master/include 

SRCFILES = ./source/json.cpp

# Target principale: compila direttamente in /usr/lib
lib : $(SRCFILES)
	$(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname,$(LIBNAME) -o $(OUT_LIB)/$(LIBNAME)

clean:
	@echo "PULIZIA DEL SISTEMA"
	rm -f *.o *~ Log.txt

# Aggiorna il linker nel container
install:
	ldconfig

.PHONY: lib clean install
