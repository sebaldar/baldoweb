CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=

APPNAME=moto

my_home=/home/sergio
DIR=$(my_home)/cpp

INCLUDE=	-I./include			\
		-I$(DIR)/ODE/include	\
		-I$(DIR)/CMat			

SRCFILES=./main.cpp 						\
			$(DIR)/CMat/funzione.cpp		\
			$(DIR)/ODE/source/odefunctor.cpp		

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)


Debug : $(SRCFILES)
	   $(CC) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

