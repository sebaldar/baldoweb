CC=g++
#CFLAGS=-fPIC -Wall -Wextra -std=c++11 -DDEBUG
CFLAGS=-fPIC -Wall -Wextra -std=c++11 
#CFLAGS_DEBUG=-g -DDEBUG

APPNAME=missile
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

LIB=-lXML -lsystem -lmysqlclient -lpthread

INCLUDE=	-I./		\
		-I/usr/include/mysql	\
		-I$(DIR11)/ODE/include		\
		-I$(DIR11)/VSOP/server		\
		-I$(DIR11)/VSOP/include		\
		-I$(DIR11)/json					\
		-I$(DIR11)/CMat/system/lib		\
		-I$(DIR11)/CMat					\
		-I$(DIR11)/CMat/interprete/include	\
		-I$(DIR11)/geometria/include	\
		-I$(DIR11)/timer/include	\
		-I$(DIR11)/date/include	\
		-I$(DIR11)/color	\
		-I$(DIR11)/bitmap	\
		-I$(DIR11)/webClient/include	\
		-I$(DIR)/crypt/include 	\
		-I$(DIR)/Server/include		\
		-I$(DIR)/mysql				\
		-I$(DIR)/Utils				\
		-I$(DIR)/xml/include		

vettore=$(DIR11)/geometria/source/vettore.cpp
funzione=$(DIR11)/CMat/funzione.cpp	
odefunctor=$(DIR11)/ODE/source/odefunctor.cpp		


WSsrv=$(DIR11)/VSOP/server/WSsrv.cpp
server=$(DIR)/Server/source/WebServerBase.cpp
client=$(DIR)/Server/source/clientHttp.cpp
webclient=$(DIR11)/webClient/source/tcp_client.cpp
ws=$(DIR)/Server/source/WebServerWS.cpp
sha1=$(DIR)/crypt/source/sha1.cpp
base64=$(DIR)/crypt/source/base64.cpp
utils=$(DIR)/Utils/Utils.cpp
timer=$(DIR11)/timer/source/timer.cpp
vsop=$(DIR11)/VSOP/source/vsop.cpp
elp=$(DIR11)/VSOP/source/elp.cpp
orbit=$(DIR11)/VSOP/source/orbit.cpp
celestial=$(DIR11)/VSOP/source/celestial_body.cpp	
moon_jupiter=$(DIR11)/VSOP/source/moon_jupiter.cpp	
solar=$(DIR11)/VSOP/source/solarSystem.cpp	
skymap=$(DIR11)/VSOP/source/skymap.cpp	
map=$(DIR11)/VSOP/source/map.cpp	
solardb=$(DIR11)/VSOP/source/solarDb.cpp	
date=$(DIR11)/date/source/date.cpp
color=$(DIR11)/color/color.cpp
elp82b=$(DIR11)/elp82b/elp82b.cpp
calc_interpolated_elements=$(DIR11)/elp82b/calc_interpolated_elements.cpp

angolo=$(DIR11)/geometria/source/angolo.cpp
line=$(DIR11)/geometria/source/line.cpp
point=$(DIR11)/geometria/source/point.cpp
plane=$(DIR11)/geometria/source/plane.cpp
horizont=$(DIR11)/geometria/source/horizont.cpp

mysql=$(DIR)/mysql/tmysql.cpp


main=./main.cpp

SRCFILES=$(server) $(client) $(utils) $(WSsrv) $(ws) 	\
	$(sha1) $(base64) $(timer) 	$(angolo)	$(color)\
	$(elp82b) $(calc_interpolated_elements) 	\
	$(vsop) $(celestial) $(moon_jupiter) $(solar) $(date) \
	$(elp)	$(orbit)	$(skymap)	$(map)			\
	$(mat) $(solardb) $(variable) $(funzione) $(matrix)	\
	$(line) $(point) $(plane) $(horizont) $(webclient) $(mysql) \
	$(odefunctor) $(vettore) \
	$(main)

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG)  $(INCLUDE) \
	   $(SRCFILES) \
	   -o $(HOME)/bin/$(APPNAME) $(LIB)




