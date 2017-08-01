#include <stdio.h>

#include "keyboard.h"
#include "sound.h"
#include "tui.h"


int main(int argc, char **argv)
{
	FILE *fp = fopen(LOG_FILE, "w");
	mainMenu();
	fclose(fp);

	return 0;
}
