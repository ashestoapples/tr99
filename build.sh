#!/usr/bin/bash
gcc -lncurses -lpthread clock.c tui.c sound.c -o clock
