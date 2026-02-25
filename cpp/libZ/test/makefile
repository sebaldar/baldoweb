CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 

CFLAGS_DEBUG=-g
LIB=-lz


APPNAME=lzip

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(my_home)/programmi/c++

INCLUDE=-I$(DIR11)/libZ/include
		
main=./main.cpp
zlib=$(DIR11)/libZ/source/zlib.cpp


SRCFILES=$(main) $(zlib)


$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME) $(LIB)

Debug : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

