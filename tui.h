#ifndef TUI_H_INCLUDED
#define TUI_H_INCLUDED

#include <ncurses.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "sound.h"

/* root directory of where we search for pathc files*/
#define PATCH_ROOT "./"
#define PATTERN_LOCATION "./patterns"

#define DEGBUG true

#define VERSION_NUMBER 0.01

static void file_iterate(char name[128], DIR *dir, struct dirent *ent, int offset, const int max_cols);
void fileBrowse(char buf[128]);

void mainMenu();
void patternEditor(float tempo, int ch, int d_iter);
void playingDisplay(float tempo, int ch, Step *seq[16]);

#endif