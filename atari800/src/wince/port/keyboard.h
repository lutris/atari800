/* (C) 2001  Vasyl Tsvirkunov */
#ifndef A800_KEYBOARD_H
#define A800_KEYBOARD_H

int prockb(void);
int initinput(void);
void uninitinput(void);
void clearkb(void);

void hitbutton(short);
void releasebutton(short);

void tapscreen(short x, short y);
void untapscreen(short x, short y);

extern int console;
extern int trig0;
extern int stick0;

int get_last_key();

/* 0 to 3 - overlay in landscape, 4 - not visible or portrait mode*/
extern int currentKeyboardMode;
/* 1 for negative image */
extern int currentKeyboardColor;

/* Packed bitmap 240x80 - five rows of buttons */
extern unsigned long* kbd_image;

#endif
