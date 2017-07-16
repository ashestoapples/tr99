#include "tui.h"

#include <ncurses.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> 

#include "sound.h"
#include "keyboard.h"

static bool debug = true;

//prototype 
/* visual file browser fn */ 
void fileBrowse(char buf[128])
{
	/* @param buf[128] : buffer that will be populated with the user selection path */

	nodelay(stdscr, FALSE); //WAIT for user input
	int x, y, offset = 0;	//screen attribs
	DIR *dir;
	struct dirent *ent;
	strcpy(buf, PATCH_ROOT);
	getmaxyx(stdscr, y, x);
	strcpy(buf, PATCH_ROOT);
	file_iterate(buf, dir, ent, offset, y-2);
}

/* recursive helper function for fileBrowse*/
static void file_iterate(char name[512], DIR *dir, struct dirent *ent, int offset, int max_y)
{
	/* @param name : char buffer to be populated
	   @param dir : dir object used in dirent
	   @praram ent : " used in dirent
	   @param offset : what display page we're on
	   @param max_y : the max y dimension of the terminal */
	int selection = 0;
	while (true)
	{
		clear();
		refresh();
		attron(A_UNDERLINE);
		printw("Inside dir: %s\n", name);
		attroff(A_UNDERLINE);
		if ((dir = opendir(name)) != NULL)
		{
			int i = 0, j = 0;
			char *n = NULL;
			int ch = 0;
			while ((ent = readdir(dir)) != NULL && j < max_y)
			{
				if (! (i < offset))
				{
					if (selection == j) 
					{
						attron(A_STANDOUT);
						n = ent->d_name;
					}
					printw("%s\n", ent->d_name);
					j++;
					attroff(A_STANDOUT);
				}
				else 
					i++;
			}
			ch = getch();
			switch(ch)
			{
				case 10:
					strcat(name, n);
					strcat(name, "/");
					file_iterate(name, dir, ent, offset, max_y);
					return;
				case KEY_DOWN:
					selection++;
					if (selection > max_y)
					{
						selection = 0;
						offset += max_y;
					}
					break;
				case KEY_UP:
					selection--;
					if (selection < 0)
					{
						selection = 0;
						offset = offset > 0 ? offset - max_y : 0;
					}
					break;
				case 8: //backspace
					if (strcmp(PATCH_ROOT, name) != 0)
					{
						strcat(name, "../");
						file_iterate(name, dir, ent, offset, max_y);
						return;
					}
					break;
			}
		}
		else
		{
			char c = name[0];
			int i = 0;
			while (c != '\0')
				c = name[i++];
			name[i-2] = '\0';
			printf("%s\n", name);
			//getch();
			return;
		}
	}
}

void mainMenu()
{
	initscr();
	raw();
	int ch = 0;
	float tempo = 120;

	clear();
	refresh();

	//Step sequance to be played
	Step *seq[16];
	//initialize all steps
	for (int i = 0; i < 16; i++)
		seq[i] = initStep();
	if (debug) printf("before fn call\n");
	importSequence("2.track", seq);

	keypad(stdscr, TRUE);
	//menu relative vars
	int select = 0, //current selected item
		chan = 0, 	//current selected channel
		d_iter = 0; 
		
	clear();
	refresh();
	while (true)
	{
		echo();
		clear();
		refresh();
		attron(A_UNDERLINE);
		printw("Tr99 - v%f | Tempo %d | \n", VERSION_NUMBER, tempo);
		attroff(A_UNDERLINE);
		if (d_iter == 0) attron(A_STANDOUT);
		printw("\tPattern Selection\n");
		attroff(A_STANDOUT);
		if (d_iter == 1) attron(A_STANDOUT);
		printw("\tLoad Sound Bank\n");
		attroff(A_STANDOUT);
		if (d_iter == 2) attron(A_STANDOUT);
		printw("\tGlobal Settings\n");
		attroff(A_STANDOUT);
		ch = getch();
		switch (ch)
		{
			case KEY_UP:
				d_iter = (d_iter > 0) ? d_iter-1 : d_iter;
				break;
			case KEY_DOWN:
				d_iter = (d_iter < 2) ? d_iter+1 : d_iter;
				break;
			case YES:
				switch (d_iter)
				{
					case 0:
						patternEditor(tempo, ch, d_iter);
						break;
					case 1:
						break;
					case 2:
						break;

				}
				break;
			case NO:
				endwin();
				return;
				break;
		}
	}
	endwin();

}

void patternEditor(float tempo, int ch, int d_iter)
{
	Step *seq[16];
	//initialize all steps
	for (int i = 0; i < 16; i++)
		seq[i] = initStep();
	if (debug) printf("before fn call\n");
	importSequence("2.track", seq);
	
	d_iter = 0, ch = 0;
	int select = 0, chan = 0;


	while (ch != 113)
	{
		bool empty = false;
		clear();
		refresh();
		printw("Tempo: %d bpm | 'p' - play/pause\n", (int)tempo);
		for (int h = 0; h < 16; h++)
		{
			if ((h) % 4 == 0 || h == 0)
				attron(A_STANDOUT);
			if (select == h)
				printw("[X]");
			else
				printw("[ ]");
			attroff(A_STANDOUT);
		}


		if (seq[select]->sound_c > 0)
		{
			if (d_iter == 0) attron(A_BOLD);
			printw("\n\nStep: %d/16\n", select + 1);
			attroff(A_BOLD);
			empty = false;
			if (d_iter == 1) attron(A_BOLD);
			printw("Channel %d/%d\n\n", chan + 1, seq[select]->sound_c);
			attroff(A_BOLD);
			if (d_iter == 2) attron(A_BOLD);
			printw("Sample Path:\t%s\n", seq[select]->sounds[chan]->fname);
			attroff(A_BOLD);
			if (d_iter == 3) attron(A_BOLD);
			printw("Velocity:\t%f\n", seq[select]->sounds[chan]->vol);
			attroff(A_BOLD);
			if (d_iter == 4) attron(A_BOLD);
			printw("Accent:\t\t0.5\n");
			attroff(A_BOLD);
			if (d_iter == 5) attron(A_BOLD);
			printw("Bass:\t\t1\n");
			attroff(A_BOLD);
			if (d_iter == 6) attron(A_BOLD);
			printw("Treble:\t\t1\n");
			attroff(A_BOLD);
			if (d_iter == 7) attron(A_BOLD);
			printw("Hold:\t\t100%%\n");
			attroff(A_BOLD);
			if (d_iter == 8) attron(A_BOLD);
			printw("\n[Add Channel] ");
			attroff(A_BOLD);
			if (d_iter == 9) attron(A_BOLD);
			printw(" [Remove Channel]");
			attroff(A_BOLD);
		}
		else
		{
			if (d_iter > 1)
				d_iter = 0;
			empty = true;
			if (d_iter == 0) attron(A_BOLD);
			printw("\n\nStep: %d/16\n", select + 1);
			attroff(A_BOLD);
			if (d_iter == 1) attron(A_BOLD);
			printw("[New Step]\n");
			attroff(A_BOLD);
		}
		ch = getch();
		clear();
		refresh();
		char fbuf[512];
		switch(ch)
		{
			case KEY_UP:
				d_iter = d_iter > 0 ? d_iter-1 : d_iter;
				break;
			case KEY_DOWN:
				if (seq[select]->sound_c != 0)
					d_iter = d_iter < 9 ? d_iter+1 : d_iter;
				else
					d_iter = d_iter < 1 ? d_iter+1 : d_iter;
				break;
			case KEY_LEFT:
				switch (d_iter)
				{
					case 0:
						select = select > 0 ? select-1 : select;
						chan = 0;
						break;
					case 1:
						chan = chan > 0 ? chan-1 : chan;
						break;
					case 2:
						break;
					case 3:
						if (seq[select]->sounds[chan]->vol > 0.1) seq[select]->sounds[chan]->vol -= 0.1;
						break;
				}
				break;
			case KEY_RIGHT:
				switch (d_iter)
				{
					case 0:
						select = select < 15 ? select+1 : select;
						chan = 0;
						break;
					case 1:
						chan = chan < seq[select]->sound_c - 1 ? chan+1 : chan;
						break;
					case 2:
						break;
					case 3:
						if (seq[select]->sounds[chan]->vol < 1.5) seq[select]->sounds[chan]->vol += 0.1;
						break;
				}
				break;
			case DELETE: 
					if (!empty)
					{
						if (chan + 1 != seq[select]->sound_c)
						{
							while (chan < seq[select]->sound_c - 1)
							{
								seq[select]->sounds[chan] = seq[select]->sounds[chan+1];
								chan++;
							}
						}
						//free(seq[select].sounds[seq[select].sound_c].fname);
						seq[select]->sound_c--;
						chan = 0;
					}
					break;
			case PLAY:
				playingDisplay(tempo, ch, seq);
				break;
			case NO:
				for (int j = 0; j < 16; j++)
					destroyStep(seq[j]);
				return;
		}
	}
}

void playingDisplay(float tempo, int ch, Step *seq[16])
{
	nodelay(stdscr, TRUE);
	int i = 0, prev = i;
	//printf("Before float operation\n");
	float steps = (60/(tempo) * 1000000) / 4;
	clear();
	refresh();
	ch = 0;
	//printf("Before assigning args\n;");
	struct p_args pa;
	//pa.seq = seq;
	for (int j = 0; j < 16; j++)
	{
		pa.com[j] = gen_command(seq[j]);
		pa.seq[j] = seq[j];
	}

	pa.ch = ch;
	pa.i = i;
	pa.steps = steps;
	pthread_t player;
	//printf("Before pthread create\n;");
	pthread_create(&player, NULL, &playSequence, (void *)&pa);
	while (pa.ch != PAUSE)
	{
		//playStep(&(seq[i]));
		//steps = ;
		if (pa.i != prev)
		{
			clear();
			refresh();
			prev = pa.i;
			printw("Tempo: %d\tSteps: %d\n", (int)tempo, (int)pa.steps);
			for (int j = 0; j < 16; j++)
			{
				if ((j) % 4 == 0 && j != 15 || j == 0)
					attron(A_STANDOUT);
				if (pa.i == j)
					printw("[*]");
				else
					printw("[ ]");
				if ((j) % 4 == 0 && j != 15 || j == 0)
					attroff(A_STANDOUT);
			}
		}

		//printf("\n");
		pa.ch = getch();
		//i = (i < 15) ? i+1 : 0;
		//usleep((int)steps);
		switch(pa.ch)
		{
			case KEY_UP:
				tempo++;
				break;
			case KEY_DOWN:
				tempo--;
				break;
		}
		pa.steps = (60/(tempo) * 1000000) / 4;
	}
	usleep(steps);
	destroy_p_args(pa);
	nodelay(stdscr, FALSE);

	return;
}
