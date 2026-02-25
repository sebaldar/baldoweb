CC=g++
CFLAGS=-fPIC -Wall -Wextra -g -std=c++11
APPNAME=crypt
DIR=$(HOME)/programmi/c++

INCLUDE=	-I./include	
			

sha1=$(DIR)/crypt/source/sha1.cpp
base64=$(DIR)/crypt/source/base64.cpp
main=$(DIR)/crypt/main.cpp

SRCFILES=$(server) $(sha1) $(base64) $(main)

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS)  $(INCLUDE) 	\
	   $(SRCFILES)					 \
	   -o $(HOME)/bin/$(APPNAME)




