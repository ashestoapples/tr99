#ifndef TUI_H_INCLUDED
#define TUI_H_INCLUDED

#include <ncurses.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "sound.h"

/* root directory of where we search for pathc files*/
#define PATCH_ROOT "./"
#define PATTERN_LOCATION "./patterns"
#define LOG_FILE "err.log"


#define DEGBUG true

#define VERSION_NUMBER 0.01

static void file_iterate(char name[128], DIR *dir, struct dirent *ent, int offset, const int max_cols);
void fileBrowse(char buf[128]);
static int handleFuckingButtons(int ch);

void mainMenu();
void patternEditor(FILE *log, float tempo, int ch, int d_iter, Channel *mix[16], Sample *bank[16], int bank_name);
void playingDisplay(FILE *log, float tempo, int ch, Channel *mix[16]);
int bankSelection(FILE *log, Sample *bank[16], int bankNumber);

char * timeStamp();

#endif