CC=g++
CFLAGS=-fPIC -Wall -Wextra -std=c++11 
LIBNAME=libcgi.so

my_home=/home/sergio
DIR=$(HOME)/programmi/c++
DIR11=$(HOME)/programmi/c++11

INCLUDE=	-I./ -I$(DIR)/Utils			
LIB=
		
cgi=$(DIR11)/cgi/cgi_base.cpp
SRCFILES=$(cgi) 


lib : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname, -o $(HOME)/lib/$(LIBNAME) $(LIB)
	   
clean:
	@echo PULIZIA DEL SISTEMA
	rm -f *.o *~ Log.txt

install:
	ln -s $(my_home)/lib/$(LIBNAME)	/usr/local/lib
  

		
		

