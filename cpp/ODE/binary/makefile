CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng

CFLAGS_DEBUG=-g
LIB=-lXML -lInterprete


APPNAME=binary
LIBNAME=libcompute

my_home=/home/sergio
DIR=$(my_home)/cpp

INCLUDE=	-I./include					\
		-I$(DIR)/CMat					\
		-I$(DIR)/ODE/include	\
		-I$(DIR)/Interprete/include		


SRCFILES=$(DIR)/CMat/cmat.cpp			\
			$(DIR)/CMat/variabile.cpp		\
			$(DIR)/CMat/funzione.cpp		\
			$(DIR)/ODE/source/odefunctor.cpp	\
			$(DIR)/CMat/matrix.cpp		\
			./main.cpp

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)

Debug : $(SRCFILES)
	   $(CC) $(CFLAGS_DEBUG) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

