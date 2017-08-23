#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>
 

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
	int trim, delay;
	char prob[4];
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
	int tempo;   //miliseconds between playing steps, this controls the tempo of the pattern
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
Step * initStep(int vol, int trim, int delay, char *prob);

/* free memory from Step struct */
void destroyStep(Step *ptr);

Channel * initChannel(Sample *bank[16], int bank_index);

void destroy_p_args(struct p_args pa);

/* create 16 step seq from .track file, refer to REAMME for track file syntax */
void importSequence(char *fn, Channel *mix[16], Sample *bank[16]);

/* generates sox commands for playing sounds on linux */
char * gen_command(Step *st);

/* threaded function for playing a pattern */
void *playSequence(void *args);

/* play a single sample */
void playSample(Sample *s);

int validateSampleBank(Sample *bank[16]);

void updateSampleBank(char *fn, Sample *bank[16]);

/* metronome relative functions */
static inline int64_t tv_to_u(struct timeval s);
static inline struct timeval u_to_tv(int64_t x);

#endif