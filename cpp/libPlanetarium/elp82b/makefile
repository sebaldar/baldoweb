CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11

CFLAGS_DEBUG=-g

LIB=-lelp82b

LIBNAME=libelp82b.so
APPNAME=elp82b

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

INCLUDE=-I./


SRCFILES=			\
		./elp82b.cpp	\
		./calc_interpolated_elements.cpp

$(APPNAME) : ./main.cpp
	   $(CC) $(CFLAGS) $(INCLUDE) ./main.cpp $(LIB) -o $(HOME)/bin/$(APPNAME)

lib : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname, -o $(HOME)/lib/$(LIBNAME) 
	   

install:
	ln -s $(my_home)/lib/$(LIBNAME)	/usr/lib
  
