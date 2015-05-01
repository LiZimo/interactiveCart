checkTEEM = $(if $(TEEM),,\
$(warning *)\
$(warning *)\
$(warning * Environment variable TEEM not set.)\
$(warning * Set this to the path into your teem-install)\
$(warning * as documented at http://teem.sourceforge.net/build.html)\
$(warning *)\
$(warning *)\
$(error Make quitting))
$(checkTEEM)

## turning off stupid implicits
#.SUFFIXES:
#%:: RCS/%,v
#%:: RCS/%
#%:: s.%
#%:: SCCS/s.%

CFLAGS = -O3 -g
#CFLAGS = -g
#CFLAGS = -p
CC = gcc
#LIBOPTS = -static
LIBOPTS =
LIBS = -lteem -lfftw3 -lpng -lz -lm
#LIBS = -lfftw3 -lm
IPATH = -I$(TEEM)/include
LPATH = -L$(TEEM)/lib -L/usr/local/lib

# HEY still have to package as library                     

all: cart tcart
# all: cart cart2 cartv cart2v

.c.o:
	$(CC) -c $(CFLAGS) $(IPATH) -o $@ $<

cart:main.o cart.o
	$(CC) $(CFLAGS) $(LPATH) $(LIBOPTS) -o cart main.o cart.o $(LIBS)

tcart:tmain.o cart.o
	$(CC) $(CFLAGS) $(LPATH) $(LIBOPTS) -o tcart tmain.o cart.o $(LIBS)

cart2:main.o cart2.o
	$(CC) $(CFLAGS) $(LIBOPTS) -o cart2 main.o cart2.o $(LIBS)

cartv:main.o cartv.o
	$(CC) $(CFLAGS) $(LIBOPTS) -o cartv main.o cartv.o $(LIBS)

cart2v:main.o cart2v.o
	$(CC) $(CFLAGS) $(LIBOPTS) -o cart2v main.o cart2v.o $(LIBS)

clean:
	rm -fv *.o


