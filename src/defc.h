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
const char rcsid_defc_h[] = "@(#)$Header: defc.h,v 1.60 98/07/04 19:32:19 kentd Exp $";
#endif

#include "defcomm.h"

#define STRUCT(a) typedef struct _ ## a a; struct _ ## a

typedef unsigned char byte;
typedef unsigned short word16;
typedef unsigned int word32;

void U_STACK_TRACE();

/* 28MHz crystal, plus every 65th 1MHz cycle is stretched 140ns */
#define CYCS_28_MHZ		(28636360)
#define DCYCS_28_MHZ		(1.0*CYCS_28_MHZ)
#define CYCS_3_5_MHZ		(CYCS_28_MHZ/8)
#define DCYCS_1_MHZ		((DCYCS_28_MHZ/28.0)*(65.0*7/(65.0*7+1.0)))
#define CYCS_1_MHZ		((int)DCYCS_1_MHZ)

#ifdef KEGS_LITTLE_ENDIAN
# define BIGEND(a)    ((((a) >> 24) & 0xff) +			\
			(((a) >> 8) & 0xff00) + 		\
			(((a) << 8) & 0xff0000) + 		\
			(((a) << 24) & 0xff000000))
#else
# define BIGEND(a)	(a)
#endif

typedef float Cyc;

#define MAXNUM_HEX_PER_LINE     32

#ifdef __NeXT__
# include <libc.h>
#endif
#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#ifdef HPUX
# include <machine/inline.h>		/* for GET_ITIMER */
#endif

#ifndef O_BINARY
/* work around some Windows junk */
# define O_BINARY	0
#endif

STRUCT(Pc_log) {
	word32	dbank_kbank_pc;
	word32	instr;
	word32	psr_acc;
	word32	xreg_yreg;
	word32	stack_direct;
};

STRUCT(Event) {
	double	dcycs;
	int	type;
	Event	*next;
};

STRUCT(Fplus) {
	float	plus_1;
	float	plus_2;
	float	plus_3;
	float	plus_x_minus_1;
};

STRUCT(Engine_reg) {
	float	fcycles;
	float	fcycles_stop;
	Fplus	*fplus_ptr;

	word32	kbank;
	word32	pc;
	word32	acc;
	word32	xreg;
	word32	yreg;
	word32	stack;
	word32	dbank;
	word32	direct;
	word32	psr;
};

STRUCT(Page_info) {
	word32 rd_wr;
};

STRUCT(Doc_reg) {
	double	dcyc_ev;
	double	dcyc_ev2;
	double	dcyc_ev3;
	double	dcyc_ev4;
	int	samps_to_do;
	int	samps_left;
	word32	cur_ptr;
	word32	cur_inc;
	word32	cur_start;
	word32	cur_end;
	int	size_bytes;
	int	event;
	int	running;
	int	has_irq_pending;
	word32	freq;
	word32	vol;
	word32	waveptr;
	word32	ctl;
	word32	wavesize;
};


#define ALTZP	(statereg & 0x80)
#define PAGE2	(statereg & 0x40)
#define RAMRD	(statereg & 0x20)
#define RAMWRT	(statereg & 0x10)
#define RDROM	(statereg & 0x08)
#define LCBANK2	(statereg & 0x04)
#define ROMB	(statereg & 0x02)
#define INTCX	(statereg & 0x01)

#define EXTRU(val, pos, len) 				\
	( ( (len) >= (pos) + 1) ? ((val) >> (31-(pos))) : \
	  (((val) >> (31-(pos)) ) & ( (1<<(len) ) - 1) ) )

#define DEP1(val, pos, old_val)				\
	(((old_val) & ~(1 << (31 - (pos))) ) |		\
	 ( ((val) & 1) << (31 - (pos))) )

#define set_halt(val) \
	if(val) { set_halt_act(val); }

#define clear_halt() \
	clr_halt_act()

extern int errno;

#define GET_PAGE_INFO_RD(page) \
	(page_info_rd_wr[page].rd_wr)

#define GET_PAGE_INFO_WR(page) \
	(page_info_rd_wr[0x10000 + PAGE_INFO_PAD_SIZE + (page)].rd_wr)

#define SET_PAGE_INFO_RD(page,val) \
	;page_info_rd_wr[page].rd_wr = val;

#define SET_PAGE_INFO_WR(page,val) \
	;page_info_rd_wr[0x10000 + PAGE_INFO_PAD_SIZE + (page)].rd_wr = val;

#define VERBOSE_DISK	0x001
#define VERBOSE_IRQ	0x002
#define VERBOSE_CLK	0x004
#define VERBOSE_SHADOW	0x008
#define VERBOSE_IWM	0x010
#define VERBOSE_DOC	0x020
#define VERBOSE_ADB	0x040
#define VERBOSE_SCC	0x080
#define VERBOSE_TEST	0x100
#define VERBOSE_VIDEO	0x200

#ifdef NO_VERB
# define DO_VERBOSE	0
#else
# define DO_VERBOSE	1
#endif

#define disk_printf	if(DO_VERBOSE && (Verbose & VERBOSE_DISK)) printf
#define irq_printf	if(DO_VERBOSE && (Verbose & VERBOSE_IRQ)) printf
#define clk_printf	if(DO_VERBOSE && (Verbose & VERBOSE_CLK)) printf
#define shadow_printf	if(DO_VERBOSE && (Verbose & VERBOSE_SHADOW)) printf
#define iwm_printf	if(DO_VERBOSE && (Verbose & VERBOSE_IWM)) printf
#define doc_printf	if(DO_VERBOSE && (Verbose & VERBOSE_DOC)) printf
#define adb_printf	if(DO_VERBOSE && (Verbose & VERBOSE_ADB)) printf
#define scc_printf	if(DO_VERBOSE && (Verbose & VERBOSE_SCC)) printf
#define test_printf	if(DO_VERBOSE && (Verbose & VERBOSE_TEST)) printf
#define vid_printf	if(DO_VERBOSE && (Verbose & VERBOSE_VIDEO)) printf


#ifndef MIN
# define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
# define MAX(a,b)	(((a) < (b)) ? (b) : (a))
#endif

#ifdef HPUX
# ifdef __GNUC__
/* Assume GNU C doesn't know inline assembly */
#  define GET_ITIMER(dest)	\
	__asm__ volatile ("mfctl 16,%0" : "=r" (dest))
# else
/* Assume HP compiler with inline asm */
#  define GET_ITIMER(dest)		\
	_MFCTL(16,dest)
# endif
#else
# define GET_ITIMER(dest)	dest = 0;
#endif

#include "iwm.h"
#include "protos.h"
