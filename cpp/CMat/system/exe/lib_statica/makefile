CC=g++

# The CFLAGS variable sets compile flags for gcc:
#  -g        compile with debug information
#  -Wall     give verbose compiler warnings
#  -O0       do not optimize generated code
#  -std=c99  use the C99 standard language definition
#  -m32      CS107 targets architecture IA32 (32-bit)
CFLAGS=-fPIC -Wall -g -std=c++11


APPNAME=system

LIB=-lsystem
my_home=/home/sergio
DIR=$(HOME)/cpp

INCLUDE=	-I./ 							\
		-I$(DIR)/CMat/Hash						\
		-I$(DIR)/CMat/cgi			\
		-I$(DIR)/CMat			\
		-I$(DIR)/CMat/interprete/include			\
		-I$(DIR)/CMat/system/include			\
		-I$(DIR)/CMat/Curva/include				\
		-I$(DIR)/Utils						\
		-I$(DIR)/libXML/include

main=$(DIR)/CMat/system/main.cpp


$(APPNAME) : $(SRCFILES) 
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(main) -o ./$(APPNAME) -L. $(LIB)





