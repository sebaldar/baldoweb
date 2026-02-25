CC=g++
CFLAGS=-fPIC -Wall -Wextra -g -std=c++11
APPNAME=testUtil
DIR=$(HOME)/cpp

INCLUDE=	-I./ 				\
		-I$(DIR)/Utils			

	
util=$(DIR)/Utils/Utils.cpp

main=$(DIR)/Utils/main.cpp
SRCFILES=$(util) $(main)

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS)  $(INCLUDE) \
	   $(SRCFILES)  \
	   -o $(HOME)/bin/$(APPNAME)


