#include "sound.h"
#include "keyboard.h"

#include "SDL_mixer.h"
#include "SDL.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> 
#include <ncurses.h>

//typedef enum { false, true } bool;

bool debug = true;

/* reserve memory for sample */
Sample * initSample(char *fn)
{
	//if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 8, 4096) == -1)
	//	printw("SDL_error: %s\n", Mix_GetError());
	Sample *ptr = (Sample*)malloc(sizeof(Sample));
	ptr->fname = (char*)malloc(sizeof(char)*128);
	strcpy(ptr->fname, fn);
	//printf("%s\n", ptr->fname);
	ptr->chunk = Mix_LoadWAV(fn);
	if (ptr->chunk == NULL)
		destroySample(ptr);
	return ptr;
}

/* free memory from saple struct */
void destroySample(Sample *ptr)
{
	free(ptr->fname);
	free(ptr);
	ptr = NULL;
}

/* reserve memory for Step struct */
Step * initStep(float vol)
{
	Step *ptr = (Step*)malloc(sizeof(Step));
	ptr->vol = vol;
	return ptr;
}

/* free memory from Step struct */
void destroyStep(Step *ptr)
{
	if (ptr == NULL)
		return;
	free(ptr);
	ptr = NULL;
}

Channel * initChannel(Sample *bank[16], int bank_index)
{
	Channel *ptr;
	ptr = (Channel*)malloc(sizeof(Channel));
	ptr->sound = bank[bank_index];
	for (int i = 0; i < 16; i++)
	{
		ptr->pattern[i] = NULL;
	}
	return ptr;
}

void DestroyChannel(Channel *ptr)
{
	for (int i = 0; i < 16; i++)
	{
		destroyStep(ptr->pattern[i]);
	}
	free(ptr);
	ptr = NULL;
}

// void destroy_p_args(struct p_args pa)
// {
// 	for (int i = 0; i < 16; i++)
// 	{
// 		if (pa.com[i] != NULL)
// 		{
// 			free(pa.com[i]);
// 			pa.com[i] = NULL;
// 		}
// 	}
// }

void loadSampleBank(char *fn, Sample *bank[16])
{
	FILE *fp = fopen(fn, "rb");
	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *str = malloc(fsize + 1);
	fread(str,fsize, 1, fp);
	fclose(fp);
	
	char *name_buf = malloc(sizeof(char)*128);

	int j = 0, k = 0;
	for (int i = 0; i < fsize; i++)
	{
		if (str[i] != ';')
			name_buf[j++] = str[i];
		else
		{
			name_buf[j] = '\0';
			bank[k++] = initSample(name_buf);
			memset(name_buf, '\0', 128);
			j = 0;
		}

	}

	free(name_buf);
}

/* create 16 step seq from .track file, refer to REAMME for track file syntax */
void importSequence(char *fn, Channel *mix[16], Sample *bank[16])
{
	FILE *fp = fopen(fn, "rb");
	fseek(fp, 0, SEEK_END);
	long fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char *str = malloc(fsize + 1);
	fread(str,fsize, 1, fp);
	fclose(fp);
	
	if (debug) printw("Alloced str mem, fsize = %d\n", fsize);

	int i, 					//for indexing buffer
		j, 					//for indexing file
		chan, 				//which cannel are we on
		bank_index, 		//which sample from the sound bank do we want
		group_attrib = 0, 	//for detection which step attrib we're one
		seq_step = 0;		//which step in the channel are we on
	char buf_vol[16];
	bool new_line = true,
		 f_bank	  = false,
		 f_group  = false;

	memset(buf_vol, '\0', 16);
	for (j = 0; j < fsize; j++)
	{	
		//printw("%c", str[j]);getch();
		if (new_line)
		{
			if (str[j] == 'C')
			{
				chan = ((int)str[++j]) - 49;
				new_line = false;
				f_bank = true;
				i = 0;
				//printw("Char: %c\nSelected Channel: %d\n", str[j-1], chan);getch();
			}
		}
		else if (f_bank && str[j] == 'B')
		{
			bank_index = (int)str[++j] - 49;
			mix[chan] = initChannel(bank, bank_index);
			f_bank = false;
			//printw("Loaded bank file: %s\n", bank[bank_index]->fname);getch();
		}
		else if (!f_bank && !new_line && str[j] == '(')
		{
			if (str[j + 1] == ')')
			{
				mix[chan]->pattern[seq_step++] = NULL;
				printw("Found empty step\n");
				//j+=2;
			}
			else
				f_group = true;
		}
		else if (f_group)
		{
			if (str[j] == ',')
			{
				group_attrib++;
				i = 0;
			}
			else if (str[j] == ')')
			{
				f_group = false;
				group_attrib = 0;
				//printw("Assigning volume value: %f\nSeq_step = %d", atof(buf_vol), seq_step);getch();
				mix[chan]->pattern[seq_step++] = initStep(atof(buf_vol));
				//printw("Done\n");
				memset(buf_vol, '\0', 16);
				i = 0;
			}
			else
			{
				switch (group_attrib)
				{
					case 0:
						buf_vol[i++] = str[j];
						break;
				}
			}
		}
		else if (str[j] == ';')
		{
			
			i = 0;
			new_line = true;
			chan++;
			seq_step = 0;
		}

	}
}

int initSDL()
{
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		return -1;

	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1)
		return -1;

	Mix_AllocateChannels(16);
	return 0;
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
	while ((pa->ch) != PAUSE)
	{
		//playStep(&(pa->seq[(pa->i)]));
		// if (pa->mix[pa->i] != NULL)
		// 	system(pa->com[pa->i]);
		for (int i = 0; i < 16; i++)
		{
			if (pa->mix[i]->pattern[pa->i] != NULL)
				Mix_PlayChannel(i, pa->mix[i]->sound->chunk, 0);
		}
		usleep((int)((pa->steps)));
		(pa->i) = ((pa->i) < 15) ? (pa->i)+1 : 0;
	}
}