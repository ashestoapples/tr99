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
#define PATTERN_LOCATION "patterns/"
/* this is the path of the log file, functions refer to the FILE* created by the main menu function for writing */
#define LOG_FILE "err.log"


#define DEGBUG true

#define VERSION_NUMBER 0.01

/* helper function, recursivler parse a directory */
static void file_iterate(char name[128], DIR *dir, struct dirent *ent, int offset, const int max_cols);
/* file browser ui*/
void fileBrowse(char buf[128]);
/* handle uper row input */
static int handleFuckingButtons();
/* menu for saveing/loading pattern */
void exportImportPattern(FILE *log, Channel *mix[16], Sample *bank[16], int ch, int mode);
/* main menu */
void mainMenu();
/* menu for editing patterns*/
void patternEditor(FILE *log, float tempo, int ch, int d_iter, Channel *mix[16], Sample *bank[16], int bank_name);
/* displayed when beat is playing */
int playingDisplay(FILE *log, float tempo, int ch, Channel *mix[16]);
/* menu for selecting sound bank */
int bankSelection(FILE *log, Sample *bank[16], int bankNumber);
/* create a char[] timestamp for writing to the log file*/
char * timeStamp();

#endif