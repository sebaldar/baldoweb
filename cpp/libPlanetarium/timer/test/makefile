CC=g++
#CFLAGS=-fPIC -Wall -Wextra -std=c++11 -DDEBUG
CFLAGS=-fPIC -Wall -Wextra -std=c++11
#CFLAGS_DEBUG=-g -DDEBUG

APPNAME=testTimer
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

LIB=

INCLUDE= -I../include
		
timer=$(DIR11)/timer/source/timer.cpp
main=./main.cpp

SRCFILES=$(timer) $(main)

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG)  $(INCLUDE) \
	   $(SRCFILES) \
	   -o $(HOME)/bin/$(APPNAME) $(LIB)




