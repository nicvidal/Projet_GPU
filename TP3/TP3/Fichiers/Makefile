src=$(wildcard *.c)
tous=$(src:.c=)
CFLAGS=-Wall -O3 -fopenmp
LDLIBS=-lm 
LDFLAGS=-fopenmp

## CC=gcc-mp-6

all : $(tous)


clean:		   
		   -/bin/rm -f $(tous)
