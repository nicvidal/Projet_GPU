LIB_MODE=

PROGRAM	:= prog

# Must be the first rule
.PHONY: default
default: $(PROGRAM)

SOURCES := $(wildcard src/*.c)

OBJECTS := $(SOURCES:src/%.c=obj/%.o)
DEPENDS := $(SOURCES:src/%.c=deps/%.d)

MAKEFILES := Makefile

LIB	:= lib/libvisu.a

CFLAGS += -O3 -g -Wall -Wno-unused-function
CFLAGS += -I./include

CFLAGS += -fopenmp
LDFLAGS += -fopenmp


CFLAGS += $(shell pkg-config SDL2_image --cflags)
LDLIBS += $(shell pkg-config SDL2_image --libs) -lpthread



$(OBJECTS): $(MAKEFILES)

ifndef LIB_MODE
$(LIB): $(OBJECTS)
	ar -crs $(LIB) $^
endif

$(PROGRAM): $(LIB)
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

$(OBJECTS): obj/%.o: src/%.c
	$(CC) -o $@ $(CFLAGS) -c $<

.PHONY: depend
depend: $(DEPENDS)

$(DEPENDS): $(MAKEFILES)

$(DEPENDS): deps/%.d: src/%.c
	$(CC) $(CFLAGS) -MM $< | \
		sed -e 's|\(.*\)\.o:|deps/\1.d obj/\1.o:|g' > $@

ifneq ($(MAKECMDGOALS),clean)
-include $(DEPENDS)
endif

.PHONY: clean
clean: 
	rm -f prog obj/*.o deps/*.d lib/*.a
