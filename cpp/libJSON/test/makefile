CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=-lJSON


APPNAME=json

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(my_home)/programmi/c++

INCLUDE=-I../include 			\
		-I$(DIR)/Utils		

main=./main.cpp



SRCFILES=$(main)


$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(APPNAME)

Debug : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(APPNAME)

