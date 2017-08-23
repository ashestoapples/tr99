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

//this whole thing is fucking spagetti

char *pattern_bank_path = "patterns/1/";

/* visual file browser fn */ 
void fileBrowse(char buf[128])
{
	/* @param buf[128] : buffer that will be populated with the user selection path */

	nodelay(stdscr, FALSE); //WAIT for user input
	int x, y, offset = 0;	//screen attribs
	DIR *dir;
	struct dirent *ent;
	strcpy(buf, PATCHES_SEARCH);
	getmaxyx(stdscr, y, x);
	strcpy(buf, PATCHES_SEARCH);
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
			printf("Selected: %s\n", name);
			//getch();
			return;
		}
	}
}

/* create a timestamp for error log writting */
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

/* initial menu, */
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
		printw("\tEdit Current Sound Bank\n");
		attroff(A_STANDOUT);
		if (d_iter == 3) attron(A_STANDOUT);
		printw("\tLoad Pattern Bank\n");
		attroff(A_STANDOUT);
		if (d_iter == 4) attron(A_STANDOUT);
		printw("\tGlobal Settings\n");
		attroff(A_STANDOUT);
		if (d_iter == 5) attron(A_STANDOUT);
		printw("\tManage Saved Patterns\n");
		attroff(A_STANDOUT);
		ch = getch();
		switch (ch)
		{
			case KEY_UP:
				d_iter = (d_iter > 0) ? d_iter-1 : d_iter;
				break;
			case KEY_DOWN:
				d_iter = (d_iter < 4) ? d_iter+1 : d_iter;
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
						if (bank_name != -1)
						{
							bankEditor(log, bank, bank_name);
						}
						else 
							bank_name = -1;
						break;
					case 3:
						patternBankSelection(log);
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

/* display for selecting a sample bank */
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
	sprintf(bank_path, "banks/%d.bank", ch);
	if ( access( bank_path, F_OK ) == -1)
	{
		FILE *fp = fopen(bank_path, "w");
		fclose(fp);
	}
	loadSampleBank(bank_path, bank);
	//getch();
	return ch;
}

void bankEditor(FILE *log, Sample *bank[16], int bankNumber)
{
	clear();
	refresh();
	int ch = 0;
	while (ch != NO)
	{
		for (int i = 0; i < 16; i++)
		{
			if (bank[i] != NULL)
				printw(" %d - [%s]\n", i, bank[i]->fname);
				else
				printw("%d - [ EMPTY ]\n", i);
		}
		ch = handleFuckingButtons();
		if (ch < 16)
		{
			int snd = ch;
			clear();
			refresh();
			printw("\t Delete or Replace ? (esc/enter)\nAnyother key to return\n");
			ch = getch();
			if (ch == DELETE)
			{
				destroySample(bank[snd]);
				char *fn =  malloc(sizeof(char)*32);
				sprintf(fn, "%s%d.bank", PATCH_ROOT, bankNumber);
				updateSampleBank(fn, bank);
				free(fn);
			}
			else if (ch == YES)
			{
				char p[128];
				printw("Entering file browser\n");getch();
				fileBrowse(p);
				destroySample(bank[snd]);
				bank[snd] = initSample(p);
				clear(); refresh();
				printw("Making some room\n");getch();
				char *fn =  malloc(sizeof(char)*32);
				printw(" Updating sample bank\n"); getch();
				sprintf(fn, "%s%d.bank", PATCH_ROOT, bankNumber);
				updateSampleBank(fn, bank);
				free(fn);
			}
			ch = 0;
		}	
	}
}

void patternBankSelection(FILE *log)
{
	clear();
	refresh();
	int ch = 99;
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(pattern_bank_path)) != NULL)
	{
		;
	}
	else
	{
		fprintf(log, "[%s] UNABLE TO READ PATTERN DIRECTORY", timeStamp());
		return;
	}
	while ((ch = handleFuckingButtons()+1) > 16 && ch > 0)
	{
		printw("Using pattern bank: %s", pattern_bank_path);
		for (int i = 0 ; i < 16; i ++)
		{
			printw(" [%d] \n", i+1);
		}
	}
	pattern_bank_path = malloc(sizeof(sizeof(char)*24));
	fprintf(log, "[%s] patern dir is now: %s\n", timeStamp(), pattern_bank_path);
	sprintf(pattern_bank_path, "patterns/%d/", ch);
}

/* menu for editing samples */
void patternEditor(FILE *log, float tempo, int ch, int d_iter, Channel *mix[16], Sample *bank[16], int bank_name)
{
	/* @param log - *FILE error log object
		@param tempo - bpm tempo
		@param ch - input character 
		@param d_iter - itereate used to keep track of position in menu 
		@param mix - pointer array to 16 mixing channels 
		@param bank - pointer array to each sample in a bank
		@param bank_name - int number of bank*/
	clear();
	refresh();

	/* initialize mixer channels*/
	for (int i = 0; i < 16; i++)
	{
		if (bank[i] != NULL)
			mix[i] = initChannel(bank, i);
		else
			mix[i] = NULL;
	}

	d_iter = 0, ch = 0;
	int select = 0, chan = 0, bank_selected = 0, sound = 0;

	enum modes {SELECT, COMPOSE, EDIT};
	enum modes mode = SELECT;

	Step *clipBoard = NULL; /* Step object acting as clipboard for pattern editing */

	while (ch != 113)
	{
		bool empty = false;
		clear();
		refresh();
		/* tempo/pattern output which is always printed */
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

		/* mode dependant display options */
		if (mode == COMPOSE)
		{
			printw("The current selected sample is: %s", mix[chan]->sound->fname);
			printw("Press TAB to exit...");
		}
		else if (mode == EDIT)
		{
			/* edit induvidual step attribs */
			printw("Press step to edit..\n");
			if (mix[chan]->pattern[select] == NULL)
				printw("This is an empty step!\n");
			else
			{
				printw("Sample:\t%s\n", mix[chan]->sound->fname);
				printw("Velocity:\t%f\n", mix[chan]->pattern[select]->vol);
				if (mix[chan]->pattern[select]->trim >= 500)
					printw("trim:\t\t%%100\n");
				else
					printw("trim:\t\t%d ms\n", mix[chan]->pattern[select]->trim);
				printw("Delay:\t\t %d ms\n", mix[chan]->pattern[select]->delay);
				printw("Probability\t %s\n", mix[chan]->pattern[select]->prob);
				printw("Pitch:\t~0\n");
				printw("Treble:\t1\t\n");
				printw("Mid:\t1\n");
				printw("Bass:\t1\n");
			}
		}
		else if (mode == SELECT)
		{
			/* select mixing channel for editing */
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

		/* get user input */
		ch = handleFuckingButtons();
		//printw("Handled buttons, ch = %d\n", ch);//getch();
		bool skip = true;
		/* switch the mode */
		if (ch == CHANGEMODE)
		{
			destroyStep(clipBoard); //the clipboard is erased when we change modes
			clipBoard = NULL;
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
			tempo = playingDisplay(log, tempo, ch, mix);
		}
		else if (ch == NO)
			return;
		if (!skip)
		{
			/* extra menu if we are switching into compose mode, ask the user which sample they would like to compose with */
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
			printw("Importing\n"); getch();
			exportImportPattern(log, mix, bank, ch, 1);
			ch = 0;
		}
		else
		{
			/* here is were we handle 16 button specific input for all cases */
			//selecting channel
			if (mode == SELECT && ch >= 0 && ch < 16)
				chan = ch;
			//adding/removing steps from the pattern
			else if (mode == COMPOSE && ch >= 0 && ch < 16)
			{
				if (mix[chan]->pattern[ch] == NULL)
				{
					mix[chan]->pattern[ch] = initStep(64, 500, 0, "%100");
				}
				else
				{
					destroyStep(mix[chan]->pattern[ch]);
					mix[chan]->pattern[ch] = NULL;
				}
			}
			//selecting/editing step attributes
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
						else if (ch == KEY_LEFT && mix[chan]->pattern[select]->trim > 25)
							mix[chan]->pattern[select]->trim-=10;
						else if (ch == KEY_RIGHT && mix[chan]->pattern[select]->trim < 500)
							mix[chan]->pattern[select]->trim+=10;
						else if (ch == DEL_UP && mix[chan]->pattern[select]->delay < 250)
							mix[chan]->pattern[select]->delay+=10;
						else if (ch == DEL_DN && mix[chan]->pattern[select]->delay > 0)
							mix[chan]->pattern[select]->delay-=10;
						else if (ch == PROB_U && strcmp(mix[chan]->pattern[select]->prob, "%100") != 0)
						{
							if (mix[chan]->pattern[select]->prob[0] == '%')
							{
								if (mix[chan]->pattern[select]->prob[1] == '9')
									strcpy(mix[chan]->pattern[select]->prob, "1/16");
								else
								{
									mix[chan]->pattern[select]->prob[1] += 1;
								}
							}
							else
							{
								if (strcmp(mix[chan]->pattern[select]->prob, "1/2") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "%100");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/3") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/2");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/4") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/3");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/6") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/4");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/8") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/6");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/10") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/8");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/12") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/10");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/16") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/12");
							}
						}
						else if (ch == PROB_D && strcmp(mix[chan]->pattern[select]->prob, "%10") != 0)
						{
							if (mix[chan]->pattern[select]->prob[0] == '%')
							{
								if (strcmp(mix[chan]->pattern[select]->prob, "%100") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/2");
								else 
									mix[chan]->pattern[select]->prob[1] -= 1;
							}
							else
							{
								if (strcmp(mix[chan]->pattern[select]->prob, "1/2") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/3");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/3") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/4");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/4") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/6");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/6") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/8");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/8") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/10");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/10") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/12");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/12") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "1/16");
								else if (strcmp(mix[chan]->pattern[select]->prob, "1/16") == 0)
									strcpy(mix[chan]->pattern[select]->prob, "%90");
							}
						}
						else if (ch == 99)
						{
							clipBoard = initStep(mix[chan]->pattern[select]->vol,
												 mix[chan]->pattern[select]->trim,
												 mix[chan]->pattern[select]->delay,
												 mix[chan]->pattern[select]->prob);
						}
						else if (ch == 112 && clipBoard != NULL)
						{
							destroyStep(mix[chan]->pattern[select]);
							mix[chan]->pattern[select] = initStep(clipBoard->vol, clipBoard->trim, clipBoard->delay, clipBoard->prob);
						}

					}
					else
					{
						if (ch == 112 && clipBoard != NULL)
						{
							mix[chan]->pattern[select] = initStep(clipBoard->vol, clipBoard->trim, clipBoard->delay, clipBoard->prob);
						}
					}
				}
			}
		}
		//printw("Channel = %d\n", chan);getch();
	}
}
/* handle top row input*/
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

/* used to save/load patterns */
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
	/* find and display open/full save slots */
	int blocks[16];
	for (int i = 0; i < 16; i++)
		blocks[i] = 0;
	if ((dir = opendir(pattern_bank_path)) != NULL)
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
					{
						blocks[0] = 1;
					}
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
	sprintf(fn, "%s%d.track\0", pattern_bank_path, ch + 1);
	fprintf(log, "[%s] opened %s for writing", timeStamp(), fn);

	FILE *fp;
	/* write if we're writing */
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
						sprintf(chunk, "(%d,%d,%d,%s)", (int)mix[i]->pattern[j]->vol, (int)mix[i]->pattern[j]->trim, (int)mix[i]->pattern[j]->delay, mix[i]->pattern[j]->prob);
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
	else /* read is we're reading */
	{
		if (blocks[ch] == 0)
		{
			printw("Cannot import empty pattern\n");
			sleep(2);
			return;
		}
		//printw("Before Destruction\n");
		//getch();
		/* destroy all step objects before importing */
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
		//the import function is in sound.c for some reason
		importSequence(fn, mix, bank);
	}
}

/* screen displaed while beat is playing */
int playingDisplay(FILE *log,float tempo, int ch, Channel *mix[16])
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
	/* we created a different thread to act as a metronome ,
		the p_args object contains pointers which refer our mixer,
		both threads read from the same struct in memory, however only the input_processing thread can write.
		The metronome thread terminates when the input thread catches the correct key code */
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
			printw("Tempo: %d\n", (int)(tempo));
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
				break;
			case KEY_DOWN:
				pa.tempo-=4;
				break;
		}
		tempo = (int)pa.tempo / 4;
		//pa.steps = (60/(tempo) * 1000000) / 4;
	}
	usleep(steps);
	//destroy_p_args(pa);
	nodelay(stdscr, FALSE);

	return tempo;
}
