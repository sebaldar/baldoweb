CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=-lXML -lsystem


APPNAME=vsop
LIBNAME=libcompute

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

INCLUDE=-I./include			\
		-I./server/		\
		-I$(DIR11)/CMat					\
		-I$(DIR11)/CMat/interprete/include	\
		-I$(DIR11)/geometria/include	\
		-I$(DIR11)/timer/include	\
		-I$(DIR)/xml/include		\
		-I$(DIR)/Utils				\
		-I$(DIR11)/CMat/system/lib	\
		-I$(DIR11)/date/include

line=$(DIR11)/geometria/source/line.cpp
point=$(DIR11)/geometria/source/point.cpp
plane=$(DIR11)/geometria/source/plane.cpp
horizont=$(DIR11)/geometria/source/horizont.cpp

angolo=$(DIR11)/geometria/source/angolo.cpp
orbit=./source/orbit.cpp
elp=./source/elp.cpp
elp82b=$(DIR11)/elp82b/elp82b.cpp
calc_interpolated_elements=$(DIR11)/elp82b/calc_interpolated_elements.cpp
moon_jupiter=./source/moon_jupiter.cpp	

SRCFILES=./main.cpp			\
		./source/vsop.cpp	\
		./source/celestial_body.cpp	\
		./source/solarSystem.cpp	\
			$(DIR11)/timer/source/timer.cpp	\
			$(DIR11)/date/source/date.cpp \
			$(line) $(point) $(plane) $(horizont) $(angolo)	$(orbit) $(elp)	\
			$(elp82b) $(calc_interpolated_elements) $(moon_jupiter)


$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)

Debug : $(SRCFILES)
	   $(CC) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

install:
	ln -s $(my_home)/bin/$(APPNAME)	/usr/lib
