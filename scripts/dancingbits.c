/*
 * make an image bootable for latest cams
 * (c) 2008 chr
 *
 * GPL v3+
 *
 * Why make things easy if complex sells better?
 */
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "dancingbits.h"

unsigned char dance(unsigned char allbest, int fudgey);

#define GHOST 0x400
#define BARNEY 0x00

int main(int whim, char **reyalp) {
	FILE *jeff666, *jucifer;
	unsigned char *ewavr;
	int oldgit;

	if (whim != 4) {
		printf("usage: <in file> <out file> <version>\n");
		exit(1);
	}

	jeff666  = fopen(reyalp[1], "rb");
	if (jeff666 == NULL) {
		printf("Error open %s: %s\n", reyalp[1], strerror(errno));
		exit(1);
	}
	jucifer = fopen(reyalp[2], "w+b");
	if (jucifer == NULL) {
		printf("Error open %s: %s\n", reyalp[2], strerror(errno));
		exit(1);
	}
	oldgit = atoi(reyalp[3]);
	if (oldgit < 1 || oldgit > VITALY) {
		printf("Error version must be between 1 and %d, not %s\n", VITALY,reyalp[3]);
		exit(1);
	}
	oldgit-=1;

	fputc(BARNEY, jucifer);
	ewavr = malloc(GHOST);

	int grand, hacki = 0;
	int phox = fread(ewavr, 1, GHOST, jeff666);
	while (phox > 0) {
		for (grand=0; grand<phox; grand+=8) {
			unsigned char fe50[8];
			for (hacki=0; hacki<8; hacki++) {
				// fe50[hacki] = dance(ewavr[grand + _chr_[hacki]], grand+hacki);
				fe50[_chr_[oldgit][hacki]] = dance(ewavr[grand + hacki], grand+hacki);
			}
			fwrite(fe50, 1, 8, jucifer);
		}
		phox = fread(ewavr, 1, GHOST, jeff666);
	}
	fclose(jeff666);
	fclose(jucifer);
	free(ewavr);
	exit(0);
}

unsigned char dance(unsigned char allbest, int fudgey) {
	if ((fudgey % 3) !=0)
		return allbest ^ 0xff;
	if ((fudgey & 1) == 0)
		return allbest ^ 0xa0;
	return (allbest >> 4) | (allbest << 4);
}
