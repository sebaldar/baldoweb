CC      = g++
CFLAGS  = -fPIC -Wall -Wextra -std=c++17

PROJECT_ROOT = /app
DIR     = $(PROJECT_ROOT)/cpp
OUT_LIB = /usr/lib
LIBNAME = libPlanetarium.so

# --- DIPENDENZE E HEADER ---
LIBDIRS = -L$(OUT_LIB) -L/usr/local/lib
LIB     = -lpthread -lcurl -lmariadb -lMYSQL -lXML -lJSON -lsystem

INCLUDE = -I./ \
          -I/usr/include/mysql \
          -I/usr/include/mariadb \
          -I$(DIR)/libPlanetarium/VSOP/include \
          -I$(DIR)/libSystem \
          -I$(DIR)/CMat \
          -I$(DIR)/CMat/interprete/include \
          -I$(DIR)/geometria/include \
          -I$(DIR)/libPlanetarium/timer/include \
          -I$(DIR)/libPlanetarium/date/include \
          -I$(DIR)/libPlanetarium/color \
          -I$(DIR)/libPlanetarium/bitmap \
          -I$(DIR)/libPlanetarium/webClient/include \
          -I$(DIR)/Utils \
          -I$(DIR)/libMYSQL \
          -I$(DIR)/libXML/include \
          -I$(DIR)/libJSON/include

# --- OGGETTI (Nomi dei file che verranno creati localmente) ---
OBJS = WSsrv.o clientHttp.o Utils.o date.o tcp_client.o vsop.o elp.o \
       orbit.o celestial_body.o moon_jupiter.o solarSystem.o skymap.o \
       isInBounds.o map.o solarDb.o elp82b.o calc_interpolated_elements.o \
       angolo.o line.o point.o plane.o horizont.o color.o timer.o

# --- VPATH (Istruzioni per trovare i file .cpp sparsi) ---
VPATH = .:$(DIR)/Utils:$(DIR)/libPlanetarium/date/source:$(DIR)/libPlanetarium/webClient/source:\
        $(DIR)/libPlanetarium/VSOP/source:$(DIR)/libPlanetarium/elp82b:$(DIR)/geometria/source:\
        $(DIR)/libPlanetarium/color:$(DIR)/libPlanetarium/timer/source

# --- TARGETS ---

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIBDIRS) $(LIB) -shared -Wl,-soname,$(LIBNAME) -o $(OUT_LIB)/$(LIBNAME)

# La regola magica: compila da ovunque (VPATH) e salva QUI (-o $@)
%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

install:
	ldconfig

clean:
	rm -f *.o $(OUT_LIB)/$(LIBNAME)
