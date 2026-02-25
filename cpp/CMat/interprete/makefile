CC=g++

# The CFLAGS variable sets compile flags for gcc:
#  -g        compile with debug information
#  -Wall     give verbose compiler warnings
#  -O0       do not optimize generated code
#  -std=c99  use the C99 standard language definition
#  -m32      CS107 targets architecture IA32 (32-bit)
CFLAGS=-fPIC -Wall -g -std=c++11


APPNAME=Interprete
LIBNAME=lib$(APPNAME).so
EXENAME=$(APPNAME)

LIB=-lmat
my_home=/home/sergio
DIR=$(HOME)/programmi/c++
DIR11=$(HOME)/programmi/c++11

INCLUDE=	-I./ 							\
		-I$(DIR)/Hash						\
		-I$(DIR11)/CMat			\
		-I$(DIR11)/CMat/interprete/include			\
		-I$(DIR)/Curva/include				\
		-I$(DIR)/Utils						\
		-I$(DIR)/xml/include

SRCFILES=$(DIR)/Hash/Hash.cpp							\
		$(DIR11)/CMat/interprete/source/Interprete.cpp			\
		$(DIR11)/CMat/interprete/source/Espressione.cpp		\
		$(DIR11)/CMat/interprete/source/ITRNode.cpp			\
		$(DIR11)/CMat/interprete/source/ext_funz.cpp			\
		$(DIR11)/CMat/funzione.cpp					\
		$(DIR)/Utils/Utils.cpp							\
		$(DIR)/Curva/source/Curva.cpp					\
		$(DIR)/Curva/source/matrice.cpp					\
		$(DIR)/Utils/field.cpp							\
		$(DIR)/xml/source/xml.cpp						\
		$(DIR)/xml/source/parser.cpp
		
main=$(DIR11)/CMat/interprete/main.cpp

$(APPNAME) : $(SRCFILES) $(main) 
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) $(main) -o $(HOME)/bin/$(APPNAME) $(LIB)


lib : $(SRCFILES)
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname, -o $(HOME)/lib/$(LIBNAME) $(LIB)

install:
	ln -s $(my_home)/lib/$(LIBNAME)	/usr/local/lib




