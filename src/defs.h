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

#ifdef INCLUDE_RCSID_S
	.data
	.export rcsdif_defs_h,data
rcsdif_defs_h
	.stringz "@(#)$Header: defs.h,v 1.16 99/06/22 22:38:44 kentd Exp $"
	.code
#endif

#include "defcomm.h"

link		.reg	%r2
acc		.reg	%r3
xreg		.reg	%r4
yreg		.reg	%r5
stack		.reg	%r6
dbank		.reg	%r7
direct		.reg	%r8
neg		.reg	%r9
zero		.reg	%r10
psr		.reg	%r11
pc		.reg	%r12
#if 0
cycles		.reg	%r13
#endif

kbank		.reg	%r14
page_info_ptr	.reg	%r15
inst_tab_ptr	.reg	%r16
fcycles_stop_ptr .reg	%r17
addr_latch	.reg	%r18

scratch1	.reg	%r19
scratch2	.reg	%r20
scratch3	.reg	%r21
scratch4	.reg	%r22
instr		.reg	%r23		; arg3

fr_dbl_1	.reg	%fr12
fr_dbl_2	.reg	%fr13
fr_dbl_3	.reg	%fr14
fr_dbl_4	.reg	%fr15

fcycles		.reg	%fr12L
ftmp_unused1	.reg	%fr12R
fr_plus_1	.reg	%fr13L
fr_plus_2	.reg	%fr13R
fr_plus_3	.reg	%fr14L
fr_plus_x_m1	.reg	%fr14R
fcycles_stop	.reg	%fr15L
ftmp_unused2	.reg	%fr15R

ftmp1		.reg	%fr4
ftmp2		.reg	%fr5
fscr1		.reg	%fr6

#define LDC(val,reg) ldil L%val,reg ! ldo R%val(reg),reg


