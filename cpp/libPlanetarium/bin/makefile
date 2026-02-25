CC=g++
#CFLAGS=-fPIC -Wall -Wextra -std=c++11 -DDEBUG
CFLAGS=-fPIC -Wall -Wextra -std=c++11
#CFLAGS_DEBUG=-g -DDEBUG

APPNAME=test
my_home=$(HOME)
DIR=$(HOME)/planeatarium/cpp

LIBDIRS=-L$(HOME)/lib -L/usr/local/lib
LIB=-lPlanetarium -lmariadbclient -lXML -lMYSQL -lsystem -lJSON

INCLUDE=	-I../

main=./main.cpp

SRCFILES=$(main)

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG)  $(INCLUDE) \
	   $(SRCFILES) \
	   -o $(APPNAME) $(LIBDIRS) $(LIB)




