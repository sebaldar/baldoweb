CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=


APPNAME=testmat

my_home=/home/sergio
DIR11=$(HOME)/cpp

INCLUDE=-I../

main=./main.cpp


SRCFILES=$(DIR11)/CMat/funzione.cpp					\
		$(DIR11)/CMat/variabile.cpp	\
		$(DIR11)/CMat/matrix.cpp		\
		$(main)


$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)

Debug : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

