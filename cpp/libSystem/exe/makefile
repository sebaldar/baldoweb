CC=g++

# The CFLAGS variable sets compile flags for gcc:
#  -g        compile with debug information
#  -Wall     give verbose compiler warnings
#  -O0       do not optimize generated code
#  -std=c99  use the C99 standard language definition
#  -m32      CS107 targets architecture IA32 (32-bit)
CFLAGS=-fPIC -Wall -g -std=c++11


APPNAME=system
 $(LIBDIRS)EXENAME=$(APPNAME)

LIB=-lsystem -lXML
my_home=/home/sergio
LIBDIRS=-L$(HOME)/lib -L/usr/local/lib
DIR=$(HOME)/planetarium/cpp


INCLUDE=	-I./ 							\
		-I$(DIR)/libSystem

SRCFILES=./main.cpp


$(APPNAME) : $(SRCFILES) 
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) $(main) -o $(HOME)/bin/$(APPNAME) $(LIBDIRS) $(LIB)





