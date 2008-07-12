#ifndef PBI_PROTO80_H_
#define PBI_PROTO80_H_

#include "atari.h"
void PBI_PROTO80_Initialise(int *argc, char *argv[]);
int PBI_PROTO80_ReadConfig(char *string, char *ptr);
void PBI_PROTO80_WriteConfig(FILE *fp);
int PBI_PROTO80_D1GetByte(UWORD addr);
void PBI_PROTO80_D1PutByte(UWORD addr, UBYTE byte);
int PBI_PROTO80_D1ffPutByte(UBYTE byte);
UBYTE PBI_PROTO80_GetPixels(int scanline, int column);
extern int PBI_PROTO80_enabled;

#endif /* PBI_PROTO80_H_ */
