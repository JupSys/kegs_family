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

const char rcsid_sound_hp_c[] = "@(#)$Header: sound_dummy.c,v 1.2 98/05/25 18:40:55 kentd Exp $";

#include "defc.h"
#include "sound.h"

extern int errno;
extern int Verbose;
extern int g_use_alib;


int	g_audio_socket = -1;
int	g_bytes_written = 0;

#define ZERO_BUF_SIZE		2048

word32 zero_buf[ZERO_BUF_SIZE];

#define ZERO_PAUSE_SAFETY_SAMPS		(AUDIO_RATE/50)
#define ZERO_PAUSE_NUM_SAMPS		(5*AUDIO_RATE)


void
reliable_buf_write(word32 *shm_addr, int pos, int size)
{
	byte	*ptr;
	int	ret;


	if(size < 1 || pos < 0 || pos > SOUND_SHM_SAMP_SIZE ||
				size > SOUND_SHM_SAMP_SIZE ||
				(pos + size) > SOUND_SHM_SAMP_SIZE) {
		printf("reliable_buf_write: pos: %04x, size: %04x\n",
			pos, size);
		exit(1);
	}

	ptr = (byte *)&(shm_addr[pos]);
	size = size * 4;

	if(g_audio_socket < 0) {
		return;
	}

	while(size > 0) {
		ret = write(g_audio_socket, ptr, size);

		if(ret < 0) {
			printf("audio write, errno: %d\n", errno);
			exit(1);
		}
		size = size - ret;
		ptr += ret;
		g_bytes_written += ret;
	}

}

void
reliable_zero_write(int amt)
{
	int	len;

	while(amt > 0) {
		len = MIN(amt, ZERO_BUF_SIZE);
		reliable_buf_write(zero_buf, 0, len);
		amt -= len;
	}
}


void
child_sound_loop(int read_fd, word32 *shm_addr)
{
	word32	tmp;
	long	status_return;
	int	zeroes_buffered;
	int	zeroes_seen;
	int	sound_paused;
	int	vbl;
	int	size;
	int	pos;
	int	ret;

	doc_printf("Child pipe fd: %d\n", read_fd);

	/* do any one-time audio initialization here */

	zeroes_buffered = 0;
	zeroes_seen = 0;
	sound_paused = 0;

	pos = 0;
	vbl = 0;
	while(1) {
		errno = 0;

		ret = read(read_fd, &tmp, 4);

		if(ret <= 0) {
			printf("child dying from ret: %d, errno: %d\n",
				ret, errno);
			break;
		}

		size = tmp & 0xffffff;

		if((tmp >> 24) == 0xa2) {
			/* play sound here */

			if(sound_paused) {
				printf("Unpausing sound, zb: %d\n",
					zeroes_buffered);
				sound_paused = 0;
			}


#if 0
			pos += zeroes_buffered;
			while(pos >= SOUND_SHM_SAMP_SIZE) {
				pos -= SOUND_SHM_SAMP_SIZE;
			}
#endif

			if(zeroes_buffered) {
				reliable_zero_write(zeroes_buffered);
			}

			zeroes_buffered = 0;
			zeroes_seen = 0;


			if((size + pos) > SOUND_SHM_SAMP_SIZE) {
				reliable_buf_write(shm_addr, pos,
						SOUND_SHM_SAMP_SIZE - pos);
				size = (pos + size) - SOUND_SHM_SAMP_SIZE;
				pos = 0;
			}

			reliable_buf_write(shm_addr, pos, size);


		} else if((tmp >> 24) == 0xa1) {
			if(sound_paused) {
				if(zeroes_buffered < ZERO_PAUSE_SAFETY_SAMPS) {
					zeroes_buffered += size;
				}
			} else {
				/* not paused, send it through */
				zeroes_seen += size;

				reliable_zero_write(size);

				if(zeroes_seen >= ZERO_PAUSE_NUM_SAMPS) {
					printf("Pausing sound\n");
					sound_paused = 1;
				}
			}
		} else {
			printf("tmp received bad: %08x\n", tmp);
			exit(3);
		}

		pos = pos + size;
		while(pos >= SOUND_SHM_SAMP_SIZE) {
			pos -= SOUND_SHM_SAMP_SIZE;
		}

		vbl++;
		if(vbl >= 60) {
			vbl = 0;
#if 0
			printf("sound bytes written: %06x\n", g_bytes_written);
			printf("Sample samples[0]: %08x %08x %08x %08x\n",
				shm_addr[0], shm_addr[1],
				shm_addr[2], shm_addr[3]);
			printf("Sample samples[100]: %08x %08x %08x %08x\n",
				shm_addr[100], shm_addr[101],
				shm_addr[102], shm_addr[103]);
#endif
			g_bytes_written = 0;
		}
	}

	/* Do whatever audio shutdown you need here, like */
	/*  ioctls to drain audio, etc. */

	exit(0);
}

