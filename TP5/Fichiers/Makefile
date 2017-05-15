src=$(wildcard *.c)
obj=$(src:.c=.o)
CFLAGS=-Wall -g -O3 -fopenmp
LDLIBS=-lm 
LDFLAGS=-fopenmp

PRG=tsp-main

$(PRG): $(obj)

clean:		   
		   -/bin/rm -f $(obj) $(PRG)
