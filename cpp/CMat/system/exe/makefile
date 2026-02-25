CC=g++

# The CFLAGS variable sets compile flags for gcc:
#  -g        compile with debug information
#  -Wall     give verbose compiler warnings
#  -O0       do not optimize generated code
#  -std=c99  use the C99 standard language definition
#  -m32      CS107 targets architecture IA32 (32-bit)
CFLAGS=-fPIC -Wall -g -std=c++11


APPNAME=system
LIBNAME=lib$(APPNAME).so
EXENAME=$(APPNAME)

LIB=-lSimulatore
my_home=/home/sergio
DIR11=$(HOME)/cpp

INCLUDE=	-I./ 							\
		-I$(DIR11)/CMat			\
		-I$(DIR11)/libXML/include	\
		-I$(DIR11)/CMat/system/include

SRCFILES=./main.cpp


$(APPNAME) : $(SRCFILES) 
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) $(main) -o $(HOME)/bin/$(APPNAME) $(LIB)





