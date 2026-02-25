CC=g++

# The CFLAGS variable sets compile flags for gcc:
#  -g        compile with debug information
#  -Wall     give verbose compiler warnings
#  -O0       do not optimize generated code
#  -std=c99  use the C99 standard language definition
#  -m32      CS107 targets architecture IA32 (32-bit)
CFLAGS=-fPIC -Wall -g -std=c++11


APPNAME=Simulatore
LIBNAME=lib$(APPNAME).so
EXENAME=$(APPNAME)

LIB=
my_home=/home/sergio
DIR=$(HOME)/cpp
DIR11=$(HOME)/cpp

INCLUDE=	-I./ 							\
		-I$(DIR)/CMat/Hash						\
		-I$(DIR11)/CMat/cgi			\
		-I$(DIR11)/CMat			\
		-I$(DIR11)/CMat/interprete/include			\
		-I$(DIR11)/CMat/system/include			\
		-I$(DIR)/CMat/Curva/include				\
		-I$(DIR)/Utils						\
		-I$(DIR)/libXML/include

SRCFILES=$(DIR)/CMat/Hash/Hash.cpp							\
		$(DIR11)/CMat/system/source/system.cpp			\
		$(DIR11)/CMat/interprete/source/Interprete.cpp			\
		$(DIR11)/CMat/interprete/source/Espressione.cpp		\
		$(DIR11)/CMat/interprete/source/ITRNode.cpp			\
		$(DIR11)/CMat/interprete/source/ext_funz.cpp			\
		$(DIR11)/CMat/funzione.cpp					\
		$(DIR11)/CMat/variabile.cpp					\
		$(DIR11)/CMat/matrix.cpp		\
		$(DIR)/Utils/Utils.cpp							\
		$(DIR)/CMat/Curva/source/Curva.cpp					\
		$(DIR)/CMat/Curva/source/matrice.cpp					\
		$(DIR11)/CMat/cgi/cgi_base.cpp							\
		$(DIR)/Utils/field.cpp							\
		$(DIR)/libXML/source/xml.cpp						\
		$(DIR)/libXML/source/parser.cpp
		
main=$(DIR11)/CMat/system/main.cpp

$(APPNAME) : $(SRCFILES) $(main) 
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) $(main) -o $(HOME)/bin/$(APPNAME) $(LIB)


lib : $(SRCFILES)
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname, -o $(HOME)/lib/$(LIBNAME) $(LIB)

install:
	ln -s $(my_home)/lib/$(LIBNAME)	/usr/local/lib




