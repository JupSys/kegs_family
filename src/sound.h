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
const char rcsid_sound_h[] = "@(#)$Header: sound.h,v 1.7 99/10/31 01:13:38 kentd Exp $";
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

#define SOUND_SHM_SAMP_SIZE		(8*1024)

#define SAMPLE_SIZE		2
#define NUM_CHANNELS		2
#define SAMPLE_CHAN_SIZE	(SAMPLE_SIZE * NUM_CHANNELS)

STRUCT(Doc_reg) {
	double	dsamp_ev;
	double	dsamp_ev2;
	double	complete_dsamp;
	int	samps_left;
	word32	cur_acc;
	word32	cur_inc;
	word32	cur_start;
	word32	cur_end;
	word32	cur_mask;
	int	size_bytes;
	int	event;
	int	running;
	int	has_irq_pending;
	word32	freq;
	word32	vol;
	word32	waveptr;
	word32	ctl;
	word32	wavesize;
	word32	last_samp_val;
};

