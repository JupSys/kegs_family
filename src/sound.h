/****************************************************************/
/*			Apple IIgs emulator			*/
/*			Copyright 1996 Kent Dickey		*/
/*								*/
/*	This code may not be used in a commercial product	*/
/*	without prior written permission of the author.		*/
/*								*/
/*	You may freely distribute this code.			*/ 
/*								*/
/*	You can contact the author at kentd@cup.hp.com.		*/
/*	HP has nothing to do with this software.		*/
/****************************************************************/

#ifdef INCLUDE_RCSID_C
const char rcsid_sound_h[] = "@(#)$Header: sound.h,v 1.4 97/04/16 00:38:07 kentd Exp $";
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

#define SOUND_SHM_SAMP_SIZE		(8*1024)

#define AUDIO_RATE		48000

#define SAMPLE_SIZE		2
#define NUM_CHANNELS		2
#define SAMPLE_CHAN_SIZE	(SAMPLE_SIZE * NUM_CHANNELS)

STRUCT(Sound_control) {
	word32	total_samps;
};
