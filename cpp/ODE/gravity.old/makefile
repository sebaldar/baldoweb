CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -g -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=-lXML -lsystem -lmysqlclient -lpthread

APPNAME=gravity

my_home=/home/sergio
DIR=$(my_home)/programmi/c++11

INCLUDE=	-I./include			\
		-I./include/include	\
		-I../include	\
		-I$(DIR)/timer/include	\
		-I$(DIR)/date/include	\
		-I$(DIR)/geometria/include	\
		-I$(DIR)/VSOP/include		\
		-I$(DIR)/CMat			

vsop=$(DIR)/VSOP/source/vsop.cpp
elp82b=$(DIR)/elp82b/elp82b.cpp
calc_interpolated_elements=$(DIR)/elp82b/calc_interpolated_elements.cpp
elp=$(DIR)/VSOP/source/elp.cpp
orbit=$(DIR)/VSOP/source/orbit.cpp
celestial=$(DIR)/VSOP/source/celestial_body.cpp	

timer=$(DIR)/timer/source/timer.cpp
date=$(DIR)/date/source/date.cpp
angolo=$(DIR)/geometria/source/angolo.cpp
line=$(DIR)/geometria/source/line.cpp
point=$(DIR)/geometria/source/point.cpp
vettore=$(DIR)/geometria/source/vettore.cpp

rocket=./source/rocket.cpp
propulsore=./source/propulsore.cpp

SRCFILES=./main.cpp 						\
			$(DIR)/CMat/funzione.cpp		\
			../source/odefunctor.cpp	$(vsop)	$(orbit) $(date) $(timer) $(angolo) $(celestial) \
			$(line) $(point) $(vettore) $(elp82b) $(elp) $(calc_interpolated_elements)	\
			$(rocket) $(propulsore)

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)


Debug : $(SRCFILES)
	   $(CC) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

