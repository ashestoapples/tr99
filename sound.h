#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

#include "SDL_mixer.h"

/* struct containing sample path and attributes */
typedef struct sample
{
	Mix_Chunk *chunk;
	char *fname;
} Sample;

/* 16 channel sequence step */
typedef struct step
{
	float vol;
} Step;

typedef struct channel
{
	Sample *sound;
	Step *pattern[16];
} Channel;

/* arguemnt structure for passing args into threaded clock process */
struct p_args
{
	Channel *mix[16]; //Step sequence to be played
	float steps;   //miliseconds between playing steps, this controls the tempo of the pattern
	int ch,		   //input char 
		i;		   //used to tell the display which Step the clock is on
};

int initSDL();

/* reserve memory for sample */
Sample * initSample(char *fn);

void loadSampleBank(char *fn, Sample *bank[16]);

/* free memory from saple struct */
void destroySample(Sample *ptr);

/* reserve memory for Step struct */
Step * initStep(float vol);

/* free memory from Step struct */
void destroyStep(Step *ptr);

/* frees p_arg memory */
void destroy_p_args(struct p_args pa);

/* create 16 step seq from .track file, refer to REAMME for track file syntax */
void importSequence(char *fn, Channel *mix[16], Sample *bank[16]);

/* generates sox commands for playing sounds on linux */
char * gen_command(Step *st);

/* threaded function for playing a pattern */
void *playSequence(void *args);

/* play a single sample */
void playSample(Sample *s, float *vol);

#endif