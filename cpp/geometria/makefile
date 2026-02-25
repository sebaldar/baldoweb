CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=-lsystem


APPNAME=geometry

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(my_home)/programmi/c++11

INCLUDE=-I./include					\
		-I$(DIR11)/angolo	\
		-I$(DIR11)/CMat/system/lib

main=./main.cpp

plane=$(DIR11)/geometria/source/plane.cpp
line=$(DIR11)/geometria/source/line.cpp
point=$(DIR11)/geometria/source/point.cpp
angolo=$(DIR11)/angolo/angolo.cpp


SRCFILES=$(line) $(point) $(plane) $(angolo)	\
	$(main)


$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)

Debug : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

