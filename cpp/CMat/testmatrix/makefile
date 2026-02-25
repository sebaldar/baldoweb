CC=g++

# The CFLAGS variable sets compile flags for gcc:
#  -g        compile with debug information
#  -Wall     give verbose compiler warnings
#  -O0       do not optimize generated code
#  -std=c99  use the C99 standard language definition
#  -m32      CS107 targets architecture IA32 (32-bit)
CFLAGS=-fPIC -Wall -g -std=c++11


APPNAME=testmatrix
EXENAME=$(APPNAME)

LIB=
my_home=/home/sergio
DIR=$(HOME)/programmi/c++
DIR11=$(HOME)/programmi/c++11

INCLUDE=	-I./ 							\
		-I$(DIR11)/CMat			\
		-I$(DIR)/Utils

SRCFILES=$(DIR11)/CMat/matrix.cpp		\
		$(DIR)/Utils/Utils.cpp
		
main=./main.cpp

$(APPNAME) : $(SRCFILES) $(main) 
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) $(main) -o $(HOME)/bin/$(APPNAME) $(LIB)







