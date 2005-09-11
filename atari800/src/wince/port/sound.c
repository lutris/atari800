/*
 * sound.c - WinCE port specific code
 *
 * Copyright (C) 2000 Krzysztof Nikiel
 * Copyright (C) 2000-2005 Atari800 development team (see DOC/CREDITS)
 *
 * This file is part of the Atari800 emulator project which emulates
 * the Atari 400, 800, 800XL, 130XE, and 5200 8-bit computers.
 *
 * Atari800 is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Atari800 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Atari800; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "config.h"

#ifdef SOUND

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "sound.h"
#include "main.h"
#include "pokeysnd.h"
#include "atari.h"
#include "log.h"

#define WAVSHIFT	9
#define WAVSIZE		(1 << WAVSHIFT)
int buffers = 0;

enum {SOUND_NONE, SOUND_WAV};

static int issound = SOUND_NONE;
static int dsprate = 22050;
static int snddelay = 40;	/* delay in milliseconds */
static int snddelaywav = 100;
#ifdef STEREO_SOUND
static int stereo = TRUE;
#else
static int stereo = FALSE;
#endif

static HANDLE event;
static HWAVEOUT wout;
static WAVEHDR *waves;


static void uninitsound_wav(void)
{
  int i;
  MMRESULT err;

  if (issound != SOUND_WAV)
    return;

l0:
  for (i = 0; i < buffers; i++)
    {
      if (!(waves[i].dwFlags & WHDR_DONE))
	{
	  WaitForSingleObject(event, 5000);
	  ResetEvent(event);
	  goto l0;
	}
    }

  waveOutReset (wout);

  for (i = 0; i < buffers; i++)
    {
      err = waveOutUnprepareHeader(wout, &waves[i], sizeof (waves[i]));
      if (err != MMSYSERR_NOERROR)
	{
	  fprintf(stderr, "warning: cannot unprepare wave header (%x)\n", err);
	}
      free(waves[i].lpData);
    }
  free(waves);

  waveOutClose(wout);
  CloseHandle(event);

  issound = SOUND_NONE;
}

static int initsound_wav(void)
{
  int i;
  WAVEFORMATEX wf;
  MMRESULT err;

  event = CreateEvent(NULL, TRUE, FALSE, NULL);

  memset(&wf, 0, sizeof(wf));

  wf.wFormatTag = WAVE_FORMAT_PCM;
  wf.nChannels = stereo ? 2 : 1;
  wf.nSamplesPerSec = dsprate;
  wf.nAvgBytesPerSec = dsprate * wf.nChannels;
  wf.nBlockAlign = 2;
  wf.wBitsPerSample = 8;
  wf.cbSize = 0;

  err = waveOutOpen(&wout, WAVE_MAPPER, &wf, (int)event, 0, CALLBACK_EVENT);
  if (err == WAVERR_BADFORMAT)
    {
      Aprint("wave output paremeters unsupported\n");
      exit(1);
    }
  if (err != MMSYSERR_NOERROR)
    {
      Aprint("cannot open wave output (%x)\n", err);
      exit(1);
    }

  buffers = ((wf.nAvgBytesPerSec * snddelaywav / 1000) >> WAVSHIFT) + 1;
  waves = malloc(buffers * sizeof(*waves));
  for (i = 0; i < buffers; i++)
    {
      memset(&waves[i], 0, sizeof (waves[i]));
      if (!(waves[i].lpData = (uint8 *)malloc(WAVSIZE)))
	{
	  Aprint("could not get wave buffer memory\n");
	  exit(1);
	}
      waves[i].dwBufferLength = WAVSIZE;
      err = waveOutPrepareHeader(wout, &waves[i], sizeof(waves[i]));
      if (err != MMSYSERR_NOERROR)
	{
	  Aprint("cannot prepare wave header (%x)\n", err);
	  exit(1);
	}
      waves[i].dwFlags |= WHDR_DONE;
    }

  Pokey_sound_init(FREQ_17_EXACT,(uint16) dsprate, 1, 0);

  issound = SOUND_WAV;
  return 0;
}

void Sound_Initialise(int *argc, char *argv[])
{
  int i, j;
  int usesound = TRUE;

  if (issound != SOUND_NONE)
    return;

  for (i = j = 1; i < *argc; i++)
    {
      if (strcmp(argv[i], "-sound") == 0)
	usesound = TRUE;
      else if (strcmp(argv[i], "-nosound") == 0)
	usesound = FALSE;
      else if (strcmp(argv[i], "-dsprate") == 0)
	sscanf(argv[++i], "%d", &dsprate);
      else if (strcmp(argv[i], "-snddelay") == 0)
      {
	sscanf(argv[++i], "%d", &snddelay);
        snddelaywav = snddelay;
      }
      else
      {
	if (strcmp(argv[i], "-help") == 0)
	{
	  Aprint("\t-sound			enable sound\n"
		 "\t-nosound			disable sound\n"
		 "\t-dsprate <rate>		set dsp rate\n"
		 "\t-snddelay <milliseconds>	set sound delay\n"
		);
	}
	argv[j++] = argv[i];
      }
    }
  *argc = j;

  if (!usesound)
    return;

  initsound_wav();
  return;
}

void Sound_Exit(void)
{
  if (issound == SOUND_WAV)
    uninitsound_wav();

  issound = SOUND_NONE;
}

static WAVEHDR *getwave(void)
{
  int i;

  for (i = 0; i < buffers; i++)
    {
      if (waves[i].dwFlags & WHDR_DONE)
	{
	  waves[i].dwFlags &= ~WHDR_DONE;
	  return &waves[i];
	}
    }

  return NULL;
}

void Sound_Update(void)
{
  MMRESULT err;
  WAVEHDR *wh;

  if (issound == SOUND_WAV)
  {
    while ((wh = getwave()))
    {
      Pokey_process(wh->lpData, wh->dwBufferLength);
      err = waveOutWrite(wout, wh, sizeof(*wh));
      if (err != MMSYSERR_NOERROR)
      {
	Aprint("cannot write wave output (%x)\n", err);
	return;
      }
    }
  }
}

void Sound_Pause(void)
{
}

void Sound_Continue(void)
{
}

#endif	/* SOUND */


/*
$Log$
Revision 1.5  2003/03/03 09:57:33  joy
Ed improved autoconf again plus fixed some little things

Revision 1.4  2003/02/24 09:33:41  joy
header cleanup

Revision 1.3  2003/02/09 21:24:13  joy
updated for different number of Pokey_sound_init parameters

Revision 1.2  2002/01/22 00:17:21  vasyl
Updating WinCE port. A lot of changes, the most important are:
fixed more keyboard bugs;
support for monochrome and paletted devices;
speedups in graphics code;
port-specific UI driver (pass-through).

Revision 1.6  2001/12/04 13:09:06  joy
DirectSound buffer creation error fixed by Nathan

Revision 1.5  2001/07/22 06:46:08  knik
waveout default sound delay 100ms, buffer size 0x200

Revision 1.4  2001/04/17 05:32:33  knik
sound_continue moved outside dx conditional

Revision 1.3  2001/04/08 05:50:16  knik
standard wave output driver added; sound and directx conditional compile

Revision 1.2  2001/03/24 10:13:43  knik
-nosound option fixed

Revision 1.1  2001/03/18 07:56:48  knik
win32 port

*/
