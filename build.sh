#!/usr/bin/bash
gcc $(pkg-config --cflags SDL_mixer) clock.c tui.c sound.c -lncurses -lpthread -lSDL -lSDL_mixer -o clock