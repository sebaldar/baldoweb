CC=g++
CFLAGS=-fPIC -Wall -Wextra -g -std=c++11
APPNAME=webclient

LIB=-lXML

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

INCLUDE=	-I./ 					\
		-I$(DIR)/xml/include		\
		-I$(DIR)/Utils				\
		-I./include

webclient=./source/tcp_client.cpp
main=./main.cpp

SRCFILES=$(webclient) $(main) 

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS)  $(INCLUDE) \
	   $(SRCFILES)  $(LIB) \
	   -o $(HOME)/bin/$(APPNAME)




