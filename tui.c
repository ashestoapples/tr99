#include "tui.h"

#include <ncurses.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h> 
#include <time.h>

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

char * timeStamp()
{
	time_t cur = time(NULL);
	char *str_time;
	if (cur == ((time_t)-1))
	{
		return "[Unable to retrive time]";
	}

	str_time = ctime(&cur);

	if (str_time == NULL)
	{
		return "[Unable to format time to string]";
	}

	return str_time;
}

void mainMenu(FILE *log)
{
	initscr();
	raw();
	if (initSDL() == -1)
	{
		fprintf(log, "%s\nUnable to init SDL\n", timeStamp());
		endwin();
	}
	int ch = 0;
	float tempo = 120;

	clear();
	refresh();

	keypad(stdscr, TRUE);
	//menu relative vars
	int select = 0, //current selected item
		chan = 0, 	//current selected channel
		d_iter = 0;
	int bank_name = -1;
	Sample *bank[16]; 
	Channel *mix[16];
		
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
		printw("\tLoad Pattern Bank\n");
		attroff(A_STANDOUT);
		if (d_iter == 3) attron(A_STANDOUT);
		printw("\tGlobal Settings\n");
		attroff(A_STANDOUT);
		ch = getch();
		switch (ch)
		{
			case KEY_UP:
				d_iter = (d_iter > 0) ? d_iter-1 : d_iter;
				break;
			case KEY_DOWN:
				d_iter = (d_iter < 3) ? d_iter+1 : d_iter;
				break;
			case YES:
				switch (d_iter)
				{
					case 0:
						if (bank_name != -1 && validateSampleBank(bank) != -1)
						{
							patternEditor(log, tempo, ch, d_iter, mix, bank, bank_name);
						}
						else 
							bank_name = -1;
						break;
					case 1:
						bank_name = bankSelection(log, bank, bank_name);
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

int bankSelection(FILE *log, Sample *bank[16], int bankNumber)
{
	if (bankNumber != -1)
	{
		for (int i = 0; i < 16; i++)
		{
			if (bank[i] != NULL)
				destroySample(bank[i]);
		}
		fprintf(log, "%s\nPurged old sample bank from memory\n", timeStamp());
	}
	clear();
	refresh();
	printw("Select bank: (1-16): ");
	int ch = -1;
	while (ch < 0 || ch > 15)
		ch = handleFuckingButtons();
	ch++;
	bankNumber = ch;
	char bank_path[32], full_path[512] ;
	sprintf(bank_path, "banks/%d/", ch);
	DIR *dir;
	int i = 0;
	struct dirent *ent;
	if ((dir = opendir(bank_path)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if (strcmp(ent->d_name, "..") != 0 && strcmp(ent->d_name, ".") != 0)
			{
				sprintf(full_path, "%s%s\0", bank_path, ent->d_name);
				bank[i++] = initSample(full_path);
				if (bank[i-1] == NULL)
				{
					fprintf(log, "%s\nFAILED to load smaple into memory: %s\nReason: %s", timeStamp(), full_path, Mix_GetError());
				}
				memset(full_path, '\0', 128);
				if (i > 15)
					break;
			}
		}
	}
	for (; i < 16; i++)
		bank[i] = NULL;
	//getch();
	return ch;
}

void patternEditor(FILE *log, float tempo, int ch, int d_iter, Channel *mix[16], Sample *bank[16], int bank_name)
{
	clear();
	refresh();
	for (int i = 0; i < 16; i++)
	{
		if (bank[i] != NULL)
			mix[i] = initChannel(bank, i);
		else
			mix[i] = NULL;
	}
	//Channel *mix[16];
	//Sample *bank[16];
	// for (int i = 0; i < 16; i++)
	// 	mix[i] = NULL;
	// loadSampleBank("1.bank", bank);
	// for (int i = 0; i < 3; i++)
	// 	printw("Bank sound %d: %s\n", i, bank[i]->fname);
	// importSequence("2.track", mix, bank);
	//char *bank_name = "1.bank";
	
	d_iter = 0, ch = 0;
	int select = 0, chan = 0, bank_selected = 0, sound = 0;

	enum modes {SELECT, COMPOSE, EDIT};
	enum modes mode = SELECT;

	while (ch != 113)
	{
		bool empty = false;
		clear();
		refresh();
		attron(A_UNDERLINE);
		printw("Tempo: %d bpm | 'p' - play/pause\n\n", (int)tempo);//getch();
		attroff(A_UNDERLINE);
		for (int i = 0; i < 16; i++)
		{
			if (mode == EDIT && select == i)
				attron(A_STANDOUT);
			if (mix[chan] != NULL && mix[chan]->pattern[i] != NULL)
				printw("[*]");
			else
				printw("[ ]");
			attroff(A_STANDOUT);
		}//getch();
		printw("\n\nThe current Sample Bank is: %d\nMode: %d\n", bank_name, mode);
		if (mode == COMPOSE)
		{
			printw("The current selected sample is: %s", mix[chan]->sound->fname);
			printw("Press TAB to exit...");
		}
		else if (mode == EDIT)
		{
			printw("Press step to edit..\n");
			if (mix[chan]->pattern[select] == NULL)
				printw("This is an empty step!\n");
			else
			{
				printw("Sample:\t%s\n", mix[chan]->sound->fname);
				printw("Velocity:\t%f\n", mix[chan]->pattern[select]->vol);
				printw("Hold:\t%%100\n");
				printw("Pitch:\t~0\n");
				printw("Treble:\t1\t\n");
				printw("Mid:\t1\n");
				printw("Bass:\t1\n");
			}
		}
		else if (mode == SELECT)
		{
			printw("Select channel to edit\n");
			for (int i = 0; i < 16; i++)
			{
				printw("%d:\t", i);
				if (bank[i] != NULL)
					printw("%s\n", bank[i]->fname);
				else
					printw("empty\n");
			}			
		}

		ch = handleFuckingButtons();
		//printw("Handled buttons, ch = %d\n", ch);//getch();
		bool skip = true;
		if (ch == CHANGEMODE)
		{
			switch (mode)
			{
				case COMPOSE:
					mode = SELECT;
					select = 0;
					break;
				case SELECT:
					//printw("Here\n");getch();
					mode = EDIT;
					//printw("Mode: %d", mode);getch();
					//skip = false;
					break;
				case EDIT:
					mode = COMPOSE;
					skip = false;
					break;
			}
			//printw("Mode will be %d\n", mode); getch();
		}
		else if (ch == PLAY)
		{
			playingDisplay(log, tempo, ch, mix);
		}
		else if (ch == NO)
			return;
		if (!skip)
		{
			clear();
			refresh();
			printw("Entering compose mode, select bank patch (1-16)\n");
			for (int i = 0; i < 16; i++)
			{
				printw("%d:\t", i);
				if (bank[i] != NULL)
					printw("%s\n", bank[i]->fname);
				else
					printw("empty\n");
			}
			ch = handleFuckingButtons();
			if (ch > 15 || bank[ch] == NULL)
			{
				if (ch != CHANGEMODE)
				{
					printw("Cannot load empty bank slot, returning to select mode..\n");
					sleep(3);
				}
				mode = SELECT;
				select = 0;
			}
			else
			{
				chan = ch;
			}
		}
		else if (ch == EXPORT)
		{
			exportImportPattern(log, mix, bank, ch, 0);
			ch = 0;
		}
		else if (ch == IMPORT)
		{
			exportImportPattern(log, mix, bank, ch, 1);
			ch = 0;
		}
		else
		{
			if (mode == SELECT && ch >= 0 && ch < 16)
				chan = ch;
			else if (mode == COMPOSE && ch >= 0 && ch < 16)
			{
				if (mix[chan]->pattern[ch] == NULL)
				{
					mix[chan]->pattern[ch] = initStep(64);
				}
				else
				{
					destroyStep(mix[chan]->pattern[ch]);
					mix[chan]->pattern[ch] = NULL;
				}
			}
			else if (mode == EDIT)
			{
				if (ch >= 0 && ch < 16)
					select = ch;
				else
				{
					if (mix[chan]->pattern[select] != NULL)
					{
						if (ch == KEY_UP && mix[chan]->pattern[select]->vol  < 128)
							mix[chan]->pattern[select]->vol+=4;
						else if (ch == KEY_DOWN && mix[chan]->pattern[select]->vol > 4)
							mix[chan]->pattern[select]->vol-=4;
					}
				}
			}
		}
		//printw("Channel = %d\n", chan);getch();
	}
}

static int handleFuckingButtons()
{
	int select,
		ch = getch();
	switch(ch)
	{
		case B1:
			select = 0;
			break;
		case B2:
			select = 1;
			break;
		case B3:
			select = 2;
			break;
		case B4:
			select = 3;
			break;
		case B5:
			select = 4;
			break;
		case B6:
			select = 5;
			break;
		case B7:
			select = 6;
			break;
		case B8:
			select = 7;
			break;
		case B9:
			select = 8;
			break;
		case B10:
			select = 9;
			break;
		case B11:
			select = 10;
			break;
		case B12:
			select = 11;
			break;
		case B13:
			select = 12;
			break;
		case B14:
			select = 13;
			break;
		case B15:
			select = 14;
			break;
		case B16:
			select = 15;
			break;
		default:
			return ch;
	}
	return select;
}

void exportImportPattern(FILE *log, Channel *mix[16], Sample *bank[16], int ch, int mode)
{
	char *mdisplay;
	if (mode == 0)
		mdisplay = "export";
	else
		mdisplay = "import";
	clear();
	refresh();
	DIR *dir;
	struct dirent *ent;
	int blocks[16];
	for (int i = 0; i < 16; i++)
		blocks[i] = 0;
	if ((dir = opendir(PATTERN_LOCATION)) != NULL)
	{
		while ((ent = readdir(dir)) != NULL)
		{
			if (ent->d_name[0] > 48 && ent->d_name[0] < 58)
			{
				if (ent->d_name[0] == 49)
				{
					if (ent->d_name[1] != '.')
					{
						blocks[10 + (int)(ent->d_name[1]) - 49] = 1;
					}
					else
						blocks[10] = 1;
				}
				else
					blocks[(int)(ent->d_name[0]) - 49] = 1;
			}
		}
	}
	else
	{
		fprintf(log, "[%s] UNABLE TO READ PATTERN DIRECTORY", timeStamp());
		return;
	}
	printw("Choose slot to %s patern (1 - 16), CHANGMODE to cancel\n", mdisplay);
	for (int i = 0; i < 16; i++)
	{
		if (blocks[i] == 1)
			printw("[%d] - OCCUPIED\n", i+1);
		else
			printw("[%d] - EMPTY\n", i+1);
	}
	ch = 99999;
	while (ch > 15)
	{
		ch = handleFuckingButtons();
		if (ch == CHANGEMODE)
		{
			return;
		}
	}
	
	char fn[24];
	sprintf(fn, "./patterns/%d.track\0", ch + 1);
	fprintf(log, "[%s] opened %s for writing", timeStamp(), fn);

	FILE *fp;
	if (mode == 0)
	{
		fp = fopen(fn, "w");
		char line[1028],
			 chunk[16];

		for (int i = 0; i < 16; i++)
		{
			if (mix[i] == NULL || mix[i]->pattern == NULL)
			{
				if (bank[i] != NULL)
				{
					sprintf(line, "C%d: B%d, ()()()()()()()()()()()()()()()();\n", i, i);
					fprintf(fp, line);
				}
			}
			else
			{
				sprintf(line, "C%d: B%d, ", i, i);
				for (int j = 0; j < 16; j++)
				{
					if (mix[i]->pattern[j] == NULL)
						strcat(line, "()");
					else
					{
						sprintf(chunk, "(%d)", (int)mix[i]->pattern[j]->vol);
						strcat(line, chunk);
						memset(chunk, '\0', 16);
					}
				}
				strcat(line, ";\n\0");
				fprintf(fp, line);
			}
			memset(line, '\0', 1028);
		}
		fclose(fp);
	}
	else
	{
		if (blocks[ch] == 0)
		{
			printw("Cannot import empty pattern\n");
			sleep(2);
			return;
		}
		//printw("Before Destruction\n");
		//getch();
		for (int i = 0; i < 16; i++)
		{
			if (mix[i] != NULL)
			{
				for (int j = 0; j < 16; j++)
					destroyStep(mix[i]->pattern[j]);
				mix[i] = NULL;
			}
		}
		printw("Before import\n");
		getch();
		importSequence(fn, mix, bank);
	}
}

void playingDisplay(FILE *log,float tempo, int ch, Channel *mix[16])
{
	//playSample(mix[0]->sound, 0);
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
		pa.mix[j] = mix[j];
	}
	printw("Mix attribs copied\n");getch();
	pa.ch = ch;
	pa.i = i;
	pa.tempo = tempo;
	pthread_t player;
	//printf("Before pthread create\n;");
	pthread_create(&player, NULL, &playSequence, (void *)&pa);
	printw("Thread created\n");//getch();
	while (pa.ch != PAUSE)
	{
		//printw("LOOP\n");
		//playStep(&(seq[i]));
		//steps = ;
		if (pa.i != prev)
		{
			clear();
			refresh();
			prev = pa.i;
			printw("Tempo: %d\n", (int)(pa.tempo/4));
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
				pa.tempo+=4;
				tempo++;
				break;
			case KEY_DOWN:
				pa.tempo-=4;
				tempo--;
				break;
		}
		//pa.steps = (60/(tempo) * 1000000) / 4;
	}
	usleep(steps);
	//destroy_p_args(pa);
	nodelay(stdscr, FALSE);

	return;
}
