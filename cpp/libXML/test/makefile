CC=g++

LIB=-lXML
CFLAGS=-fPIC -Wall -g

EXENAME=testXML

my_home=$(HOME)
DIR=$(my_home)/cpp
 
INCLUDE=	-I$(DIR)/libXML/include	\
		-I$(DIR)/Utils		


SRCFILES=./main.cpp

exe : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) $(LIB) -o ./$(EXENAME)
	   

clean:
	@echo PULIZIA DEL SISTEMA
	rm -f *.o *~ Log.txt

  

