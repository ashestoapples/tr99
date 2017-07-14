#ifndef SOUND_H_INCLUDED
#define SOUND_H_INCLUDED

/* struct containing sample path and attributes */
typedef struct sample
{
	char *fname;
	float vol;
} Sample;

/* 16 channel sequence step */
typedef struct step
{
	Sample *sounds[16];
	int sound_c;
} Step;

/* arguemnt structure for passing args into threaded clock process */
struct p_args
{
	Step *seq[16]; //Step sequence to be played
	char *com[16]; //commands genereated to utilize the cox lib to play samples
	float steps;   //miliseconds between playing steps, this controls the tempo of the pattern
	int ch,		   //input char 
		i;		   //used to tell the display which Step the clock is on
};

/* reserve memory for sample */
Sample * initSample(char *fn, float v);


/* free memory from saple struct */
void destroySample(Sample *ptr);

/* reserve memory for Step struct */
Step * initStep();

/* free memory from Step struct */
void destroyStep(Step *ptr);

/* frees p_arg memory */
void destroy_p_args(struct p_args pa);

/* create 16 step seq from .track file, refer to REAMME for track file syntax */
void importSequence(char *fn, Step *seq[16]);

/* generates sox commands for playing sounds on linux */
char * gen_command(Step *st);

/* threaded function for playing a pattern */
void *playSequence(void *args);

/* play a single sample */
void playSample(Sample *s, float *vol);

#endif