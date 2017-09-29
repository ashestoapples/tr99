LD_FLAGS=`pkg-config --cflags SDL_mixer`
C_FLAGS= -g -Wall -lncurses -lpthread -lSDL -lSDL_mixer 
CC=gcc
SOURCES=tui.c sound.c

OBJECTS=tui.o sound.o

all: tr99

tr99: $(OBJECTS) clock.c
	$(CC) $(LD_FLAGS) clock.c $(OBJECTS) $(C_FLAGS) -o tr99

tui.o: $(SOURCES)
	$(CC) $(LD_FLAGS) $(SOURCES) $(C_FLAGS) -c

sound.o: sound.c
	$(CC) $(LD_FLAGS) sound.c $(C_FLAGS) -c

clean:
	rm *.o tr99
