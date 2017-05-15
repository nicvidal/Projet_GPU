ARCH            := $(shell uname -s | tr a-z A-Z)

CFLAGS		:= -std=c99 -O3

ifeq ($(ARCH),DARWIN)
LDLIBS		:= -framework OpenCL
else
LDLIBS		:= -lOpenCL
endif

CL_SOURCE	:= $(wildcard *.cl)
OBJECT		:= $(CL_SOURCE:.cl=.o)
EXEC		:= $(CL_SOURCE:.cl=)

CFLAGS		+= -DKERNEL_FILE=\"$(CL_SOURCE)\"

target: $(EXEC)

$(EXEC): util.o

$(OBJECT): util.h

util.o: util.h

clean:
	rm -rf $(EXEC) *.o
