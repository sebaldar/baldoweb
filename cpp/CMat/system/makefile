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

INCLUDE=	-I./ 							\
		-I$(DIR)/CMat/system/interfaccia	\
		-I$(DIR)/CMat/Hash					\
		-I$(DIR)/CMat/cgi					\
		-I$(DIR)/CMat						\
		-I$(DIR)/CMat/interprete/include	\
		-I$(DIR)/CMat/system/include		\
		-I$(DIR)/CMat/Curva/include			\
		-I$(DIR)/Utils						\
		-I$(DIR)/libXML/include

SRCFILES=$(DIR)/CMat/Hash/Hash.cpp						\
		$(DIR)/CMat/system/source/system.cpp			\
		$(DIR)/CMat/interprete/source/Interprete.cpp	\
		$(DIR)/CMat/interprete/source/Espressione.cpp	\
		$(DIR)/CMat/interprete/source/ITRNode.cpp		\
		$(DIR)/CMat/interprete/source/ext_funz.cpp	\
		$(DIR)/CMat/funzione.cpp						\
		$(DIR)/CMat/variabile.cpp						\
		$(DIR)/CMat/matrix.cpp						\
		$(DIR)/Utils/Utils.cpp							\
		$(DIR)/CMat/Curva/source/Curva.cpp				\
		$(DIR)/CMat/Curva/source/matrice.cpp			\
		$(DIR)/CMat/cgi/cgi_base.cpp					\
		$(DIR)/Utils/field.cpp							\
		$(DIR)/libXML/source/xml.cpp					\
		$(DIR)/libXML/source/parser.cpp					\
		$(DIR)/CMat/system/interfaccia/interfaccia.cpp
		
main=$(DIR)/CMat/system/main.cpp

$(APPNAME) : $(SRCFILES) $(main) 
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) $(main) -o $(HOME)/bin/$(APPNAME) $(LIB)


lib : $(SRCFILES)
	   $(CC) $(CFLAGS) -Dv_eng $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname, -o $(HOME)/lib/$(LIBNAME) $(LIB)

install:
	ln -s $(my_home)/lib/$(LIBNAME)	/usr/local/lib




