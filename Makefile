CC = gcc
CFLAGS = -m32 -Og -fcommon

LD = gcc
LFLAGS = -m32 -lm -lc -lSDL2 -lSDL2main

obj/%.o: src/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

CFILES  = $(wildcard src/*.c)
OFILES  = $(CFILES:src/%.c=obj/%.o)

all: bin/quake

bin/quake: $(OFILES)
	mkdir -p $(@D)
	$(LD) $(LFLAGS) $(OFILES) -o $@

test: all
	bin/quake +timedemo demo1 2> /dev/null

clean:
	rm -rf bin obj > /dev/null 2> /dev/null || true
