CC=g++
CFLAGS=-g
APPNAME=plot
DIR=$(HOME)/programmi/c++
LIB=-lgd -lpng -lz -ljpeg -lfreetype -lm

INCLUDE=	-I./ 					\
		-I$(DIR)/Utils				\
		-I$(DIR)/xml/include		\
		-I$(DIR)/Mat/Include		\
		-I$(DIR)/cgi					\
		-I$(DIR)/Curva/include	

DIRLIB=						\
		-L/usr/local/lib	

SRCFILES=$(DIR)/Curva/source/Curva.cpp			\
		$(DIR)/Curva/source/matrice.cpp			\
		$(DIR)/Utils/field.cpp					\
		$(DIR)/Utils/Utils.cpp					\
		$(DIR)/xml/source/xml.cpp				\
		$(DIR)/xml/source/parser.cpp			\
		$(DIR)/cgi/cgi_base.cpp				\
		main.cpp


$(APPNAME) : $(SRCFILES)
	   $(CC) $(LIB) $(CFLAGS) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

Debug : $(SRCFILES)
	   $(CC) $(LIB) $(CFLAGS) $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

install:
	cp $(HOME)/bin/$(APPNAME)	/var/www/cgi-bin/
