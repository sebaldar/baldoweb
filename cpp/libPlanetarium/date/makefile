CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=


APPNAME=date

my_home=/home/sergio
DIR=$(my_home)/programmi/c++11

INCLUDE=-I./include			\
		-I$(DIR)/timer/include


SRCFILES=./main.cpp			\
		./source/date.cpp			\
			$(DIR)/timer/source/timer.cpp

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)

Debug : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

