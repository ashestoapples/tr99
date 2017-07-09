#include <unistd.h>
#include <stdio.h>
#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>

/* root directory of where we search for pathc files*/
#define PATCH_ROOT "./"

/* debug flag */
static const bool debug = true;

/* for cating unexpected interupt sequences */
static volatile int run = 1;

/* struct containing sample path and attributes */
typedef struct sample
{
	char *fname;
	float vol;
} Sample;

/* reserve memory for sample */
Sample * initSample(char *fn, float v)
{
	Sample *ptr = (Sample*)malloc(sizeof(Sample));
	ptr->fname = (char*)malloc(sizeof(char)*128);
	strcpy(ptr->fname, fn);
	ptr->vol = v;
	return ptr;
}

/* free memory from saple struct */
void destroySample(Sample *ptr)
{
	free(ptr->fname);
	free(ptr);
	ptr = NULL;
}

/* 16 channel sequence step */
typedef struct step
{
	Sample *sounds[16];
	int sound_c;
} Step;

/* reserve memory for Step struct */
Step * initStep()
{
	Step *ptr = (Step*)malloc(sizeof(Step));
	ptr->sound_c = 0;
	return ptr;
}

/* free memory from Step struct */
void destroyStep(Step *ptr)
{
	for (int i = 0; i < ptr->sound_c; i++)
	{
		destroySample(ptr->sounds[i]);
	}
	free(ptr);
	ptr = NULL;
}

/* arguemnt structure for passing args into threaded clock process */
struct p_args
{
	Step *seq[16]; //Step sequence to be played
	char *com[16]; //commands genereated to utilize the cox lib to play samples
	float steps;   //miliseconds between playing steps, this controls the tempo of the pattern
	int ch,		   //input char 
		i;		   //used to tell the display which Step the clock is on
};

/* frees p_arg memory */
static void destroy_p_args(struct p_args pa)
{
	for (int i = 0; i < 16; i++)
	{
		if (pa.com[i] != NULL)
		{
			free(pa.com[i]);
			pa.com[i] = NULL;
		}
	}
}

//prototype 
static void file_iterate(char name[128], DIR *dir, struct dirent *ent, int offset, const int max_cols);

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

/* create 16 step seq from .track file, refer to REAMME for track file syntax */
void importSequence(char *fn, Step *seq[16])
{
	FILE *fp = fopen(fn, "rb");
	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *str = malloc(fsize + 1);
	fread(str,fsize, 1, fp);
	fclose(fp);
	
	if (debug) printf("Alloced str mem, fsize = %d\n", fsize);

	int i = 0, c = 0, s = 0, j;
	char buf_name[128], buf_vol[16];

	memset(buf_name, '\0', 128);
	memset(buf_vol, '\0', 16);
	bool fn_flag = false,
		 vol_flag = false;
	for (j = 0; j < fsize; j++)
	{
		//if (debug) printf("in for loop, j = %d\n", j);
		if ((fn_flag || vol_flag) && (int)str[j] != 44 )
		{
			if (fn_flag)
				buf_name[c++] = str[j];
			else if (vol_flag)
				buf_vol[c++] = str[j];
		}
		if ((int)str[j] == 40) //open (
		{
			//if (debug) printf("Opened (\n");
			fn_flag = true;
		}
		else if ((int)str[j] == 44) // comma ,
		{
			//if (debug) printw("File: %s\n", buf);
			fn_flag = false;
			//seq[i]->sounds[s]->fname = (char*)malloc(sizeof(char)*128);
			//strcpy(seq[i].sounds[s].fname, buf);
			//memset(buf, '\0', 128);
			c = 0;
			vol_flag = true;
		}
		else if ((int)str[j] == 41) // close )
		{
			if (!fn_flag)
			{
				//if (debug) printw("vol: %f\n", atof(buf_vol));
				vol_flag = false;
				//seq[i]->sounds[s]->vol = atof(buf_vol);
				//memset(buf, '\0', 128);
				seq[i]->sounds[s] = initSample(buf_name, atof(buf_vol));
				c = 0;
				s++;
				memset(buf_name, '\0', 128);
				memset(buf_vol, '\0', 16);
			}
			else
			{
				fn_flag = false;
				vol_flag = false;
			}
			c = 0;
		}
		else if ((int)str[j] == 59) // new step ;
		{
			//if (debug) printf("new step ;\n");
			seq[i++]->sound_c = s;
			s = 0;
		}

		if (i > 15)
			return;
		

	}
}

/* play a single sample */	
void playSample(Sample *s, float *vol)
{
	char output[128];
	sprintf(output, "play -v %f %s -q &", *vol, s->fname); 
	system(output);
}

/* threaded function for playing a pattern */
void *playSequence(void *args)
{
	/* @param args : argument structure */
	struct p_args *pa = args;
	//printf("Test value, pa->ch = %d", pa->ch);
	pa->i = 0;
	while ((pa->ch) != 112)
	{
		//playStep(&(pa->seq[(pa->i)]));
		if (pa->com[pa->i] != NULL)
			system(pa->com[pa->i]);
		usleep((int)((pa->steps)));
		(pa->i) = ((pa->i) < 15) ? (pa->i)+1 : 0;
	}
}

/* generates sox commands for playing sounds on linux */
static char * gen_command(Step *st)
{
	if (st->sound_c == 0)
	{
		return NULL;
	}
	else if (st->sound_c == 1)
	{
		char *output = (char*)malloc(sizeof(char)*512);
		sprintf(output, "play -v %f %s -q &", st->sounds[0]->vol, st->sounds[0]->fname);
		//system(output);
		return output;
	}
	else
	{
		char *output = (char*)malloc(sizeof(char)*128*(st->sound_c)),
			 *single = (char*)malloc(sizeof(char)*128);
		strcpy(output, "play -V0 -m ");
		for (int i = 0; i < st->sound_c; i++)
		{
			sprintf(single, " -v %f %s ", st->sounds[i]->vol, st->sounds[i]->fname);
			//printw(" - %s -\n", st->sounds[i]);
			strcat(output, single);
		}
		strcat(output, " -q &");
		free(single);
		//printf("%s\n", output);
		//system(output);
		//endwin();
		//exit(0);
		return output;
	}
}

/* hanlde interuption calls */
void interupt(int dummy)
{
	clear();
	endwin();
	exit(0);
}

int main(int argc, char *argv[])
{
	//init ncurses 
	initscr();
	raw();

	int ch = 0;			//input char
	float tempo = 120;	//BPM tempo, 120 is default
	
	//more ncurses shite
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
			case 114: //r
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
			case 112: //p
				//printf("output test\n");
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
				while (pa.ch != 112)
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
				destroy_p_args(pa);
				nodelay(stdscr, FALSE);
				break;
			case 10: //enter
				if (d_iter == 2)
				{
					//printw("BROWSE PLS");
					fileBrowse(fbuf);
					strcpy(seq[select]->sounds[chan]->fname, fbuf);
					memset(fbuf, '\0', 512);
				}
				else if (d_iter == 8)
				{
					fileBrowse(fbuf);
					chan = seq[select]->sound_c++;
					//printf("incremented\n");
					seq[select]->sounds[chan] = initSample(fbuf, 1.0);
				}
				else if (d_iter == 9)
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
					destroySample(seq[select]->sounds[chan]);
					seq[select]->sound_c--;
					chan = 0;
				}
				else if (d_iter == 1 && empty)
				{
					chan = 0;
					seq[select]->sound_c++;
					//seq[select].sounds[chan].fname = (char*)malloc(sizeof(char)*128);
					fileBrowse(fbuf);
					seq[select]->sounds[chan] = initSample(fbuf, 1.0);
				}
				break;
		}
	}
	endwin();
}
