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

#ifdef ASM
# ifdef INCLUDE_RCSID_S
	.stringz "@(#)$Header: instable.h,v 1.77 97/07/27 14:24:18 kentd Exp $"
# endif
#endif

inst00_SYM		/*  brk */
#ifdef ASM
	ldil	l%g_testing,arg3
	ldil	l%g_num_brk,arg1
	ldw	r%g_testing(arg3),arg3
	addi	2,pc,pc
	ldw	r%g_num_brk(arg1),arg2
	comib,<> 0,arg3,brk_testing_SYM
	extru	pc,31,16,arg0
	addi	1,arg2,arg2
	bb,>=	psr,23,brk_native_SYM
	stw	arg2,r%g_num_brk(arg1)

	bl	push_16,link
	nop

	bl	push_8,link
	extru	psr,31,8,arg0		;B bit already on in PSR

	ldil	l%0xfffe,arg0
	bl	get_mem_long_16,link
	ldo	r%0xfffe(arg0),arg0

#if 0
	ldil	l%halt_sim,arg0
	ldi	3,arg1
	stw	arg1,r%halt_sim(arg0)
#endif


	ldi	0,kbank
	ldi	0,dbank			;clear dbank in emul mode
	copy	ret0,pc
	b	dispatch
	depi	1,29,2,psr		;ints masked, decimal off


brk_native_SYM
	stw	arg0,STACK_SAVE_COP_ARG0(sp)
	bl	push_8,link
	copy	kbank,arg0

	bl	push_16,link
	ldw	STACK_SAVE_COP_ARG0(sp),arg0

	bl	push_8,link
	extru	psr,31,8,arg0

	ldil	l%0xffe6,arg0
	bl	get_mem_long_16,link
	ldo	r%0xffe6(arg0),arg0

#if 0
#endif
	ldil	l%halt_sim,arg0
	ldi	3,arg1
	stw	arg1,r%halt_sim(arg0)

	ldi	0,kbank
	copy	ret0,pc
	b	dispatch
	depi	1,29,2,psr		;ints masked, decimal off

brk_testing_SYM
	extru	ret0,31,8,ret0
	addi	-2,pc,pc
	CYCLES_PLUS_2
	b	dispatch_done
	depi	RET_BREAK,3,4,ret0

#else
	CYCLES7();
	FINISH(RET_BREAK, arg & 0xff);
#endif

inst01_SYM		/*  ORA (Dloc,X) */
/*  called with arg = val to ORA in */
	GET_DLOC_X_IND_RD();
	ORA_INST();

inst02_SYM		/*  COP */
#ifdef ASM
	ldil	l%g_num_cop,arg1
	addi	2,pc,pc
	ldw	r%g_num_cop(arg1),arg2
	extru	pc,31,16,arg0
	addi	1,arg2,arg2
	bb,>=	psr,23,cop_native_SYM
	stw	arg2,r%g_num_cop(arg1)

	bl	push_16,link
	nop

	bl	push_8,link
	extru	psr,31,8,arg0

	ldil	l%0xfff4,arg0
	bl	get_mem_long_16,link
	ldo	r%0xfff4(arg0),arg0

	ldi	0,kbank
	ldi	0,dbank			;clear dbank in emul mode
	copy	ret0,pc
	depi	1,29,2,psr		;ints masked, decimal off

	ldil	l%halt_sim,arg0
	ldi	3,arg1
	b	dispatch
	stw	arg1,r%halt_sim(arg0)

cop_native_SYM
	stw	arg0,STACK_SAVE_COP_ARG0(sp)
	bl	push_8,link
	copy	kbank,arg0

	bl	push_16,link
	ldw	STACK_SAVE_COP_ARG0(sp),arg0

	bl	push_8,link
	extru	psr,31,8,arg0

	ldil	l%0xfff4,arg0
	bl	get_mem_long_16,link
	ldo	r%0xfff4(arg0),arg0

	ldi	0,kbank
	copy	ret0,pc
	b	dispatch
	depi	1,29,2,psr		;ints masked, decimal off


#else
	CYCLES2();
	FINISH(RET_COP, arg & 0xff);
#endif

inst03_SYM		/*  ORA Disp8,S */
	GET_DISP8_S_RD();
	ORA_INST();

inst04_SYM		/*  TSB Dloc */
	GET_DLOC_RD();
	TSB_INST();

inst05_SYM		/*  ORA Dloc */
	GET_DLOC_RD();
	ORA_INST();

inst06_SYM		/*  ASL Dloc */
	GET_DLOC_RD();
	ASL_INST();

inst07_SYM		/*  ORA [Dloc] */
	GET_DLOC_L_IND_RD();
	ORA_INST();

inst08_SYM		/*  PHP */
#ifdef ASM
	dep	neg,24,1,psr
	ldil	l%dispatch,link
	addi	1,pc,pc
	depi	0,30,1,psr
	comiclr,<> 0,zero,0
	depi	1,30,1,psr
	ldo	r%dispatch(link),link
	b	push_8
	extru	psr,31,8,arg0
#else
	C_PHP();
#endif

inst09_SYM		/*  ORA #imm */
	GET_IMM_MEM();
	ORA_INST();

inst0a_SYM		/*  ASL a */
#ifdef ASM
# ifdef ACC8
	ldi	0xff,scratch1
	sh1add	acc,0,scratch3
	addi	1,pc,pc
	extru	scratch3,24,1,neg
	and	scratch3,scratch1,zero
	extru	scratch3,23,1,scratch2
	dep	zero,31,8,acc
	b	dispatch
	dep	scratch2,31,1,psr		/* set carry */
# else
	zdepi	-1,31,16,scratch1
	sh1add	acc,0,scratch3
	addi	1,pc,pc
	extru	scratch3,16,1,neg
	and	scratch3,scratch1,zero
	extru	scratch3,15,1,scratch2
	dep	scratch2,31,1,psr		/*  set carry */
	b	dispatch
	dep	zero,31,16,acc
# endif
#else
	C_ASL_A();
#endif

inst0b_SYM		/*  PHD */
#ifdef ASM
	ldil	l%dispatch,link
	extru	direct,31,16,arg0
	addi	1,pc,pc
	b	push_16_unsafe
	ldo	r%dispatch(link),link
#else
	C_PHD();
#endif

inst0c_SYM		/*  TSB abs */
	GET_ABS_RD();
	TSB_INST();

inst0d_SYM		/*  ORA abs */
	GET_ABS_RD();
	ORA_INST();

inst0e_SYM		/*  ASL abs */
	GET_ABS_RD();
	ASL_INST();

inst0f_SYM		/*  ORA long */
	GET_LONG_RD();
	ORA_INST();


inst10_SYM		/*  BPL disp8 */
#ifdef ASM
	COND_BR1
	comib,<> 0,neg,inst10_2_SYM
	COND_BR2

inst10_2_SYM
	COND_BR_UNTAKEN
#else
	C_BPL_DISP8();
#endif

inst11_SYM		/*  ORA (Dloc),y */
	GET_DLOC_IND_Y_RD();
	ORA_INST();

inst12_SYM		/*  ORA (Dloc) */
	GET_DLOC_IND_RD();
	ORA_INST();

inst13_SYM		/*  ORA (Disp8,s),y */
	GET_DISP8_S_IND_Y_RD();
	ORA_INST();

inst14_SYM		/*  TRB Dloc */
	GET_DLOC_RD();
	TRB_INST();

inst15_SYM		/*  ORA Dloc,x */
	GET_DLOC_X_RD();
	ORA_INST();

inst16_SYM		/*  ASL Dloc,X */
	GET_DLOC_X_RD();
	ASL_INST();

inst17_SYM		/*  ORA [Dloc],Y */
	GET_DLOC_L_IND_Y_RD();
	ORA_INST();

inst18_SYM		/*  CLC */
#ifdef ASM
	depi	0,31,1,psr		/* clear carry */
	b	dispatch
	addi	1,pc,pc
#else
	C_CLC();
#endif

inst19_SYM		/*  ORA abs,y */
	GET_ABS_Y_RD();
	ORA_INST();

inst1a_SYM		/*  INC a */
#ifdef ASM
# ifdef ACC8
	ldi	0xff,scratch2
	addi	1,acc,scratch1
	extru	scratch1,24,1,neg
	addi	1,pc,pc
	extru	scratch1,31,8,zero
	b	dispatch
	dep	zero,31,8,acc
# else
	zdepi	-1,31,16,scratch2
	addi	1,acc,scratch1
	extru	scratch1,16,1,neg
	addi	1,pc,pc
	extru	scratch1,31,16,zero
	b	dispatch
	dep	zero,31,16,acc
# endif
#else
	C_INC_A();
#endif

inst1b_SYM		/*  TCS */
#ifdef ASM
	copy	acc,stack
	extru,=	psr,23,1,0		/* in emulation mode, stack page 1 */
	depi	1,23,24,stack
	b	dispatch
	addi	1,pc,pc
#else
	C_TCS();
#endif

inst1c_SYM		/*  TRB Abs */
	GET_ABS_RD();
	TRB_INST();

inst1d_SYM		/*  ORA Abs,X */
	GET_ABS_X_RD();
	ORA_INST();

inst1e_SYM		/*  ASL Abs,X */
	GET_ABS_X_RD_WR();
	ASL_INST();

inst1f_SYM		/*  ORA Long,X */
	GET_LONG_X_RD();
	ORA_INST();


inst20_SYM		/*  JSR abs */
#ifdef ASM
	addi	2,pc,pc
	CYCLES_PLUS_2
	extru	pc,31,16,arg0
	ldil	l%dispatch,link
	extru	ret0,31,16,pc
	b	push_16
	ldo	r%dispatch(link),link
#else
	C_JSR_ABS();
#endif

inst21_SYM		/*  AND (Dloc,X) */
/*  called with arg = val to AND in */
	GET_DLOC_X_IND_RD();
	AND_INST();

inst22_SYM		/*  JSL Long */
#ifdef ASM
	addi	3,pc,pc
	CYCLES_PLUS_3
	extru	pc,31,16,arg0
	stw	ret0,STACK_SAVE_INSTR_TMP1(sp)
	extru	ret0,31,16,pc
	bl	push_24_unsafe,link
	dep	kbank,15,8,arg0

	b	change_kbank
	ldb	STACK_SAVE_INSTR_TMP1+1(sp),arg0	/* new val for kbank */
#else
	C_JSL_LONG();
#endif

inst23_SYM		/*  AND Disp8,S */
/*  called with arg = val to AND in */
	GET_DISP8_S_RD();
	AND_INST();

inst24_SYM		/*  BIT Dloc */
	GET_DLOC_RD();
	BIT_INST();

inst25_SYM		/*  AND Dloc */
/*  called with arg = val to AND in */
	GET_DLOC_RD();
	AND_INST();

inst26_SYM		/*  ROL Dloc */
	GET_DLOC_RD();
/*  save1 is now apple addr */
/*  ret0 is data */
	ROL_INST();

inst27_SYM		/*  AND [Dloc] */
	GET_DLOC_L_IND_RD();
	AND_INST();

inst28_SYM		/*  PLP */
#ifdef ASM
	bl	pull_8,link
	ldi	0,zero

	extru	psr,27,2,scratch2		/* save old x & m */
	dep	ret0,31,8,psr
	CYCLES_PLUS_1
	addi	1,pc,pc
	extru,<> ret0,30,1,0
	ldi	1,zero
	copy	scratch2,arg0
	b	update_system_state
	extru	ret0,24,1,neg
#else
	C_PLP();
#endif
	

inst29_SYM		/*  AND #imm */
	GET_IMM_MEM();
	AND_INST();

inst2a_SYM		/*  ROL a */
#ifdef ASM
# ifdef ACC8
	extru	psr,31,1,scratch2
	ldi	0xff,scratch1
	sh1add	acc,scratch2,scratch3
	addi	1,pc,pc
	extru	scratch3,24,1,neg
	and	scratch3,scratch1,zero
	extru	scratch3,23,1,scratch2
	dep	zero,31,8,acc
	b	dispatch
	dep	scratch2,31,1,psr		/* set carry */
# else
	extru	psr,31,1,scratch2
	addi	1,pc,pc
	sh1add	acc,scratch2,scratch3
	zdepi	-1,31,16,scratch1
	extru	scratch3,16,1,neg
	and	scratch3,scratch1,zero
	extru	scratch3,15,1,scratch2
	dep	scratch2,31,1,psr		/*  set carry */
	b	dispatch
	dep	zero,31,16,acc
# endif
#else
	C_ROL_A();
#endif

inst2b_SYM		/*  PLD */
#ifdef ASM
	addi	1,pc,pc
	bl	pull_16_unsafe,link
	CYCLES_PLUS_1
	extru	ret0,31,16,direct
	extru	ret0,16,1,neg
	b	dispatch
	copy	direct,zero
#else
	C_PLD();
#endif

inst2c_SYM		/*  BIT abs */
	GET_ABS_RD();
	BIT_INST();

inst2d_SYM		/*  AND abs */
	GET_ABS_RD();
	AND_INST();

inst2e_SYM		/*  ROL abs */
	GET_ABS_RD();
	ROL_INST();

inst2f_SYM		/*  AND long */
	GET_LONG_RD();
	AND_INST();


inst30_SYM		/*  BMI disp8 */
#ifdef ASM
	COND_BR1
	comib,= 0,neg,inst30_2_SYM
	COND_BR2

inst30_2_SYM
	COND_BR_UNTAKEN
#else
	C_BMI_DISP8();
#endif

inst31_SYM		/*  AND (Dloc),y */
	GET_DLOC_IND_Y_RD();
	AND_INST();

inst32_SYM		/*  AND (Dloc) */
	GET_DLOC_IND_RD();
	AND_INST();

inst33_SYM		/*  AND (Disp8,s),y */
	GET_DISP8_S_IND_Y_RD();
	AND_INST();

inst34_SYM		/*  BIT Dloc,x */
	GET_DLOC_X_RD();
	BIT_INST();

inst35_SYM		/*  AND Dloc,x */
	GET_DLOC_X_RD();
	AND_INST();

inst36_SYM		/*  ROL Dloc,X */
	GET_DLOC_X_RD();
	ROL_INST();

inst37_SYM		/*  AND [Dloc],Y */
	GET_DLOC_L_IND_Y_RD();
	AND_INST();

inst38_SYM		/*  SEC */
#ifdef ASM
	depi	1,31,1,psr		/* set carry */
	b	dispatch
	addi	1,pc,pc
#else
	C_SEC();
#endif

inst39_SYM		/*  AND abs,y */
	GET_ABS_Y_RD();
	AND_INST();

inst3a_SYM		/*  DEC a */
#ifdef ASM
# ifdef ACC8
	addi	-1,acc,scratch1
	extru	scratch1,24,1,neg
	addi	1,pc,pc
	extru	scratch1,31,8,zero
	b	dispatch
	dep	zero,31,8,acc
# else
	addi	-1,acc,scratch1
	extru	scratch1,16,1,neg
	addi	1,pc,pc
	extru	scratch1,31,16,zero
	b	dispatch
	dep	zero,31,16,acc
# endif
#else
	C_DEC_A();
#endif

inst3b_SYM		/*  TSC */
/*  set N,Z according to 16 bit acc */
#ifdef ASM
	copy	stack,acc
	extru	stack,16,1,neg
	addi	1,pc,pc
	b	dispatch
	extru	acc,31,16,zero
#else
	C_TSC();
#endif

inst3c_SYM		/*  BIT Abs,x */
	GET_ABS_X_RD();
	BIT_INST();

inst3d_SYM		/*  AND Abs,X */
	GET_ABS_X_RD();
	AND_INST();

inst3e_SYM		/*  ROL Abs,X */
	GET_ABS_X_RD_WR();
	ROL_INST();

inst3f_SYM		/*  AND Long,X */
	GET_LONG_X_RD();
	AND_INST();


inst40_SYM		/*  RTI */
#ifdef ASM
	bb,>=	psr,23,rti_native_SYM
	CYCLES_PLUS_1
/*  emulation */
	bl	pull_24,link
	ldi	0,zero

	extru	psr,27,2,scratch2
	extru	ret0,23,16,pc
	copy	scratch2,arg0
	extru,<> ret0,30,1,0
	ldi	1,zero
	dep	ret0,31,8,psr

	b	update_system_state
	extru	ret0,24,1,neg

rti_native_SYM
	bl	pull_8,link
	ldi	0,zero

	copy	ret0,scratch1
	extru	ret0,24,1,neg
	dep	ret0,31,8,scratch1
	bl	pull_24,link
	stw	scratch1,STACK_SAVE_INSTR_TMP1(sp)

	extru	psr,27,2,scratch2
	ldw	STACK_SAVE_INSTR_TMP1(sp),psr
	extru	ret0,31,16,pc
	extru,<> psr,30,1,0
	ldi	1,zero

	extru	ret0,15,8,kbank
	b	update_system_state_and_change_kbank
	copy	scratch2,arg0
#else
	C_RTI();
#endif


inst41_SYM		/*  EOR (Dloc,X) */
/*  called with arg = val to EOR in */
	GET_DLOC_X_IND_RD();
	EOR_INST();

inst42_SYM		/*  WDM */
#ifdef ASM
	extru	ret0,31,8,ret0
	b	dispatch_done
	depi	RET_WDM,3,4,ret0
#else
	CYCLES7();
	FINISH(RET_WDM, arg & 0xff);
#endif

inst43_SYM		/*  EOR Disp8,S */
/*  called with arg = val to EOR in */
	GET_DISP8_S_RD();
	EOR_INST();

inst44_SYM		/*  MVP */
#ifdef ASM
	extru	ret0,23,8,scratch2		/* src bank */
	bb,<	psr,23,inst44_notnat_SYM
	stw	scratch2,STACK_SRC_BANK(sp)
	bb,<	psr,27,inst44_notnat_SYM
	extru	ret0,31,8,dbank			/* dest bank */

inst44_loop_SYM
	CYCLES_PLUS_3
	ldw	STACK_SRC_BANK(sp),scratch2
	fcmp,<,sgl fcycles,fcycles_stop
	ftest
	b	inst44_out_of_time_SYM
	copy	xreg,arg0

	CYCLES_PLUS_2
	bl	get_mem_long_8,link
	dep	scratch2,15,8,arg0
/*  got byte */
	copy	ret0,arg1
	copy	yreg,arg0
	bl	set_mem_long_8,link
	dep	dbank,15,8,arg0
/*  wrote byte, dec acc */
	addi	-1,xreg,xreg
	zdepi	-1,31,16,scratch2
	addi	-1,yreg,yreg
	addi	-1,acc,acc
	and	xreg,scratch2,xreg
	extrs	acc,31,16,scratch1
	and	yreg,scratch2,yreg
	comib,<> -1,scratch1,inst44_loop_SYM
	and	acc,scratch2,acc

/*  get here if done */
	b	dispatch
	addi	3,pc,pc

inst44_notnat_SYM
	extru	ret0,31,16,ret0
	CYCLES_PLUS_3
	depi	RET_MVP,3,4,ret0
	b	dispatch_done
	CYCLES_PLUS_2

inst44_out_of_time_SYM
/*  cycle have gone positive, just get out, do not update pc */
	b,n	dispatch
#else
	C_MVP();
#endif


inst45_SYM		/*  EOR Dloc */
/*  called with arg = val to EOR in */
	GET_DLOC_RD();
	EOR_INST();

inst46_SYM		/*  LSR Dloc */
	GET_DLOC_RD();
/*  save1 is now apple addr */
/*  ret0 is data */
	LSR_INST();

inst47_SYM		/*  EOR [Dloc] */
	GET_DLOC_L_IND_RD();
	EOR_INST();

inst48_SYM		/*  PHA */
#ifdef ASM
# ifdef ACC8
	addi	1,pc,pc
	ldil	l%dispatch,link
	extru	acc,31,8,arg0
	b	push_8
	ldo	r%dispatch(link),link
# else
	addi	1,pc,pc
	ldil	l%dispatch,link
	extru	acc,31,16,arg0
	b	push_16
	ldo	r%dispatch(link),link
# endif
#else
	C_PHA();
#endif

inst49_SYM		/*  EOR #imm */
	GET_IMM_MEM();
	EOR_INST();

inst4a_SYM		/*  LSR a */
#ifdef ASM
# ifdef ACC8
	extru	acc,31,1,scratch2
	addi	1,pc,pc
	extru	acc,30,7,zero
	ldi	0,neg
	dep	scratch2,31,1,psr		/* set carry */
	b	dispatch
	dep	zero,31,8,acc
# else
	extru	acc,31,1,scratch2
	addi	1,pc,pc
	extru	acc,30,15,zero
	ldi	0,neg
	dep	scratch2,31,1,psr		/*  set carry */
	b	dispatch
	dep	zero,31,16,acc
# endif
#else
	C_LSR_A();
#endif

inst4b_SYM		/*  PHK */
#ifdef ASM
	ldil	l%dispatch,link
	extru	kbank,31,8,arg0
	addi	1,pc,pc
	b	push_8
	ldo	r%dispatch(link),link
#else
	C_PHK();
#endif

inst4c_SYM		/*  JMP abs */
#ifdef ASM
	extru	ret0,31,16,pc
	b	dispatch
	CYCLES_PLUS_1
#else
	C_JMP_ABS();
#endif
	

inst4d_SYM		/*  EOR abs */
	GET_ABS_RD();
	EOR_INST();

inst4e_SYM		/*  LSR abs */
	GET_ABS_RD();
	LSR_INST();

inst4f_SYM		/*  EOR long */
	GET_LONG_RD();
	EOR_INST();


inst50_SYM		/*  BVC disp8 */
#ifdef ASM
	COND_BR1
	bb,<	psr,25,inst50_2_SYM
	COND_BR2

inst50_2_SYM
	COND_BR_UNTAKEN

#else
	C_BVC_DISP8();
#endif

inst51_SYM		/*  EOR (Dloc),y */
	GET_DLOC_IND_Y_RD();
	EOR_INST();

inst52_SYM		/*  EOR (Dloc) */
	GET_DLOC_IND_RD();
	EOR_INST();

inst53_SYM		/*  EOR (Disp8,s),y */
	GET_DISP8_S_IND_Y_RD();
	EOR_INST();

inst54_SYM		/*  MVN  */
#ifdef ASM
	extru	ret0,23,8,scratch2		/* src bank */
	bb,<	psr,23,inst54_notnat_SYM
	stw	scratch2,STACK_SRC_BANK(sp)
	bb,<	psr,27,inst54_notnat_SYM
	extru	ret0,31,8,dbank			/* dest bank */

/*  even in 8bit acc mode, use 16-bit accumulator! */

inst54_loop_SYM
	CYCLES_PLUS_3
	ldw	STACK_SRC_BANK(sp),scratch2
	fcmp,<,sgl fcycles,fcycles_stop
	ftest
	b	inst54_out_of_time_SYM
	copy	xreg,arg0

	CYCLES_PLUS_2
	bl	get_mem_long_8,link
	dep	scratch2,15,8,arg0
/*  got byte */
	copy	ret0,arg1
	copy	yreg,arg0
	bl	set_mem_long_8,link
	dep	dbank,15,8,arg0
/*  wrote byte, dec acc */
	addi	1,xreg,xreg
	zdepi	-1,31,16,scratch2
	addi	1,yreg,yreg
	addi	-1,acc,acc
	and	xreg,scratch2,xreg
	extrs	acc,31,16,scratch1
	and	yreg,scratch2,yreg
	comib,<> -1,scratch1,inst54_loop_SYM
	and	acc,scratch2,acc

/*  get here if done */
	b	dispatch
	addi	3,pc,pc

inst54_out_of_time_SYM
/*  cycle have gone positive, just get out, don't update pc */
	b,n	dispatch

inst54_notnat_SYM
	extru	ret0,31,16,ret0
	CYCLES_PLUS_3
	depi	RET_MVN,3,4,ret0
	b	dispatch_done
	CYCLES_PLUS_3
#else
	C_MVN();
#endif

inst55_SYM		/*  EOR Dloc,x */
	GET_DLOC_X_RD();
	EOR_INST();

inst56_SYM		/*  LSR Dloc,X */
	GET_DLOC_X_RD();
	LSR_INST();

inst57_SYM		/*  EOR [Dloc],Y */
	GET_DLOC_L_IND_Y_RD();
	EOR_INST();

inst58_SYM		/*  CLI */
#ifdef ASM
	/* FIX THIS to halt_sim=2 if any irqs pending */
	depi	0,29,1,psr		/* clear int disable */
	b	dispatch
	addi	1,pc,pc
#else
	C_CLI();
#endif

inst59_SYM		/*  EOR abs,y */
	GET_ABS_Y_RD();
	EOR_INST();

inst5a_SYM		/*  PHY */
#ifdef ASM
	addi	1,pc,pc
	ldil	l%dispatch,link
	bb,>=	psr,27,phy_16_SYM
	ldo	r%dispatch(link),link

	b	push_8
	copy	yreg,arg0

phy_16_SYM
	b	push_16
	copy	yreg,arg0
#else
	C_PHY();
#endif

inst5b_SYM		/*  TCD */
#ifdef ASM
	extru	acc,31,16,direct
	addi	1,pc,pc
	copy	acc,zero
	b	dispatch
	extru	acc,16,1,neg
#else
	C_TCD();
#endif

inst5c_SYM		/*  JMP Long */
#ifdef ASM
	extru	ret0,31,16,pc
	CYCLES_PLUS_1
	b	change_kbank
	extru	ret0,15,8,arg0
#else
	C_JMP_LONG();
#endif

inst5d_SYM		/*  EOR Abs,X */
	GET_ABS_X_RD();
	EOR_INST();

inst5e_SYM		/*  LSR Abs,X */
	GET_ABS_X_RD_WR();
	LSR_INST();

inst5f_SYM		/*  EOR Long,X */
	GET_LONG_X_RD();
	EOR_INST();


inst60_SYM		/*  RTS */
#ifdef ASM
	bl	pull_16,link
	CYCLES_PLUS_2
/*  ret0 is new pc-1 */
	b	dispatch
	addi	1,ret0,pc
#else
	C_RTS();
#endif


inst61_SYM		/*  ADC (Dloc,X) */
/*  called with arg = val to ADC in */
	GET_DLOC_X_IND_RD();
	ADC_INST();

inst62_SYM		/*  PER */
#ifdef ASM
	addi	3,pc,pc
	CYCLES_PLUS_2
	add	pc,ret0,arg0
	ldil	l%dispatch,link
	extru	arg0,31,16,arg0
	b	push_16_unsafe
	ldo	r%dispatch(link),link
#else
	C_PER();
#endif

inst63_SYM		/*  ADC Disp8,S */
/*  called with arg = val to ADC in */
	GET_DISP8_S_RD();
	ADC_INST();

inst64_SYM		/*  STZ Dloc */
	GET_DLOC_ADDR();
	STZ_INST();

inst65_SYM		/*  ADC Dloc */
/*  called with arg = val to ADC in */
	GET_DLOC_RD();
	ADC_INST();

inst66_SYM		/*  ROR Dloc */
	GET_DLOC_RD();
/*  save1 is now apple addr */
/*  ret0 is data */
	ROR_INST();

inst67_SYM		/*  ADC [Dloc] */
	GET_DLOC_L_IND_RD();
	ADC_INST();

inst68_SYM		/*  PLA */
#ifdef ASM
# ifdef ACC8
	addi	1,pc,pc
	bl	pull_8,link
	CYCLES_PLUS_1
	extru	ret0,31,8,zero
	extru	ret0,24,1,neg
	b	dispatch
	dep	ret0,31,8,acc
# else
	addi	1,pc,pc
	bl	pull_16,link
	CYCLES_PLUS_1

	extru	ret0,31,16,zero
	extru	ret0,16,1,neg
	b	dispatch
	extru	ret0,31,16,acc
# endif
#else
	C_PLA();
#endif


inst69_SYM		/*  ADC #imm */
	GET_IMM_MEM();
	ADC_INST();

inst6a_SYM		/*  ROR a */
#ifdef ASM
# ifdef ACC8
	extru	psr,31,1,neg
	addi	1,pc,pc
	extru	acc,30,7,zero
	dep	neg,24,1,zero
	dep	acc,31,1,psr			/* set carry */
	b	dispatch
	dep	zero,31,8,acc
# else
	extru	psr,31,1,neg
	addi	1,pc,pc
	extru	acc,30,15,zero
	dep	neg,16,1,zero
	dep	acc,31,1,psr		/*  set carry */
	b	dispatch
	dep	zero,31,16,acc
# endif
#else
	C_ROR_A();
#endif

inst6b_SYM		/*  RTL */
#ifdef ASM
	bl	pull_24,link
	CYCLES_PLUS_1
/*  ret0 is new pc-1 */
	extru	ret0,15,8,arg0
	b	change_kbank
	addi	1,ret0,pc
#else
	C_RTL();
#endif

inst6c_SYM		/*  JMP (abs) */
#ifdef ASM
	extru	ret0,31,16,arg0
	bl	get_mem_long_16,link
	CYCLES_PLUS_1
/*  ret0 is addr to jump to */
	b	dispatch
	extru	ret0,31,16,pc
#else
	C_JMP_IND_ABS();
#endif

inst6d_SYM		/*  ADC abs */
	GET_ABS_RD();
	ADC_INST();

inst6e_SYM		/*  ROR abs */
	GET_ABS_RD();
	ROR_INST();

inst6f_SYM		/*  ADC long */
	GET_LONG_RD();
	ADC_INST();


inst70_SYM		/*  BVS disp8 */
#ifdef ASM
	COND_BR1
	bb,>=	psr,25,inst70_2_SYM
	COND_BR2

inst70_2_SYM
	COND_BR_UNTAKEN
#else
	C_BVS_DISP8();
#endif

inst71_SYM		/*  ADC (Dloc),y */
	GET_DLOC_IND_Y_RD();
	ADC_INST();

inst72_SYM		/*  ADC (Dloc) */
	GET_DLOC_IND_RD();
	ADC_INST();

inst73_SYM		/*  ADC (Disp8,s),y */
	GET_DISP8_S_IND_Y_RD();
	ADC_INST();

inst74_SYM		/*  STZ Dloc,x */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_DLOC_X_WR();
	STZ_INST();
#else
	C_STZ_DLOC_X();
#endif

inst75_SYM		/*  ADC Dloc,x */
	GET_DLOC_X_RD();
	ADC_INST();

inst76_SYM		/*  ROR Dloc,X */
	GET_DLOC_X_RD();
	ROR_INST();

inst77_SYM		/*  ADC [Dloc],Y */
	GET_DLOC_L_IND_Y_RD();
	ADC_INST();

inst78_SYM		/*  SEI */
#ifdef ASM
	depi	1,29,1,psr		/* set int disable */
	b	dispatch
	addi	1,pc,pc
#else
	C_SEI();
#endif

inst79_SYM		/*  ADC abs,y */
	GET_ABS_Y_RD();
	ADC_INST();

inst7a_SYM		/*  PLY */
#ifdef ASM
	bb,>=	psr,27,inst7a_16bit_SYM
	addi	1,pc,pc

	bl	pull_8,link
	CYCLES_PLUS_1

	extru	ret0,31,8,zero
	extru	ret0,24,1,neg
	b	dispatch
	copy	zero,yreg

inst7a_16bit_SYM
	bl	pull_16,link
	CYCLES_PLUS_1

	extru	ret0,31,16,zero
	extru	ret0,16,1,neg
	b	dispatch
	copy	zero,yreg

#else
	C_PLY();
#endif

inst7b_SYM		/*  TDC */
#ifdef ASM
	extru	direct,31,16,zero
	copy	direct,acc
	extru	direct,16,1,neg
	b	dispatch
	addi	1,pc,pc
#else
	C_TDC();
#endif

inst7c_SYM		/*  JMP (Abs,x) */
/*  is this right?  Should xreg allow wrapping into next bank? */
#ifdef ASM
	dep	kbank,15,16,ret0
	CYCLES_PLUS_2
	add	xreg,ret0,arg0
	bl	get_mem_long_16,link
	extru	arg0,31,24,arg0
	b	dispatch
	extru	ret0,31,16,pc
#else
	C_JMP_IND_ABS_X();
#endif

inst7d_SYM		/*  ADC Abs,X */
	GET_ABS_X_RD();
	ADC_INST();

inst7e_SYM		/*  ROR Abs,X */
	GET_ABS_X_RD_WR();
	ROR_INST();

inst7f_SYM		/*  ADC Long,X */
	GET_LONG_X_RD();
	ADC_INST();


inst80_SYM		/*  BRA */
#ifdef ASM
	COND_BR1
	COND_BR2
#else
	C_BRA_DISP8();
#endif


inst81_SYM		/*  STA (Dloc,X) */
	GET_DLOC_X_IND_ADDR();
	STA_INST();

inst82_SYM		/*  BRL disp16 */
#ifdef ASM
	add	ret0,pc,pc
	CYCLES_PLUS_1
	b	dispatch
	addi	3,pc,pc				/*  yup, this is needed */
#else
	C_BRL_DISP16();
#endif

inst83_SYM		/*  STA Disp8,S */
	GET_DISP8_S_ADDR();
	STA_INST();

inst84_SYM		/*  STY Dloc */
	GET_DLOC_ADDR();
	STY_INST();


inst85_SYM		/*  STA Dloc */
	GET_DLOC_ADDR();
	STA_INST();

inst86_SYM		/*  STX Dloc */
	GET_DLOC_ADDR();
	STX_INST();


inst87_SYM		/*  STA [Dloc] */
	GET_DLOC_L_IND_ADDR();
	STA_INST();

inst88_SYM		/*  DEY */
#ifdef ASM
	addi	-1,yreg,yreg
	bb,<	psr,27,inst88_8bit_SYM
	addi	1,pc,pc
/*  16 bit */
	extru	yreg,31,16,zero
	extru	yreg,16,1,neg
	b	dispatch
	copy	zero,yreg

inst88_8bit_SYM
	extru	yreg,31,8,zero
	extru	yreg,24,1,neg
	b	dispatch
	copy	zero,yreg
#else
	C_DEY();
#endif

inst89_SYM		/*  BIT #imm */
#ifdef ASM
	GET_IMM_MEM();
# ifdef ACC8
/* Immediate BIT does not set condition flags */
	and	acc,ret0,zero
	b	dispatch
	extru	zero,31,8,zero
# else
	and	acc,ret0,zero
	b	dispatch
	extru	zero,31,16,zero
# endif
#else
	C_BIT_IMM();
#endif

inst8a_SYM		/*  TXA */
#ifdef ASM
# ifdef ACC8
	extru	xreg,31,8,zero
	addi	1,pc,pc
	extru	xreg,24,1,neg
	b	dispatch
	dep	zero,31,8,acc
# else
	extru	xreg,31,16,zero
	addi	1,pc,pc
	extru	xreg,16,1,neg
	b	dispatch
	zdep	zero,31,16,acc
# endif
#else
	C_TXA();
#endif

inst8b_SYM		/*  PHB */
#ifdef ASM
	ldil	l%dispatch,link
	extru	dbank,31,8,arg0
	addi	1,pc,pc
	b	push_8
	ldo	r%dispatch(link),link
#else
	C_PHB();
#endif

inst8c_SYM		/*  STY abs */
	GET_ABS_ADDR();
	STY_INST();

inst8d_SYM		/*  STA abs */
	GET_ABS_ADDR();
	STA_INST();

inst8e_SYM		/*  STX abs */
	GET_ABS_ADDR();
	STX_INST();


inst8f_SYM		/*  STA long */
	GET_LONG_ADDR();
	STA_INST();


inst90_SYM		/*  BCC disp8 */
#ifdef ASM
	COND_BR1
	bb,<	psr,31,inst90_2_SYM
	COND_BR2

inst90_2_SYM
	COND_BR_UNTAKEN
#else
	C_BCC_DISP8();
#endif


inst91_SYM		/*  STA (Dloc),y */
	GET_DLOC_IND_Y_ADDR_FOR_WR();
	STA_INST();

inst92_SYM		/*  STA (Dloc) */
	GET_DLOC_IND_ADDR();
	STA_INST();

inst93_SYM		/*  STA (Disp8,s),y */
	GET_DISP8_S_IND_Y_ADDR();
	STA_INST();

inst94_SYM		/*  STY Dloc,x */
	GET_DLOC_X_ADDR();
	STY_INST();

inst95_SYM		/*  STA Dloc,x */
	GET_DLOC_X_ADDR();
	STA_INST();

inst96_SYM		/*  STX Dloc,Y */
	GET_DLOC_Y_ADDR();
	STX_INST();

inst97_SYM		/*  STA [Dloc],Y */
	GET_DLOC_L_IND_Y_ADDR();
	STA_INST();

inst98_SYM		/*  TYA */
#ifdef ASM
# ifdef ACC8
	extru	yreg,31,8,zero
	addi	1,pc,pc
	extru	yreg,24,1,neg
	b	dispatch
	dep	zero,31,8,acc
# else
	extru	yreg,31,16,zero
	addi	1,pc,pc
	extru	yreg,16,1,neg
	b	dispatch
	zdep	zero,31,16,acc
# endif
#else
	C_TYA();
#endif

inst99_SYM		/*  STA abs,y */
#ifdef ASM
	GET_ABS_INDEX_ADDR_FOR_WR(yreg)
	STA_INST();
#else
	GET_ABS_Y_ADDR_FOR_WR();
	STA_INST();
#endif

inst9a_SYM		/*  TXS */
#ifdef ASM
	copy	xreg,stack
	extru,=	psr,23,1,0
	depi	1,23,24,stack
	b	dispatch
	addi	1,pc,pc
#else
	C_TXS();
#endif


inst9b_SYM		/*  TXY */
#ifdef ASM
	extru	xreg,24,1,neg
	addi	1,pc,pc
	extru,<> psr,27,1,0		;skip next if 8bit
	extru	xreg,16,1,neg
	copy	xreg,yreg
	b	dispatch
	copy	xreg,zero
#else
	C_TXY();
#endif


inst9c_SYM		/*  STZ Abs */
	GET_ABS_ADDR();
	STZ_INST();

inst9d_SYM		/*  STA Abs,X */
	GET_ABS_INDEX_ADDR_FOR_WR(xreg);
	STA_INST();

inst9e_SYM		/*  STZ Abs,X */
	GET_ABS_INDEX_ADDR_FOR_WR(xreg);
	STZ_INST();

inst9f_SYM		/*  STA Long,X */
	GET_LONG_X_ADDR_FOR_WR();
	STA_INST();


insta0_SYM		/*  LDY #imm */
#ifdef ASM
	extru	ret0,31,8,zero
	bb,>=	psr,27,insta0_16bit_SYM
	addi	2,pc,pc

	extru	ret0,24,1,neg
	b	dispatch
	copy	zero,yreg
insta0_16bit_SYM
	extru	ret0,31,16,zero
	addi	1,pc,pc
	extru	ret0,16,1,neg
	CYCLES_PLUS_1
	b	dispatch
	copy	zero,yreg
#else
	C_LDY_IMM();
#endif


insta1_SYM		/*  LDA (Dloc,X) */
/*  called with arg = val to LDA in */
	GET_DLOC_X_IND_RD();
	LDA_INST();

insta2_SYM		/*  LDX #imm */
#ifdef ASM
	extru	ret0,31,8,zero
	bb,>=	psr,27,insta2_16bit_SYM
	addi	2,pc,pc

	extru	ret0,24,1,neg
	b	dispatch
	copy	zero,xreg
insta2_16bit_SYM
	extru	ret0,31,16,zero
	addi	1,pc,pc
	extru	ret0,16,1,neg
	CYCLES_PLUS_1
	b	dispatch
	copy	zero,xreg
#else
	C_LDX_IMM();
#endif

insta3_SYM		/*  LDA Disp8,S */
/*  called with arg = val to LDA in */
	GET_DISP8_S_RD();
	LDA_INST();

insta4_SYM		/*  LDY Dloc */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_DLOC_WR()
	b	get_yreg_from_mem
	nop
#else
	C_LDY_DLOC();
#endif

insta5_SYM		/*  LDA Dloc */
/*  called with arg = val to LDA in */
	GET_DLOC_RD();
	LDA_INST();

insta6_SYM		/*  LDX Dloc */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_DLOC_WR()
	b	get_xreg_from_mem
	nop
#else
	C_LDX_DLOC();
#endif

insta7_SYM		/*  LDA [Dloc] */
	GET_DLOC_L_IND_RD();
	LDA_INST();

insta8_SYM		/*  TAY */
#ifdef ASM
	addi	1,pc,pc
	bb,>=	psr,27,insta8_16bit_SYM
	extru	acc,31,8,zero

	extru	acc,24,1,neg
	b	dispatch
	copy	zero,yreg

insta8_16bit_SYM
	extru	acc,31,16,zero
	extru	acc,16,1,neg
	b	dispatch
	copy	zero,yreg
#else
	C_TAY();
#endif

insta9_SYM		/*  LDA #imm */
	GET_IMM_MEM();
	LDA_INST();

instaa_SYM		/*  TAX */
#ifdef ASM
	addi	1,pc,pc
	bb,>=	psr,27,instaa_16bit_SYM
	extru	acc,31,8,zero

	extru	acc,24,1,neg
	b	dispatch
	copy	zero,xreg

instaa_16bit_SYM
	extru	acc,31,16,zero
	extru	acc,16,1,neg
	b	dispatch
	copy	zero,xreg
#else
	C_TAX();
#endif

instab_SYM		/*  PLB */
#ifdef ASM
	addi	1,pc,pc
	bl	pull_8,link
	CYCLES_PLUS_1

	extru	ret0,31,8,zero
	extru	ret0,24,1,neg
	b	dispatch
	copy	zero,dbank
#else
	C_PLB();
#endif

instac_SYM		/*  LDY abs */
#ifdef ASM
	GET_ABS_ADDR()
	b	get_yreg_from_mem
	nop
#else
	C_LDY_ABS();
#endif


instad_SYM		/*  LDA abs */
	GET_ABS_RD();
	LDA_INST();

instae_SYM		/*  LDX abs */
#ifdef ASM
	GET_ABS_ADDR()
	b	get_xreg_from_mem
	nop
#else
	C_LDX_ABS();
#endif

instaf_SYM		/*  LDA long */
	GET_LONG_RD();
	LDA_INST();


instb0_SYM		/*  BCS disp8 */
#ifdef ASM
	COND_BR1
	bb,>=	psr,31,instb0_2_SYM
	COND_BR2

instb0_2_SYM
	COND_BR_UNTAKEN
#else
	C_BCS_DISP8();
#endif

instb1_SYM		/*  LDA (Dloc),y */
	GET_DLOC_IND_Y_RD();
	LDA_INST();

instb2_SYM		/*  LDA (Dloc) */
	GET_DLOC_IND_RD();
	LDA_INST();

instb3_SYM		/*  LDA (Disp8,s),y */
	GET_DISP8_S_IND_Y_RD();
	LDA_INST();

instb4_SYM		/*  LDY Dloc,x */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_DLOC_X_WR();
	b	get_yreg_from_mem
	nop
#else
	C_LDY_DLOC_X();
#endif

instb5_SYM		/*  LDA Dloc,x */
	GET_DLOC_X_RD();
	LDA_INST();

instb6_SYM		/*  LDX Dloc,y */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_DLOC_Y_WR();
	b	get_xreg_from_mem
	nop
#else
	C_LDX_DLOC_Y();
#endif

instb7_SYM		/*  LDA [Dloc],Y */
	GET_DLOC_L_IND_Y_RD();
	LDA_INST();

instb8_SYM		/*  CLV */
#ifdef ASM
	depi	0,25,1,psr		/* clear overflow */
	b	dispatch
	addi	1,pc,pc
#else
	C_CLV();
#endif

instb9_SYM		/*  LDA abs,y */
	GET_ABS_Y_RD();
	LDA_INST();

instba_SYM		/*  TSX */
#ifdef ASM
	addi	1,pc,pc
	bb,>=	psr,27,instba_16bit_SYM
	extru	stack,31,8,zero

	extru	stack,24,1,neg
	b	dispatch
	copy	zero,xreg
instba_16bit_SYM
	copy	stack,zero
	extru	stack,16,1,neg
	b	dispatch
	copy	zero,xreg
#else
	C_TSX();
#endif

instbb_SYM		/*  TYX */
#ifdef ASM
	addi	1,pc,pc
	bb,>=	psr,27,instbb_16bit_SYM
	copy	yreg,xreg

/*  8 bit */
	extru	yreg,24,1,neg
	b	dispatch
	copy	yreg,zero
instbb_16bit_SYM
	extru	yreg,16,1,neg
	b	dispatch
	copy	yreg,zero
#else
	C_TYX();
#endif

instbc_SYM		/*  LDY Abs,X */
#ifdef ASM
	GET_ABS_INDEX_ADDR_FOR_RD(xreg)
	b	get_yreg_from_mem
	nop
#else
	C_LDY_ABS_X();
#endif

instbd_SYM		/*  LDA Abs,X */
	GET_ABS_X_RD();
	LDA_INST();

instbe_SYM		/*  LDX Abs,y */
#ifdef ASM
	GET_ABS_INDEX_ADDR_FOR_RD(yreg)
	b	get_xreg_from_mem
	nop
#else
	C_LDX_ABS_Y();
#endif

instbf_SYM		/*  LDA Long,X */
	GET_LONG_X_RD();
	LDA_INST();


instc0_SYM		/*  CPY #imm */
#ifdef ASM
	bb,>=	psr,27,instc0_16bit_SYM
	addi	2,pc,pc
	CMP_INDEX_REG_MEAT8(yreg)
instc0_16bit_SYM
	CYCLES_PLUS_1
	addi	1,pc,pc
	CMP_INDEX_REG_MEAT16(yreg)
#else
	C_CPY_IMM();
#endif


instc1_SYM		/*  CMP (Dloc,X) */
/*  called with arg = val to CMP in */
	GET_DLOC_X_IND_RD();
	CMP_INST();

instc2_SYM		/*  REP #imm */
#ifdef ASM
	extru	psr,27,2,arg0		/* save old x & m */
	addi	2,pc,pc
	dep	neg,24,1,psr
	CYCLES_PLUS_1
	depi	0,30,1,psr
	comiclr,<> 0,zero,0
	depi	1,30,1,psr
	andcm	psr,ret0,ret0
	ldi	0,zero
	extru,<> ret0,30,1,0
	ldi	1,zero
	dep	ret0,31,8,psr
	b	update_system_state
	extru	ret0,24,1,neg
#else
	C_REP_IMM();
#endif


instc3_SYM		/*  CMP Disp8,S */
/*  called with arg = val to CMP in */
	GET_DISP8_S_RD();
	CMP_INST();

instc4_SYM		/*  CPY Dloc */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_DLOC_WR()
	CMP_INDEX_REG_LOAD(instc4_16bit_SYM, yreg)
#else
	C_CPY_DLOC();
#endif


instc5_SYM		/*  CMP Dloc */
	GET_DLOC_RD();
	CMP_INST();

instc6_SYM		/*  DEC Dloc */
	GET_DLOC_RD();
	DEC_INST();

instc7_SYM		/*  CMP [Dloc] */
	GET_DLOC_L_IND_RD();
	CMP_INST();

instc8_SYM		/*  INY */
#ifdef ASM
	addi	1,pc,pc
	addi	1,yreg,yreg
	bb,>=	psr,27,instc8_16bit_SYM
	extru	yreg,31,8,zero

	extru	yreg,24,1,neg
	b	dispatch
	copy	zero,yreg

instc8_16bit_SYM
	extru	yreg,31,16,zero
	extru	yreg,16,1,neg
	b	dispatch
	copy	zero,yreg
#else
	C_INY();
#endif

instc9_SYM		/*  CMP #imm */
	GET_IMM_MEM();
	CMP_INST();

instca_SYM		/*  DEX */
#ifdef ASM
	addi	1,pc,pc
	addi	-1,xreg,xreg
	bb,>=	psr,27,instca_16bit_SYM
	extru	xreg,31,8,zero

	extru	xreg,24,1,neg
	b	dispatch
	copy	zero,xreg

instca_16bit_SYM
	extru	xreg,31,16,zero
	extru	xreg,16,1,neg
	b	dispatch
	copy	zero,xreg
#else
	C_DEX();
#endif

instcb_SYM		/*  WAI */
#ifdef ASM
	ldil	l%g_wait_pending,scratch1
	CYCLES_FINISH
	ldi	1,scratch2
	b	dispatch
	stw	scratch2,r%g_wait_pending(scratch1)
#else
	C_WAI();
#endif

instcc_SYM		/*  CPY abs */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_ABS_ADDR()
	CMP_INDEX_REG_LOAD(instcc_16bit_SYM, yreg)
#else
	C_CPY_ABS();
#endif




instcd_SYM		/*  CMP abs */
	GET_ABS_RD();
	CMP_INST();

instce_SYM		/*  DEC abs */
	GET_ABS_RD();
	DEC_INST();


instcf_SYM		/*  CMP long */
	GET_LONG_RD();
	CMP_INST();


instd0_SYM		/*  BNE disp8 */
#ifdef ASM
	COND_BR1
	comib,=	0,zero,instd0_2_SYM
	COND_BR2

instd0_2_SYM
	COND_BR_UNTAKEN
#else
	C_BNE_DISP8();
#endif

instd1_SYM		/*  CMP (Dloc),y */
	GET_DLOC_IND_Y_RD();
	CMP_INST();

instd2_SYM		/*  CMP (Dloc) */
	GET_DLOC_IND_RD();
	CMP_INST();

instd3_SYM		/*  CMP (Disp8,s),y */
	GET_DISP8_S_IND_Y_RD();
	CMP_INST();

instd4_SYM		/*  PEI Dloc */
#ifdef ASM
	GET_DLOC_ADDR()
	bl	get_mem_long_16,link
	CYCLES_PLUS_1

/*  push ret0 */
	extru	ret0,31,16,arg0
	ldil	l%dispatch,link
	b	push_16_unsafe
	ldo	r%dispatch(link),link
#else
	C_PEI_DLOC();
#endif

instd5_SYM		/*  CMP Dloc,x */
	GET_DLOC_X_RD();
	CMP_INST();

instd6_SYM		/*  DEC Dloc,x */
	GET_DLOC_X_RD();
	DEC_INST();

instd7_SYM		/*  CMP [Dloc],Y */
	GET_DLOC_L_IND_Y_RD();
	CMP_INST();

instd8_SYM		/*  CLD */
#ifdef ASM
	depi	0,28,1,psr		/* clear decimal */
	b	dispatch
	addi	1,pc,pc
#else
	C_CLD();
#endif

instd9_SYM		/*  CMP abs,y */
	GET_ABS_Y_RD();
	CMP_INST();

instda_SYM		/*  PHX */
#ifdef ASM
	addi	1,pc,pc
	bb,>=	psr,27,instda_16bit_SYM
	ldil	l%dispatch,link

	extru	xreg,31,8,arg0
	b	push_8
	ldo	r%dispatch(link),link

instda_16bit_SYM
	extru	xreg,31,16,arg0
	b	push_16
	ldo	r%dispatch(link),link
#else
	C_PHX();
#endif

instdb_SYM		/*  STP */
#ifdef ASM
	extru	ret0,31,8,ret0
	CYCLES_PLUS_1
	b	dispatch_done
	depi	RET_STP,3,4,ret0
#else
	C_STP();
#endif

instdc_SYM		/*  JML (Abs) */
#ifdef ASM
	CYCLES_PLUS_1
	bl	get_mem_long_24,link
	extru	ret0,31,16,arg0

	extru	ret0,31,16,pc
	b	change_kbank
	extru	ret0,15,8,arg0
#else
	C_JML_IND_ABS();
#endif

instdd_SYM		/*  CMP Abs,X */
	GET_ABS_X_RD();
	CMP_INST();

instde_SYM		/*  DEC Abs,X */
	GET_ABS_X_RD_WR();
	DEC_INST();

instdf_SYM		/*  CMP Long,X */
	GET_LONG_X_RD();
	CMP_INST();


inste0_SYM		/*  CPX #imm */
#ifdef ASM
	bb,>=	psr,27,inste0_16bit_SYM
	addi	2,pc,pc
	CMP_INDEX_REG_MEAT8(xreg)
inste0_16bit_SYM
	CYCLES_PLUS_1
	addi	1,pc,pc
	CMP_INDEX_REG_MEAT16(xreg)
#else
	C_CPX_IMM();
#endif


inste1_SYM		/*  SBC (Dloc,X) */
/*  called with arg = val to SBC in */
	GET_DLOC_X_IND_RD();
	SBC_INST();

inste2_SYM		/*  SEP #imm */
#ifdef ASM
	extru	psr,27,2,arg0		/* save old x & m */
	addi	2,pc,pc
	dep	neg,24,1,psr
	CYCLES_PLUS_1
	depi	0,30,1,psr
	comiclr,<> 0,zero,0
	depi	1,30,1,psr
	or	psr,ret0,ret0
	ldi	0,zero
	extru,<> ret0,30,1,0
	ldi	1,zero
	dep	ret0,31,8,psr
	b	update_system_state
	extru	ret0,24,1,neg
#else
	C_SEP_IMM();
#endif


inste3_SYM		/*  SBC Disp8,S */
/*  called with arg = val to SBC in */
	GET_DISP8_S_RD();
	SBC_INST();

inste4_SYM		/*  CPX Dloc */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_DLOC_WR()
	CMP_INDEX_REG_LOAD(inste4_16bit_SYM, xreg)
#else
	C_CPX_DLOC();
#endif


inste5_SYM		/*  SBC Dloc */
/*  called with arg = val to SBC in */
	GET_DLOC_RD();
	SBC_INST();

inste6_SYM		/*  INC Dloc */
	GET_DLOC_RD();
	INC_INST();

inste7_SYM		/*  SBC [Dloc] */
	GET_DLOC_L_IND_RD();
	SBC_INST();

inste8_SYM		/*  INX */
#ifdef ASM
	addi	1,pc,pc
	addi	1,xreg,xreg
	bb,>=	psr,27,inste8_16bit_SYM
	extru	xreg,31,8,zero

	extru	xreg,24,1,neg
	b	dispatch
	copy	zero,xreg

inste8_16bit_SYM
	extru	xreg,31,16,zero
	extru	xreg,16,1,neg
	b	dispatch
	copy	zero,xreg
#else
	C_INX();
#endif

inste9_SYM		/*  SBC #imm */
	GET_IMM_MEM();
	SBC_INST();

instea_SYM		/*  NOP */
#ifdef ASM
	b	dispatch
	addi	1,pc,pc
#else
	C_NOP();
#endif

insteb_SYM		/*  XBA */
#ifdef ASM
	extru	acc,16,1,neg		/* Z and N reflect status of low 8 */
	CYCLES_PLUS_1			/*   bits of final acc value! */
	copy	acc,scratch1		/* regardlessof ACC 8 or 16 bit */
	extru	acc,23,8,acc
	addi	1,pc,pc
	copy	acc,zero
	b	dispatch
	dep	scratch1,23,8,acc
#else
	C_XBA();
#endif

instec_SYM		/*  CPX abs */
#ifdef ASM
	extru	ret0,31,8,arg0
	GET_ABS_ADDR()
	CMP_INDEX_REG_LOAD(instec_16bit_SYM, xreg)
#else
	C_CPX_ABS();
#endif




insted_SYM		/*  SBC abs */
	GET_ABS_RD();
	SBC_INST();

instee_SYM		/*  INC abs */
	GET_ABS_RD();
	INC_INST();


instef_SYM		/*  SBC long */
	GET_LONG_RD();
	SBC_INST();


instf0_SYM		/*  BEQ disp8 */
#ifdef ASM
	COND_BR1
	comib,<> 0,zero,instf0_2_SYM
	COND_BR2

instf0_2_SYM
	COND_BR_UNTAKEN
#else
	C_BEQ_DISP8();
#endif

instf1_SYM		/*  SBC (Dloc),y */
	GET_DLOC_IND_Y_RD();
	SBC_INST();

instf2_SYM		/*  SBC (Dloc) */
	GET_DLOC_IND_RD();
	SBC_INST();

instf3_SYM		/*  SBC (Disp8,s),y */
	GET_DISP8_S_IND_Y_RD();
	SBC_INST();

instf4_SYM		/*  PEA Abs */
#ifdef ASM
	addi	3,pc,pc
	extru	ret0,31,16,arg0
	CYCLES_PLUS_1
	ldil	l%dispatch,link
	b	push_16_unsafe
	ldo	r%dispatch(link),link
#else
	C_PEA_ABS();
#endif

instf5_SYM		/*  SBC Dloc,x */
	GET_DLOC_X_RD();
	SBC_INST();

instf6_SYM		/*  INC Dloc,x */
	GET_DLOC_X_RD();
	INC_INST();

instf7_SYM		/*  SBC [Dloc],Y */
	GET_DLOC_L_IND_Y_RD();
	SBC_INST();

instf8_SYM		/*  SED */
#ifdef ASM
	depi	1,28,1,psr		/* set decimal */
	b	dispatch
	addi	1,pc,pc
#else
	C_SED();
#endif

instf9_SYM		/*  SBC abs,y */
	GET_ABS_Y_RD();
	SBC_INST();

instfa_SYM		/*  PLX */
#ifdef ASM
	bb,<	psr,27,instfa_8bit_SYM
	CYCLES_PLUS_1

	bl	pull_16,link
	addi	1,pc,pc

	extru	ret0,31,16,zero
	extru	ret0,16,1,neg
	b	dispatch
	copy	zero,xreg

instfa_8bit_SYM
	bl	pull_8,link
	addi	1,pc,pc

	extru	ret0,31,8,zero
	extru	ret0,24,1,neg
	b	dispatch
	copy	zero,xreg
#else
	C_PLX();
#endif

instfb_SYM		/*  XCE */
#ifdef ASM
	extru	psr,27,2,arg0		/* save old x & m */
	addi	1,pc,pc
	extru	psr,23,1,scratch1	/* e bit */
	dep	psr,23,1,psr		/* copy carry to e bit */
	b	update_system_state
	dep	scratch1,31,1,psr	/* copy e bit to carry */
#else
	C_XCE();
#endif

instfc_SYM		/*  JSR (Abs,X) */
#ifdef ASM
	dep	kbank,15,16,ret0
	addi	2,pc,pc
	add	xreg,ret0,arg0
	bl	get_mem_long_16,link
	extru	arg0,31,24,arg0

	CYCLES_PLUS_2
	extru	pc,31,16,arg0
	ldil	l%dispatch,link
	extru	ret0,31,16,pc
	b	push_16_unsafe
	ldo	r%dispatch(link),link
#else
	C_JSR_IND_ABS_X();
#endif

instfd_SYM		/*  SBC Abs,X */
	GET_ABS_X_RD();
	SBC_INST();

instfe_SYM		/*  INC Abs,X */
	GET_ABS_X_RD_WR();
	INC_INST();

instff_SYM		/*  SBC Long,X */
	GET_LONG_X_RD();
	SBC_INST();

