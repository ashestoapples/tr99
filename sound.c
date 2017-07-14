#include "sound.h"
#include "keyboard.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h> 

typedef enum { false, true } bool;

bool debug = true;

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

void destroy_p_args(struct p_args pa)
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
	while ((pa->ch) != PAUSE)
	{
		//playStep(&(pa->seq[(pa->i)]));
		if (pa->com[pa->i] != NULL)
			system(pa->com[pa->i]);
		usleep((int)((pa->steps)));
		(pa->i) = ((pa->i) < 15) ? (pa->i)+1 : 0;
	}
}

/* generates sox commands for playing sounds on linux */
char * gen_command(Step *st)
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