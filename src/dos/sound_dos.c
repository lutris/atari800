/*
 * sound_dos.c - high level sound routines for DOS port
 *
 * Copyright (c) 1998-2000 Matthew Conte
 * Copyright (c) 2000-2005 Atari800 development team (see DOC/CREDITS)
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"
#include <string.h>		/* for strcmp */

#ifdef SOUND

#include "atari.h"
#include "log.h"
#include "pokeysnd.h"
#include "util.h"
#include "sound.h"

#include "dos_sb.h"

static int sound_enabled = TRUE;

static int playback_freq = POKEYSND_FREQ_17_APPROX / 28 / 3;
#ifdef STEREO_SOUND
static int buffersize = 880;
static int stereo = TRUE;
#else
static int buffersize = 440;
static int stereo = FALSE;
#endif
static int bps = 8;

int Sound_Initialise(int *argc, char *argv[])
{
	int i, j;
	int help_only = FALSE;

	for (i = j = 1; i < *argc; i++) {
		int i_a = (i + 1 < *argc); /* is argument available? */
		int a_m = FALSE; /* error, argument missing! */

		if (strcmp(argv[i], "-sound") == 0)
			sound_enabled = TRUE;
		else if (strcmp(argv[i], "-nosound") == 0)
			sound_enabled = FALSE;
		else if (strcmp(argv[i], "-dsprate") == 0) {
			if (i_a)
				playback_freq = Util_sscandec(argv[++i]);
			else a_m = TRUE;
		}
		else if (strcmp(argv[i], "-bufsize") == 0) {
			if (i_a)
				buffersize = Util_sscandec(argv[++i]);
			else a_m = TRUE;
		}
		else {
			if (strcmp(argv[i], "-help") == 0) {
				help_only = TRUE;
				Log_print("\t-sound           Enable sound");
				Log_print("\t-nosound         Disable sound");
				Log_print("\t-dsprate <freq>  Set mixing frequency (Hz)");
				Log_print("\t-bufsize <size>  Set sound buffer size");
			}
			argv[j++] = argv[i];
		}

		if (a_m) {
			Log_print("Missing argument for '%s'", argv[i]);
			sound_enabled = FALSE;
			return FALSE;
		}
	}

	*argc = j;

	if (help_only) {
		sound_enabled = FALSE;
		return TRUE;
	}

	if (sound_enabled) {
		if (sb_init(&playback_freq, &bps, &buffersize, &stereo) < 0) {
			Log_print("Cannot init sound card");
			sound_enabled = FALSE;
		}
		else {
			POKEYSND_Init(POKEYSND_FREQ_17_APPROX, playback_freq, stereo ? 2 : 1, 0);
			sb_startoutput((sbmix_t) POKEYSND_Process);
		}
	}

	return TRUE;
}

void Sound_Pause(void)
{
	if (sound_enabled)
		sb_stopoutput();
}

void Sound_Continue(void)
{
	if (sound_enabled)
		sb_startoutput((sbmix_t) POKEYSND_Process);
}

void Sound_Update(void)
{
}

void Sound_Exit(void)
{
	if (sound_enabled)
		sb_shutdown();
}

#endif /* SOUND */
