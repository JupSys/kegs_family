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

const char rcsid_sound_c[] = "@(#)$Header: sound.c,v 1.60 97/07/07 23:09:16 kentd Exp $";

#include "defc.h"

#define INCLUDE_RCSID_C
#include "sound.h"
#undef INCLUDE_RCSID_C

# define DO_DOC_LOG
#if 0
#endif

extern int stepping;
extern int Verbose;
extern word32 g_vbl_count;


extern double g_dcycles_in_16ms;
extern double g_last_vbl_dcycs;

extern int errno;

void U_STACK_TRACE();

byte doc_ram[0x10000 + 16];

word32 doc_sound_ctl = 0;
word32 doc_saved_val = 0;
word32 doc_ptr = 0;
int	g_doc_num_osc_en = 1;
double	g_dcycs_per_doc_update = 1.0;
double	g_dupd_per_dcyc = 1.0;
double	g_drecip_osc_en_plus_2 = 1.0 / (double)(1 + 2);

int	g_doc_saved_ctl = 0;
int	g_queued_samps = 0;
int	g_queued_nonsamps = 0;


Doc_reg g_doc_regs[32];

word32 doc_reg_e0 = 0xff;

/* local function prototypes */
void doc_write_ctl_reg(int osc, int val, double dcycs, int samps_to_do);


const int	g_audio_rate = AUDIO_RATE;
const double	g_daudio_rate = (AUDIO_RATE)*1.0;
const double	g_dsamps_per_dcyc = ((AUDIO_RATE*1.0) / DCYCS_1_MHZ);
const double	g_dcycs_per_samp = (DCYCS_1_MHZ / (AUDIO_RATE*1.0));
const float	g_fsamps_per_dcyc = (float)((AUDIO_RATE*1.0) / DCYCS_1_MHZ);

int	g_doc_vol = 2;

#define MAX_C030_TIMES		4096

double g_last_sound_play_dsamp = 0.0;

float c030_fsamps[MAX_C030_TIMES + 1];
int g_num_c030_fsamps = 0;

#define DOC_SCAN_RATE	(DCYCS_28_MHZ/32.0)

int	g_pipe_fd[2];
word32	*g_sound_shm_addr = 0;
int	g_sound_shm_pos = 0;

#define LEN_DOC_LOG	256

STRUCT(Doc_log) {
	char	*msg;
	int	osc;
	double	dcycs;
	double	dtmp2;
	int	etc;
	Doc_reg	doc_reg;
};

Doc_log g_doc_log[LEN_DOC_LOG];
int	g_doc_log_pos = 0;


#ifdef DO_DOC_LOG
# define DOC_LOG(a,b,c,d)	doc_log_rout(a,b,c,d)
#else
# define DOC_LOG(a,b,c,d)
#endif

#define UPDATE_G_DCYCS_PER_DOC_UPDATE(osc_en)				\
	g_dcycs_per_doc_update = (double)((osc_en + 2) * DCYCS_1_MHZ) /	\
		DOC_SCAN_RATE;						\
	g_dupd_per_dcyc = 1.0 / g_dcycs_per_doc_update;			\
	g_drecip_osc_en_plus_2 = 1.0 / (double)(osc_en + 2);

#define SND_PTR_SHIFT		14
#define SND_PTR_SHIFT_DBL	((double)(1 << SND_PTR_SHIFT))

void
doc_log_rout(char *msg, int osc, double dcycs, int etc)
{
	int	pos;

	pos = g_doc_log_pos;
	g_doc_log[pos].msg = msg;
	g_doc_log[pos].osc = osc;
	g_doc_log[pos].dcycs = dcycs;
	g_doc_log[pos].dtmp2 = g_last_sound_play_dsamp;
	g_doc_log[pos].etc = etc;
	if(osc >= 0 && osc < 32) {
		g_doc_log[pos].doc_reg = g_doc_regs[osc];
	}
	pos++;
	if(pos >= LEN_DOC_LOG) {
		pos = 0;
	}

	doc_printf("log: %s, osc:%d dsamp:%f, etc:%d\n", msg, osc, dcycs, etc);

	g_doc_log_pos = pos;
}

extern double g_cur_dcycs;

void
show_doc_log(void)
{
	Doc_reg	*rptr;
	int	pos;
	int	i;

	pos = g_doc_log_pos;
	printf("DOC log pos: %d\n", pos);
	for(i = 0; i < LEN_DOC_LOG; i++) {
		rptr = &(g_doc_log[pos].doc_reg);
		printf("%03x:%03x: %s dc:%11.1f dt2:%10.1f etc:%4d o:%2d "
			"c:%02x fq:%04x\n",
			i, pos, g_doc_log[pos].msg,
			g_doc_log[pos].dcycs, g_doc_log[pos].dtmp2,
			g_doc_log[pos].etc, g_doc_log[pos].osc, rptr->ctl,
			rptr->freq);
		printf("             ire:%d,%d,%d ptr:%08x inc:%08x "
			"samp_do:%d samp_left:%d\n",
			rptr->has_irq_pending, rptr->running, rptr->event,
			rptr->cur_ptr, rptr->cur_inc, rptr->samps_to_do,
			rptr->samps_left);
		pos++;
		if(pos >= LEN_DOC_LOG) {
			pos = 0;
		}
	}

	printf("cur_dcycs: %f\n", g_cur_dcycs);

}

void
sound_init()
{
	Doc_reg	*rptr;
	word32	shmaddr;
	int	shmid;
	int	ret;
	int	pid;
	int	i;

	shmid = shmget(IPC_PRIVATE, SOUND_SHM_SAMP_SIZE * SAMPLE_CHAN_SIZE,
							IPC_CREAT | 0777);
	if(shmid < 0) {
		printf("sound_init: shmget ret: %d, errno: %d\n", shmid,
			errno);
		exit(2);
	}

	shmaddr = (word32)shmat(shmid, 0, 0);
	if(shmaddr == -1) {
		printf("sound_init: shmat ret: %08x, errno: %d\n", shmaddr,
			errno);
		exit(3);
	}

	ret = shmctl(shmid, IPC_RMID, 0);
	if(ret < 0) {
		printf("sound_init: shmctl ret: %d, errno: %d\n", ret,
			errno);
		exit(4);
	}

	g_sound_shm_addr = (word32 *)shmaddr;

	/* prepare pipe so child/parent can signal each other */
	ret = pipe(&g_pipe_fd[0]);
	if(ret < 0) {
		printf("sound_init: pipe ret: %d, errno: %d\n", ret, errno);
		exit(5);
	}

	pid = fork();
	switch(pid) {
	case 0:
		/* child */
		/* close stdin and write-side of pipe */
		close(0);
		close(g_pipe_fd[1]);
		child_sound_loop(g_pipe_fd[0], g_sound_shm_addr);
		exit(0);
	case -1:
		/* error */
		printf("sound_init: fork ret: -1, errno: %d\n", errno);
		exit(6);
	default:
		/* parent */
		/* close read-side of pipe */
		close(g_pipe_fd[0]);
		doc_printf("Child is pid: %d\n", pid);
	}

	for(i = 0; i < 32; i++) {
		rptr = &(g_doc_regs[i]);
		rptr->dcyc_ev = 0.0;
		rptr->dcyc_ev2 = 0.0;
		rptr->dcyc_ev3 = 0.0;
		rptr->dcyc_ev4 = 0.0;
		rptr->samps_to_do = 0;
		rptr->samps_left = 0;
		rptr->cur_ptr = 0;
		rptr->cur_inc = 0;
		rptr->cur_start = 0;
		rptr->cur_end = 0;
		rptr->size_bytes = 0;
		rptr->event = 0;
		rptr->running = 0;
		rptr->has_irq_pending = 0;
		rptr->freq = 0;
		rptr->vol = 0;
		rptr->waveptr = 0;
		rptr->ctl = 1;
		rptr->wavesize = 0;
        }
}


void
sound_reset(double dcycs)
{
	int	i;

	for(i = 0; i < 32; i++) {
		doc_write_ctl_reg(i, g_doc_regs[i].ctl | 1, dcycs, 0);
		doc_reg_e0 = 0xff;
		if(g_doc_regs[i].has_irq_pending) {
			printf("reset: has_irq[%02x] = %d\n", i,
				g_doc_regs[i].has_irq_pending);
			set_halt(1);
		}
	}

	g_doc_num_osc_en = 1;
	UPDATE_G_DCYCS_PER_DOC_UPDATE(1);
}

void
sound_shutdown()
{
	if(g_pipe_fd[1] != 0) {
		close(g_pipe_fd[1]);
	}
}



void
sound_update(double dcycs)
{

	/* first, finish "playing" sounds for this vbl */

	sound_play(dcycs);


#if 0
	/* Then, find new sound events in the next vbl */
	scan_osc_for_events(-1, dcycs, g_last_vbl_dcycs + g_dcycles_in_16ms);
#endif
}

#define MAX_SND_BUF	65536

int samp_buf[2*MAX_SND_BUF];
word32 zero_buf[SOUND_SHM_SAMP_SIZE];

double g_doc_dsamps_extra = 0.0;

float	g_fvoices = 0.0;

word32 g_cycs_in_sound1 = 0;
word32 g_cycs_in_sound2 = 0;
word32 g_cycs_in_sound3 = 0;
word32 g_cycs_in_sound4 = 0;
word32 g_cycs_in_start_sound = 0;
word32 g_cycs_in_est_sound = 0;

int	g_num_snd_plays = 0;
int	g_num_doc_events = 0;
int	g_num_start_sounds = 0;
int	g_num_scan_osc = 0;
int	g_num_recalc_snd_parms = 0;

word32	g_last_c030_vbl_count = 0;
int	g_c030_state = 0;

#define VAL_C030_RANGE		(32768)
#define VAL_C030_BASE		(-16384)

int	g_sound_file_num = 0;
int	g_sound_file_fd = -1;
int	g_send_sound_to_file = 0;
int	g_send_file_bytes = 0;

void
open_sound_file()
{
	char	name[256];
	int	fd;

	sprintf(name, "snd.out.%d", g_sound_file_num);

	fd = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0x1ff);
	if(fd < 0) {
		printf("open_sound_file open ret: %d, errno: %d\n", fd, errno);
		exit(1);
	}

	g_sound_file_fd = fd;
	g_sound_file_num++;
	g_send_file_bytes = 0;
}

void
close_sound_file()
{
	if(g_sound_file_fd >= 0) {
		close(g_sound_file_fd);
	}

	g_sound_file_fd = -1;
}

void
check_for_range(word32 *addr, int num_samps, int offset)
{
	short	*shortptr;
	int	i;
	int	left;
	int	right;
	int	max;

	max = -32768;

	if(num_samps > SOUND_SHM_SAMP_SIZE) {
		printf("num_samps: %d > %d!\n", num_samps, SOUND_SHM_SAMP_SIZE);
		set_halt(1);
	}

	for(i = 0; i < num_samps; i++) {
		shortptr = (short *)&(addr[i]);
		left = shortptr[0];
		right = shortptr[1];
		if((left > 0x3000) || (right > 0x3000)) {
			printf("Sample %d of %d at snd_buf: %08x is: %d/%d\n",
				i + offset, num_samps, (word32)(addr+i),
				left, right);
			set_halt(1);
			return;
		}

		max = MAX(max, left);
		max = MAX(max, right);
	}

	printf("check4 max: %d over %d\n", max, num_samps);
}

void
send_sound_to_file(word32 *addr, int cur_pos, int num_samps)
{
	int	size;
	int	ret;

	if(g_sound_file_fd < 0) {
		open_sound_file();
	}

	size = 0;
	if((num_samps + cur_pos) > SOUND_SHM_SAMP_SIZE) {
		size = SOUND_SHM_SAMP_SIZE - cur_pos;
		g_send_file_bytes += (size * 4);

		ret = write(g_sound_file_fd, &(addr[cur_pos]), 4*size);
		if(ret != 4*size) {
			printf("wrote %d not %d\n", ret, 4*size);
			set_halt(1);
		}

		if(g_doc_vol < 3) {
			check_for_range(&(addr[cur_pos]), size, 0);
		} else {
			printf("Not checking %d bytes since vol: %d\n",
				4*size, g_doc_vol);
		}
		cur_pos = 0;
		num_samps -= size;
	}

	g_send_file_bytes += (num_samps * 4);

	ret = write(g_sound_file_fd, &(addr[cur_pos]), 4*num_samps);
	if(ret != 4*num_samps) {
		printf("wrote %d not %d\n", ret, 4*num_samps);
		set_halt(1);
	}

	if(g_doc_vol < 3) {
		check_for_range(&(addr[cur_pos]), num_samps, size);
	} else {
		printf("Not checking2 %d bytes since vol: %d\n",
			4*num_samps, g_doc_vol);
	}

}

void
send_sound(int fd, int real_samps, int size)
{
	word32	tmp;
	int	ret;

	if(real_samps) {
		tmp = size + 0xa2000000;
	} else {
		tmp = size + 0xa1000000;
	}

	ret = write(fd, &tmp, 4);
	if(ret != 4) {
		printf("send_sound, write ret: %d, errno: %d\n", ret, errno);
		set_halt(1);
	}
}

void
show_c030_state()
{
	show_c030_samps(&(samp_buf[0]), 100);
}

void
show_c030_samps(int *outptr, int num)
{
	int	i;

	printf("c030_fsamps[]: %d\n", g_num_c030_fsamps);

	for(i = 0; i < g_num_c030_fsamps+2; i++) {
		printf("%3d: %5.3f\n", i, c030_fsamps[i]);
	}

	printf("Samples[] = %d\n", num);

	for(i = 0; i < num+2; i++) {
		printf("%4d: %d  %d\n", i, outptr[0], outptr[1]);
		outptr += 2;
	}
}

int g_sound_play_depth = 0;

void
sound_play(double dcycs)
{
	register word32 start_time1, start_time2, start_time3, start_time4;
	register word32 end_time1, end_time2, end_time3;
	int	doc_active_num[32];
	Doc_reg *rptr;
	int	*outptr;
	int	*outptr_start;
	word32	*sndptr;
	double	dcycs_per_doc_update;
	double	dupd_per_dcyc;
	double	dsamps_per_dcyc;
	double	dcycs_per_samp;
	double	dnum_samps;
	double	dsamp_num;
	int	val, val2;
	int	new_val;
	float	ftmp;
	int	imul;
	int	off;
	int	num;
	float	fsampnum;
	float	next_fsampnum;
	int	c030_lo_val, c030_hi_val;
	float	fc030_range;
	float	fc030_base;
	int	sampnum;
	int	next_sampnum;
	float	fpercent;
	int	c030_state;
	int	val0, val1;
	word32	cur_ptr;
	word32	cur_inc;
	word32	cur_end;
	int	ctl;
	int	num_osc_en;
	int	osc_active;
	int	samps_left;
	int	samps_to_do;
	int	samps_played;
	int	snd_buf_init;
	int	pos;
	int	num_running;
	int	num_samps;
	int	osc;
	int	i, j;


	GET_ITIMER(start_time1);

	g_num_snd_plays++;
	if(g_sound_play_depth) {
		printf("Nested sound_play!\n");
		set_halt(1);
	}

	g_sound_play_depth++;

	/* calc sample num */

	dsamps_per_dcyc = g_dsamps_per_dcyc;
	dcycs_per_samp = g_dcycs_per_samp;

	dsamp_num = dcycs * dsamps_per_dcyc;

	num_samps = (int)(dsamp_num - g_last_sound_play_dsamp);
	dnum_samps = (double)num_samps;

	if(num_samps < 1) {
		/* just say no */
		g_sound_play_depth--;
		return;
	}

	g_last_sound_play_dsamp = g_last_sound_play_dsamp + dnum_samps;

	DOC_LOG("sound_play", -1, dcycs, num_samps);

	dcycs_per_doc_update = g_dcycs_per_doc_update;
	dupd_per_dcyc = g_dupd_per_dcyc;

	if(num_samps > MAX_SND_BUF) {
		printf("num_samps: %d, too big!\n", num_samps);
		g_sound_play_depth--;
		return;
	}


	GET_ITIMER(start_time4);

	outptr_start = &(samp_buf[0]);
	outptr = outptr_start;

	snd_buf_init = 0;

	samps_played = 0;

	num = g_num_c030_fsamps;

	if(num || ((g_vbl_count - g_last_c030_vbl_count) < 240)) {

		if(num) {
			g_last_c030_vbl_count = g_vbl_count;
		}

		pos = 0;
		outptr = outptr_start;
		c030_state = g_c030_state;

		c030_hi_val = ((VAL_C030_BASE + VAL_C030_RANGE)*g_doc_vol) >> 4;
		c030_lo_val = (VAL_C030_BASE * g_doc_vol) >> 4;

		fc030_range = (float)(((VAL_C030_RANGE) * g_doc_vol) >> 4);
		fc030_base = (float)(((VAL_C030_BASE) * g_doc_vol) >> 4);

		val = c030_lo_val;
		if(c030_state) {
			val = c030_hi_val;
		}

		snd_buf_init++;

		c030_fsamps[num] = (float)(num_samps);
		c030_fsamps[num+1] = (float)(num_samps+1);

		ftmp = (float)num_samps;
		/* ensure that all samps are in range */
		for(i = num - 1; i >= 0; i--) {
			if(c030_fsamps[i] > ftmp) {
				c030_fsamps[i] = ftmp;
			}
		}

		num++;
		fsampnum = c030_fsamps[0];
		sampnum = (int)fsampnum;
		fpercent = (float)0.0;
		i = 0;

		while(i < num) {
			next_fsampnum = c030_fsamps[i+1];
			next_sampnum = (int)next_fsampnum;
			if(sampnum < 0 || sampnum > num_samps) {
				printf("play c030: [%d]:%f is %d, > %d\n",
					i, fsampnum, sampnum, num_samps);
				set_halt(1);
				break;
			}

			/* write in samples to all samps < me */
			new_val = c030_lo_val;
			if(c030_state) {
				new_val = c030_hi_val;
			}
			for(j = pos; j < sampnum; j++) {
				outptr[0] = new_val;
				outptr[1] = new_val;
				outptr += 2;
				pos++;
			}

			/* now, calculate me */
			fpercent = (float)0.0;
			if(c030_state) {
				fpercent = (fsampnum - (float)sampnum);
			}

			c030_state = !c030_state;

			while(next_sampnum == sampnum) {
				if(c030_state) {
					fpercent += (next_fsampnum - fsampnum);
				}
				i++;
				fsampnum = next_fsampnum;

				next_fsampnum = c030_fsamps[i+1];
				next_sampnum = (int)next_fsampnum;
				c030_state = !c030_state;
			}

			if(c030_state) {
				/* add in fractional time */
				ftmp = (int)(fsampnum + (float)1.0);
				fpercent += (ftmp - fsampnum);
			}

			if((fpercent < (float)0.0) || (fpercent > (float)1.0)) {
				printf("fpercent: %d = %f\n", i, fpercent);
				show_c030_samps(outptr_start, num_samps);
				set_halt(1);
				break;
			}

			val = (int)((fpercent * fc030_range) + fc030_base);
			outptr[0] = val;
			outptr[1] = val;
			outptr += 2;
			pos++;
			i++;

			sampnum = next_sampnum;
			fsampnum = next_fsampnum;
		}

		samps_played += num_samps;

		/* since we pretended to get one extra sample, we will */
		/*  have toggled the speaker one time too many.  Fix it */
		g_c030_state = !c030_state;

		if(g_send_sound_to_file) {
			show_c030_samps(outptr_start, num_samps);
		}
	}

	g_num_c030_fsamps = 0;

	GET_ITIMER(start_time2);

	num_running = 0;

	num_osc_en = g_doc_num_osc_en;

	osc_active = 0;
	for(j = 0; j < num_osc_en; j++) {
		rptr = &(g_doc_regs[j]);
		rptr->samps_to_do = num_samps;
		if(rptr->running && rptr->cur_inc) {
			doc_active_num[osc_active] = j;
			osc_active++;
		}
	}

	for(j = 0; j < osc_active; j++) {

		osc = doc_active_num[j];
		rptr = &(g_doc_regs[osc]);

		if(!rptr->running || rptr->cur_inc == 0) {
			doc_active_num[j] = doc_active_num[osc_active - 1];
			osc_active--;
			j--;
			continue;
		}

		samps_to_do = rptr->samps_to_do;
		samps_left = rptr->samps_left;
		cur_ptr = rptr->cur_ptr;
		cur_inc = rptr->cur_inc;
		cur_end = rptr->cur_end;
		ctl = rptr->ctl;

		outptr = outptr_start + 2*(num_samps - samps_to_do);
		if(ctl & 0x10) {
			outptr += 1;
		}

		imul = (rptr->vol * g_doc_vol);
		off = imul * 128;

		if(imul == 0) {
			/* produce no sound */
			samps_left = samps_left - samps_to_do;
			cur_ptr += cur_inc * samps_to_do;
			rptr->samps_left = samps_left;
			rptr->cur_ptr = cur_ptr;
			DOC_LOG("nosnd", osc, dcycs, samps_to_do);
			if(samps_left <= 0) {
				doc_active_num[j] = doc_sound_end(osc,
					-samps_left, dcycs, 1);
				j--;
			}
			continue;
		}

		if(snd_buf_init == 0) {
			memset(outptr_start, 0,
					2*sizeof(outptr_start[0])*num_samps);
			snd_buf_init++;
		}

		val = 0;
		for(i = (num_samps - samps_to_do); i < num_samps; i++) {
			pos = cur_ptr >> SND_PTR_SHIFT;
			cur_ptr += cur_inc;
			val = doc_ram[pos];

			val2 = (val * imul - off) >> 4;
			if(val == 0 || cur_ptr >= cur_end) {
				rptr->cur_ptr = cur_ptr;
				doc_active_num[j] = doc_sound_end(osc,
						(num_samps - i), dcycs, val);
				j--;
				val = 0;
				break;
			}

			val2 = outptr[0] + val2;

			samps_left--;
			*outptr = val2;
			outptr += 2;
		}

		if(val != 0) {
			rptr->cur_ptr = cur_ptr;
			rptr->samps_left = samps_left;
		}

		samps_played += (i - (num_samps - samps_to_do));
	}

	GET_ITIMER(end_time2);

	g_cycs_in_sound2 += (end_time2 - start_time2);



	GET_ITIMER(start_time3);

	outptr = outptr_start;

	pos = g_sound_shm_pos;
	sndptr = g_sound_shm_addr;

#if 0
	printf("samps_left: %d, num_samps: %d\n", samps_left, num_samps);
#endif

	if(snd_buf_init) {
		/* convert sound buf */

		for(i = 0; i < num_samps; i++) {
			val0 = outptr[0];
			val1 = outptr[1];
			val = val0;
			if(val0 > 32767) {
				val = 32767;
			}
			if(val0 < -32768) {
				val = -32768;
			}

			val0 = val;
			val = val1;
			if(val1 > 32767) {
				val = 32767;
			}
			if(val1 < -32768) {
				val = -32768;
			}


			outptr += 2;

			sndptr[pos] = (val0 << 16) + (val & 0xffff);
			pos++;
			if(pos >= SOUND_SHM_SAMP_SIZE) {
				pos = 0;
			}
		}

		if(g_queued_nonsamps) {
			/* force out old 0 samps */
			send_sound(g_pipe_fd[1], 0, g_queued_nonsamps);
			g_queued_nonsamps = 0;
		}

		if(g_send_sound_to_file) {
			send_sound_to_file(g_sound_shm_addr, g_sound_shm_pos,
				num_samps);
		}

		g_queued_samps += num_samps;
	} else {
		/* move pos */
		pos += num_samps;
		while(pos >= SOUND_SHM_SAMP_SIZE) {
			pos -= SOUND_SHM_SAMP_SIZE;
		}

		if(g_send_sound_to_file) {
			send_sound_to_file(zero_buf, g_sound_shm_pos,
				num_samps);
		}

		if(g_queued_samps) {
			/* force out old non-0 samps */
			send_sound(g_pipe_fd[1], 1, g_queued_samps);
			g_queued_samps = 0;
		}

		g_queued_nonsamps += num_samps;
	}

	g_sound_shm_pos = pos;


	GET_ITIMER(end_time3);

	g_fvoices += ((float)(samps_played) / (float)(AUDIO_RATE));

	if(g_queued_samps >= (AUDIO_RATE/30)) {
		send_sound(g_pipe_fd[1], 1, g_queued_samps);
		g_queued_samps = 0;
	}

	if(g_queued_nonsamps >= (AUDIO_RATE/30)) {
		send_sound(g_pipe_fd[1], 0, g_queued_nonsamps);
		g_queued_nonsamps = 0;
	}

	GET_ITIMER(end_time1);

	g_cycs_in_sound1 += (end_time1 - start_time1);
	g_cycs_in_sound3 += (end_time3 - start_time3);
	g_cycs_in_sound4 += (start_time2 - start_time4);

	g_sound_play_depth--;
}


void
doc_handle_event(int osc, double dcycs)
{
	/* handle osc stopping and maybe interrupting */

	g_num_doc_events++;

	DOC_LOG("doc_ev", osc, dcycs, 0);

	g_doc_regs[osc].event = 0;

	sound_play(dcycs);

}

int
doc_sound_end(int osc, int samps_to_do, double dcycs, int can_repeat)
{
	Doc_reg	*rptr;
	double	dcyc_ev;
	int	mode;
	int	other_osc;
	int	ctl;

	/* handle osc stopping and maybe interrupting */

#if 0
	printf("osc %d stopped at %f, repeat: %d\n", osc, dcycs, can_repeat);
#endif

	if(osc < 0 || osc > 31) {
		printf("doc_handle_event: osc: %d!\n", osc);
		return osc;
	}

	rptr = &(g_doc_regs[osc]);
	ctl = rptr->ctl;

	if(rptr->event) {
		dcyc_ev = remove_event_doc(osc);
		if((ctl & 0x08) && (dcyc_ev - dcycs) > 100.0) {
			printf("sound end: %d, int1, event: %f now %f\n",
				osc, dcyc_ev, dcycs);
			printf("ev2: %f\n", rptr->dcyc_ev2);
			printf("It's now: %f, 3:%f 4:%f\n", dcycs,
				rptr->dcyc_ev3, rptr->dcyc_ev4);
			printf("dcycs_per_samp: %f\n", g_dcycs_per_samp);
			set_halt(1);
		}
	} else {
		/* make sure we have not too many more cycles left */
		if((ctl & 0x8) && samps_to_do > 7) {
			dcyc_ev = rptr->dcyc_ev;
			printf("sound end: %d, interrupt, but no ev: %f %f!\n",
				osc, dcyc_ev, rptr->dcyc_ev2);
			printf("It's now: %f, 3:%f 4:%f\n", dcycs,
				rptr->dcyc_ev3, rptr->dcyc_ev4);
			printf("dcycs_per_samp: %f\n", g_dcycs_per_samp);
			set_halt(1);
		}
	}
	rptr->event = 0;


	/* check to make sure osc is running */
	if(ctl & 0x01) {
		/* Oscillator already stopped. */
		printf("Osc %d interrupt, but it was already stopped!\n",osc);
		U_STACK_TRACE();
		set_halt(1);
		return osc;
	}

	if(ctl & 0x08) {
		if(rptr->has_irq_pending == 0) {
			add_sound_irq(osc);
		}
	}

	if(!rptr->running) {
		printf("Doc event for osc %d, but ! running\n", osc);
		set_halt(1);
	}

	rptr->running = 0;

	mode = (ctl >> 1) & 3;

	if(mode == 3) {
		/* swap mode! */
		rptr->ctl |= 1;
		other_osc = osc ^ 1;
		rptr = &(g_doc_regs[other_osc]);
		if(!rptr->running && (rptr->ctl & 0x1)) {
			rptr->ctl = rptr->ctl & (~1);
			start_sound(other_osc, dcycs, samps_to_do);
		}
		return other_osc;
	} else if(mode == 0 && can_repeat) {
		/* free-running mode with no 0 byte! */
		/* start doing it again */

		start_sound(osc, dcycs, samps_to_do);

		return osc;
	} else {
		/* stop the oscillator */
		rptr->ctl |= 1;
	}

	return osc;
}

void
add_sound_irq(int osc)
{
	if(g_doc_regs[osc].has_irq_pending) {
		printf("Adding sound_irq for %02x, but irq_p: %d\n", osc,
			g_doc_regs[osc].has_irq_pending);
		set_halt(1);
	}

	g_doc_regs[osc].has_irq_pending = 1;
	add_irq();
	doc_reg_e0 = 0x00 + (osc << 1);
}

void
remove_sound_irq(int osc)
{
	int	i;

	doc_printf("remove irq for osc: %d, has_irq: %d\n",
		osc, g_doc_regs[osc].has_irq_pending);

	if(g_doc_regs[osc].has_irq_pending) {
		g_doc_regs[osc].has_irq_pending = 0;
		remove_irq();

		for(i = 0; i < g_doc_num_osc_en; i++) {
			if(g_doc_regs[i].has_irq_pending) {
				/* This guy is still interrupting! */
				doc_reg_e0 = 0x00 + (i << 1);
				return;
			}
		}

		doc_reg_e0 |= 0x80;
	} else {
#if 0
		/* make sure no int pending */
		if(doc_reg_e0 != 0xff) {
			printf("remove_sound_irq[%02x] = 0, but e0: %02x\n",
				osc, doc_reg_e0);
			set_halt(1);
		}
#endif
	}

	if(doc_reg_e0 & 0x80) {
		for(i = 0; i < 0x20; i++) {
			if(g_doc_regs[i].has_irq_pending) {
				printf("remove_sound_irq[%02x], but [%02x]!\n",
					osc, i);
				set_halt(1);
			}
		}
	}
}

void
start_sound(int osc, double dcycs, int samps_to_do)
{
	register word32 start_time1;
	register word32 end_time1;
	Doc_reg	*rptr;
	int	ctl;
	int	mode;
	word32	sz;
	word32	size;
	word32	wave_size;

	if(osc < 0 || osc > 31) {
		printf("start_sound: osc: %02x!\n", osc);
		set_halt(1);
	}

	g_num_start_sounds++;

	if(dcycs < 1.0) {
		printf("start_sound, but dcycs: %f\n", dcycs);
	}

	rptr = &(g_doc_regs[osc]);

	if(osc >= g_doc_num_osc_en) {
		rptr->ctl |= 1;
		return;
	}

	GET_ITIMER(start_time1);

	ctl = rptr->ctl;

	mode = (ctl >> 1) & 3;

	wave_size = rptr->wavesize;

	sz = ((wave_size >> 3) & 7) + 8;
	size = 1 << sz;

	if(size < 0x100) {
		printf("size: %08x is too small, sz: %08x!\n", size, sz);
		set_halt(1);
	}

	if(rptr->running) {
		printf("start_sound osc: %d, already running!\n", osc);
		set_halt(1);
	}

	rptr->running = 1;

	rptr->cur_ptr = rptr->cur_start;
	rptr->samps_to_do = samps_to_do;

	doc_printf("Starting osc %02x, dcycs: %f\n", osc, dcycs);
	doc_printf("size: %04x\n", size);

	if((mode == 2) && ((osc & 1) == 0)) {
		printf("Sync mode osc %d starting!\n", osc);
		set_halt(1);

		/* see if we should start our odd partner */
		if((rptr[1].ctl & 7) == 5) {
			/* odd partner stopped in sync mode--start him */
			rptr[1].ctl &= (~1);
			start_sound(osc + 1, dcycs, samps_to_do);
		} else {
			printf("Osc %d starting sync, but osc %d ctl: %02x\n",
				osc, osc+1, rptr[1].ctl);
			set_halt(1);
		}
	}

	wave_end_estimate(osc, dcycs, samps_to_do);

	DOC_LOG("start playing", osc, dcycs, size);

	GET_ITIMER(end_time1);

	g_cycs_in_start_sound += (end_time1 - start_time1);
}

void
wave_end_estimate(int osc, double dcycs, int samps_to_do)
{
	register word32 start_time1;
	register word32 end_time1;
	Doc_reg *rptr;
	byte	*ptr1;
	double	dcycs_per_samp;
	double	dsamps_per_byte;
	double	dsamps;
	double	dcur_inc;
	double	dsamps_to_do;
	word32	tmp1;
	word32	cur_inc;
	word32	save_val;
	int	save_size;
	int	pos;
	int	size;
	int	estimate;

	GET_ITIMER(start_time1);

	dcycs_per_samp = g_dcycs_per_samp;

	rptr = &(g_doc_regs[osc]);

	cur_inc = rptr->cur_inc;
	dcur_inc = (double)cur_inc;
	dsamps_per_byte = 0.0;
	if(cur_inc) {
		dsamps_per_byte = SND_PTR_SHIFT_DBL / (double)dcur_inc;
	}

	/* see if there's a zero byte */
	tmp1 = rptr->cur_ptr;
	pos = tmp1 >> SND_PTR_SHIFT;
	size = ((rptr->cur_end) >> SND_PTR_SHIFT) - pos;

	ptr1 = &doc_ram[pos];

	estimate = 0;
	if(rptr->ctl & 0x08 || g_doc_regs[osc ^ 1].ctl & 0x08) {
		estimate = 1;
	}

#if 0
	estimate = 1;
#endif
	if(estimate) {
		save_size = size;
		save_val = ptr1[size];
		ptr1[size] = 0;
		size = strlen((char *)ptr1);
		ptr1[save_size] = save_val;
	}

	/* calc samples to play */
	dsamps = (dsamps_per_byte * (double)size) + 1.0;
	dsamps_to_do = (double)samps_to_do;

	rptr->samps_left = (int)dsamps;

	if(rptr->event) {
		remove_event_doc(osc);
	}
	rptr->event = 0;

	if(estimate) {
		rptr->event = 1;
		rptr->dcyc_ev = dcycs +
				((dsamps - dsamps_to_do) * dcycs_per_samp);
		rptr->dcyc_ev2 = dcycs;
		rptr->dcyc_ev3 = dsamps;
		rptr->dcyc_ev4 = dsamps_to_do;
		add_event_doc(dcycs +
			((dsamps - dsamps_to_do + 1.0) * dcycs_per_samp), osc);
	}

	GET_ITIMER(end_time1);

	g_cycs_in_est_sound += (end_time1 - start_time1);
}


void
remove_sound_event(osc)
{
	if(g_doc_regs[osc].event) {
		g_doc_regs[osc].event = 0;
		remove_event_doc(osc);
	}
}


void
doc_write_ctl_reg(int osc, int val, double dcycs, int samps_to_do)
{
	word32	old_halt;
	word32	new_halt;
	int	old_val;
	int	mode;

	if(osc < 0 || osc >= 0x20) {
		printf("doc_write_ctl_reg: osc: %02x, val: %02x\n",
			osc, val);
		set_halt(1);
		return;
	}
	if(dcycs < 1.0 && g_last_vbl_dcycs > 10000.0) {
		printf("doc_write_ctl_reg, dcycs: %f\n", dcycs);
	}

	old_val = g_doc_regs[osc].ctl;
	g_doc_saved_ctl = old_val;

	if(old_val == val) {
		return;
	}

	mode = (val >> 1) & 3;

	old_halt = (old_val & 1);
	new_halt = (val & 1);

	/* bits are:	28: old int bit */
	/*		29: old halt bit */
	/*		30: new int bit */
	/*		31: new halt bit */

#if 0
	if(osc == 0x10) {
		printf("osc %d new_ctl: %02x, old: %02x\n", osc, val, old_val);
	}
#endif

	/* no matter what, remove any pending IRQs on this osc */
	remove_sound_irq(osc);

#if 0
	if(old_halt) {
		printf("doc_write_ctl to osc %d, val: %02x, old: %02x\n",
			osc, val, old_val);
	}
#endif

	if(new_halt != 0) {
		/* make sure sound is stopped */
		remove_sound_event(osc);
		if(old_halt == 0) {
			/* it was playing, finish it up */
#if 0
			printf("Aborted osc %d at dcyc: %f, ctl: %02x, "
				"oldctl: %02x\n",
				osc, dcycs, val, old_val);
			set_halt(1);
#endif
			/* sound_play(dcycs); */
		}

		g_doc_regs[osc].ctl = val;
		g_doc_regs[osc].running = 0;
	} else {
		/* new halt == 0 = make sure sound is running */
		if(old_halt != 0) {
			/* start sound */
			sound_play(dcycs);
			g_doc_regs[osc].ctl = val;

			start_sound(osc, dcycs, samps_to_do);
		} else {
			/* was running, and something changed */
			if(old_val != val) {
				doc_printf("osc %d old ctl:%02x new:%02x!\n",
					osc, old_val, val);
				/* sound_play(dcycs); */
			}

			g_doc_regs[osc].ctl = val;
		}
	}
}

void
doc_recalc_sound_parms(int osc, double dcycs)
{
	Doc_reg	*rptr;
	double	dfreq;
	double	dtmp1;
	double	dacc, dacc_recip;
	word32	res;
	word32	sz;
	word32	size;
	word32	wave_size;
	word32	cur_start;
	word32	shifted_size;

	g_num_recalc_snd_parms++;

	rptr = &(g_doc_regs[osc]);

	wave_size = rptr->wavesize;

	dfreq = (double)rptr->freq;

	sz = ((wave_size >> 3) & 7) + 8;
	size = 1 << sz;
	rptr->size_bytes = size;
	res = wave_size & 7;

	shifted_size = size << SND_PTR_SHIFT;
	cur_start = (rptr->waveptr << (8 + SND_PTR_SHIFT)) & (-shifted_size);

	dtmp1 = (dfreq * (DOC_SCAN_RATE / (double)AUDIO_RATE));
	dacc = (double)(1 << (20 - (17 - sz + res)));
	dacc_recip = (SND_PTR_SHIFT_DBL) / ((double)(1 << 20));
	dtmp1 = dtmp1 * g_drecip_osc_en_plus_2 * dacc * dacc_recip;

	rptr->cur_inc = (int)(dtmp1);
	rptr->cur_start = cur_start;
	rptr->cur_end = cur_start + shifted_size;
}

int
doc_read_c030(double dcycs)
{
	int	num;

	num = g_num_c030_fsamps;
	if(num >= MAX_C030_TIMES) {
		printf("Too many clicks per vbl: %d\n", num);
		set_halt(1);
		return 0;
	}

	c030_fsamps[num] = (float)(dcycs * g_dsamps_per_dcyc -
						g_last_sound_play_dsamp);
	g_num_c030_fsamps = num + 1;

	doc_printf("read c030, num this vbl: %04x\n", num);

	return 0;
}

int
doc_read_c03c(double dcycs)
{
	return doc_sound_ctl;
}

int
doc_read_c03d(double dcycs)
{
	Doc_reg	*rptr;
	int	osc;
	int	type;
	int	ret;

	ret = doc_saved_val;
	if(doc_sound_ctl & 0x40) {
		/* Read RAM */
		doc_saved_val = doc_ram[doc_ptr];
	} else {
		/* Read DOC */
		doc_saved_val = 0;

		osc = doc_ptr & 0x1f;
		type = (doc_ptr >> 5) & 0x7;
		rptr = &(g_doc_regs[osc]);

		switch(type) {
		case 0x0:	/* freq lo */
			doc_saved_val = rptr->freq & 0xff;
			break;
		case 0x1:	/* freq hi */
			doc_saved_val = rptr->freq >> 8;
			break;
		case 0x2:	/* vol */
			doc_saved_val = rptr->vol;
			break;
		case 0x3:	/* data register */
#if 0
			printf("Reading doc_data_reg[%02x]!\n", osc);
#endif
			doc_saved_val = 0;
			break;
		case 0x4:	/* wave ptr register */
			doc_saved_val = rptr->waveptr;
			break;
		case 0x5:	/* control register */
			doc_saved_val = rptr->ctl;
			break;
		case 0x6:	/* control register */
			doc_saved_val = rptr->wavesize;
			break;
		case 0x7:	/* 0xe0-0xff */
			switch(osc) {
			case 0x00:	/* 0xe0 */
				doc_saved_val = doc_reg_e0;
				doc_printf("Reading doc 0xe0, ret: %02x\n",
								doc_saved_val);

				/* Clear IRQ on read of e0, if any irq pend */
				if((doc_reg_e0 & 0x80) == 0) {
					remove_sound_irq(doc_reg_e0 >> 1);
				}
				break;
			case 0x01:	/* 0xe1 */
				doc_saved_val = (g_doc_num_osc_en - 1) << 1;
				break;
			case 0x02:	/* 0xe2 */
				doc_saved_val = 0x80;
				printf("Reading doc 0xe2, ret: %02x\n",
								doc_saved_val);
				set_halt(1);
				break;
			default:
				doc_saved_val = 0;
				printf("Reading bad doc_reg[%04x]: %02x\n",
							doc_ptr, doc_saved_val);
				set_halt(1);
			}
			break;
		default:
			doc_saved_val = 0;
			printf("Reading bad doc_reg[%04x]: %02x\n",
						doc_ptr, doc_saved_val);
			set_halt(1);
		}
	}

	doc_printf("read c03d, doc_ptr: %04x, ret: %02x, saved: %02x\n",
		doc_ptr, ret, doc_saved_val);

	if(doc_sound_ctl & 0x20) {
		doc_ptr = (doc_ptr + 1) & 0xffff;
	}


	return ret;
}

void
doc_write_c03c(int val, double dcycs)
{
	int	vol;

	vol = val & 0xf;
	if(g_doc_vol != vol) {
		/* don't bother playing sound..wait till next update */
		/* sound_play(dcycs); */

		g_doc_vol = vol;
		doc_printf("Setting doc vol to 0x%x at %f\n",
			vol, dcycs);
	}

	doc_sound_ctl = val;
}

void
doc_write_c03d(int val, double dcycs)
{
	Doc_reg	*rptr;
	int	osc;
	int	type;
	int	ctl;
	int	i;

	val = val & 0xff;

	doc_printf("write c03d, doc_ptr: %04x, val: %02x\n",
		doc_ptr, val);

	if(doc_sound_ctl & 0x40) {
		/* RAM */
		doc_ram[doc_ptr] = val;
	} else {
		/* DOC */
#if 0
		printf("doc ctl write: %02x: %02x\n", doc_ptr & 0xff, val);
#endif

		osc = doc_ptr & 0x1f;
		type = (doc_ptr >> 5) & 0x7;
		
		rptr = &(g_doc_regs[osc]);
		ctl = rptr->ctl;
#if 0
		if((ctl & 1) == 0) {
			if(type < 2 || type == 4 || type == 6) {
				printf("Osc %d is running, old ctl: %02x, "
					"but write reg %02x=%02x\n",
					osc, ctl, doc_ptr & 0xff, val);
				set_halt(1);
			}
		}
#endif

		switch(type) {
		case 0x0:	/* freq lo */
			if((rptr->freq & 0xff) == val) {
				break;
			}
			if((ctl & 1) == 0) {
				/* play through current status */
				sound_play(dcycs);
			}
			rptr->freq = (rptr->freq & 0xff00) + val;
			doc_recalc_sound_parms(osc, dcycs);
			break;
		case 0x1:	/* freq hi */
			if((rptr->freq >> 8) == val) {
				break;
			}
			if((ctl & 1) == 0) {
				/* play through current status */
				sound_play(dcycs);
			}
			rptr->freq = (rptr->freq & 0xff) + (val << 8);
			doc_recalc_sound_parms(osc, dcycs);
			break;
		case 0x2:	/* vol */
			if(rptr->vol == val) {
				break;
			}
			if((ctl & 1) == 0) {
				/* play through current status */
				sound_play(dcycs);
			}
			rptr->vol = val;
			break;
		case 0x3:	/* data register */
#if 0
			printf("Writing %02x into doc_data_reg[%02x]!\n",
				val, osc);
#endif
			break;
		case 0x4:	/* wave ptr register */
			if(rptr->waveptr == val) {
				break;
			}
			if((ctl & 1) == 0) {
				/* play through current status */
				sound_play(dcycs);
			}
			rptr->waveptr = val;
			doc_recalc_sound_parms(osc, dcycs);
			break;
		case 0x5:	/* control register */
#if 0
			printf("doc_write ctl osc %d, val: %02x\n", osc, val);
#endif
			if(rptr->ctl == val) {
				break;
			}
			doc_write_ctl_reg(osc, val, dcycs, 0);
			break;
		case 0x6:	/* wavesize register */
			if(rptr->wavesize == val) {
				break;
			}
			if((ctl & 1) == 0) {
				/* play through current status */
				sound_play(dcycs);
			}
			rptr->wavesize = val;
			doc_recalc_sound_parms(osc, dcycs);
			break;
		case 0x7:	/* 0xe0-0xff */
			switch(osc) {
			case 0x00:	/* 0xe0 */
				doc_printf("writing doc 0xe0 with %02x, "
					"was:%02x\n", val, doc_reg_e0);
				if(val != doc_reg_e0) {
#if 0
					set_halt(1);
#endif
				}
				break;
			case 0x01:	/* 0xe1 */
				doc_printf("Writing doc 0xe1 with %02x\n", val);
				g_doc_num_osc_en = (val >> 1) + 1;
				UPDATE_G_DCYCS_PER_DOC_UPDATE((val >> 1) + 1);
				if((val & 1) || (val > 0x40)) {
					printf("doc 0xe1: %02x!\n", val);
					set_halt(1);
				}

				/* Stop any oscs that were running but now */
				/*   are disabled */
				for(i = g_doc_num_osc_en; i < 0x20; i++) {
					doc_write_ctl_reg(i,
						g_doc_regs[i].ctl | 1, dcycs,0);
				}

				break;
			case 0x02:	/* 0xe2 */
				printf("Writing doc 0xe2 with %02x\n", val);
				set_halt(1);
				break;
			default:
				printf("Writing %02x into bad doc_reg[%04x]\n",
					val, doc_ptr);
				set_halt(1);
			}
			break;
		default:
			printf("Writing %02x into bad doc_reg[%04x]\n",
				val, doc_ptr);
			set_halt(1);
		}
	}

	if(doc_sound_ctl & 0x20) {
		doc_ptr = (doc_ptr + 1) & 0xffff;
	}

	doc_saved_val = val;
}

void
doc_write_c03e(int val)
{
	doc_ptr = (doc_ptr & 0xff00) + val;
}

void
doc_write_c03f(int val)
{
	doc_ptr = (doc_ptr & 0xff) + (val << 8);
}

void
doc_show_ensoniq_state(int osc)
{
	Doc_reg	*rptr;
	int	i;

	printf("Ensoniq state\n");
	printf("c03c doc_sound_ctl: %02x, doc_saved_val: %02x\n",
		doc_sound_ctl, doc_saved_val);
	printf("doc_ptr: %04x,    num_osc_en: %02x, e0: %02x\n", doc_ptr,
		g_doc_num_osc_en, doc_reg_e0);

	for(i = 0; i < 32; i += 8) {
		printf("irqp: %02x: %04x %04x %04x %04x %04x %04x %04x %04x\n",
			i,
			g_doc_regs[i].has_irq_pending,
			g_doc_regs[i + 1].has_irq_pending,
			g_doc_regs[i + 2].has_irq_pending,
			g_doc_regs[i + 3].has_irq_pending,
			g_doc_regs[i + 4].has_irq_pending,
			g_doc_regs[i + 5].has_irq_pending,
			g_doc_regs[i + 6].has_irq_pending,
			g_doc_regs[i + 7].has_irq_pending);
	}

	for(i = 0; i < 32; i += 8) {
		printf("freq: %02x: %04x %04x %04x %04x %04x %04x %04x %04x\n",
			i,
			g_doc_regs[i].freq, g_doc_regs[i + 1].freq,
			g_doc_regs[i + 2].freq, g_doc_regs[i + 3].freq,
			g_doc_regs[i + 4].freq, g_doc_regs[i + 5].freq,
			g_doc_regs[i + 6].freq, g_doc_regs[i + 7].freq);
	}

	for(i = 0; i < 32; i += 8) {
		printf("vol: %02x: %02x %02x %02x %02x %02x %02x %02x %02x\n",
			i,
			g_doc_regs[i].vol, g_doc_regs[i + 1].vol,
			g_doc_regs[i + 2].vol, g_doc_regs[i + 3].vol,
			g_doc_regs[i + 4].vol, g_doc_regs[i + 5].vol,
			g_doc_regs[i + 6].vol, g_doc_regs[i + 6].vol);
	}

	for(i = 0; i < 32; i += 8) {
		printf("wptr: %02x: %02x %02x %02x %02x %02x %02x %02x %02x\n",
			i,
			g_doc_regs[i].waveptr, g_doc_regs[i + 1].waveptr,
			g_doc_regs[i + 2].waveptr, g_doc_regs[i + 3].waveptr,
			g_doc_regs[i + 4].waveptr, g_doc_regs[i + 5].waveptr,
			g_doc_regs[i + 6].waveptr, g_doc_regs[i + 7].waveptr);
	}

	for(i = 0; i < 32; i += 8) {
		printf("ctl: %02x: %02x %02x %02x %02x %02x %02x %02x %02x\n",
			i,
			g_doc_regs[i].ctl, g_doc_regs[i + 1].ctl,
			g_doc_regs[i + 2].ctl, g_doc_regs[i + 3].ctl,
			g_doc_regs[i + 4].ctl, g_doc_regs[i + 5].ctl,
			g_doc_regs[i + 6].ctl, g_doc_regs[i + 7].ctl);
	}

	for(i = 0; i < 32; i += 8) {
		printf("wsize: %02x: %02x %02x %02x %02x %02x %02x %02x %02x\n",
			i,
			g_doc_regs[i].wavesize, g_doc_regs[i + 1].wavesize,
			g_doc_regs[i + 2].wavesize, g_doc_regs[i + 3].wavesize,
			g_doc_regs[i + 4].wavesize, g_doc_regs[i + 5].wavesize,
			g_doc_regs[i + 6].wavesize, g_doc_regs[i + 7].wavesize);
	}

	show_doc_log();

	for(i = 0; i < 32; i++) {
		rptr = &(g_doc_regs[i]);
		printf("%2d: ctl:%02x wp:%02x ws:%02x freq:%04x vol:%02x "
			"ev:%d run:%d irq:%d sz:%04x\n", i,
			rptr->ctl, rptr->waveptr, rptr->wavesize, rptr->freq,
			rptr->vol, rptr->event, rptr->running,
			rptr->has_irq_pending, rptr->size_bytes);
		printf("    ptr:%08x inc:%08x st:%08x end:%08x samp_to_do:%d\n",
			rptr->cur_ptr, rptr->cur_inc, rptr->cur_start,
			rptr->cur_end, rptr->samps_to_do);
		printf("    samps_left:%d ev:%f ev2:%f ev3:%f ev4:%f\n",
			rptr->samps_left, rptr->dcyc_ev, rptr->dcyc_ev2,
			rptr->dcyc_ev3, rptr->dcyc_ev4);
	}

#if 0
	for(osc = 0; osc < 32; osc++) {
		fmax = 0.0;
		printf("osc %d has %d samps\n", osc, g_fsamp_num[osc]);
		for(i = 0; i < g_fsamp_num[osc]; i++) {
			printf("%4d: %f\n", i, g_fsamps[osc][i]);
			fmax = MAX(fmax, g_fsamps[osc][i]);
		}
		printf("osc %d, fmax: %f\n", osc, fmax);
	}
#endif
}
