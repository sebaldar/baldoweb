CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=-lnova


APPNAME=nova

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

INCLUDE=-I../	\
		-I$(DIR11)/timer/include	\
	-I$(DIR11)/date/include


SRCFILES=./main.cpp	\
			$(DIR11)/timer/source/timer.cpp	\
			$(DIR11)/date/source/date.cpp

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)

