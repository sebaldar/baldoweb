CC=g++
CFLAGS=-fPIC -Wall -Wextra -pedantic -std=c++11 -Dv_eng -DHAVE_ROUND $(INCLUDE)

CFLAGS_DEBUG=-g


APPNAME=nova
LIBNAME=libnova

my_home=/home/sergio
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

INCLUDE=-I./

SRCFILES= ./libnova.cpp	\
		./aberration.c	\
		./dynamical_time.c	\
		./jupiter.c	\
		./mars.c	\
		pluto.c	\
		sidereal_time.c	\
		vsop87.c	\
		airmass.c	\
		earth.c		\
		mercury.c	\
		precession.c	\
		solar.c	\
		angular_separation.c	\
		elliptic_motion.c	\
		lunar.c	\
		neptune.c	\
		proper_motion.c	\
		transform.c	\
		apparent_position.c	\
		heliocentric_time.c	\
		nutation.c	\
		refraction.c	\
		uranus.c	\
		asteroid.c	\
		hyperbolic_motion.c	\
		parabolic_motion.c	\
		rise_set.c	\
		utility.c	\
		comet.c	\
		saturn.c	\
		venus.c	\
		julian_day.c	\
		parallax.c


$(APPNAME) :   ./main.cpp $(SRCFILES)
	   $(CC) $(CFLAGS)  ./main.cpp $(SRCFILES) $(LIB) -o $(HOME)/bin/$(APPNAME)

lib : $(SRCFILES)
	   $(CC) $(CFLAGS) $(INCLUDE) $(SRCFILES) -Wno-deprecated -shared -Wl,-soname, -o $(HOME)/lib/$(LIBNAME).so 
	   
Debug : $(SRCFILES)
	   $(CC) $(CFLAGS_DEBUG)  -DHAVE_ROUND $(INCLUDE) $(SRCFILES) -o $(HOME)/bin/$(APPNAME)

install:
	ln -s $(my_home)/lib/$(LIBNAME).so	/usr/lib
