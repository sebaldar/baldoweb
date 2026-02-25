CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Wzero-as-null-pointer-constant
APPNAME=testMysql

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

INCLUDE=	-I./ 					\
		-I$(DIR11)/libMYSQL			\
		-I$(DIR)/Util				\
		-I/usr/include/mysql/				
LIB=								\
		-lMYSQL


SRCFILES=        ./main.cpp 
$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS)  $(INCLUDE) $(SRCFILES) -L$(HOME)/lib $(LIB) -o $(HOME)/bin/$(APPNAME)


