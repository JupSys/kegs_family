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
	.data
	.export rcsdif_defs_instr_h,data
rcsdif_defs_instr_h
	.stringz "@(#)$Header: defs_instr.h,v 1.48 98/05/17 16:21:52 kentd Exp $"
	.code
# endif

# ifdef ACC8
	.export defs_instr_start_8,data
defs_instr_start_8	.word	0
# else
	.export defs_instr_start_16,data
defs_instr_start_16	.word	0
# endif	/* ACC8*/
#endif	/* ASM */


#undef GET_DLOC_X_IND_RD

#ifdef ASM
# ifdef ACC8
#  define GET_DLOC_X_IND_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_X_IND_WR()			! \
	bl	get_mem_long_8,link		! \
	nop
# else
#  define GET_DLOC_X_IND_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_X_IND_WR()			! \
	bl	get_mem_long_16,link		! \
	nop
# endif /* ACC8 */
#else /* C*/
# ifdef ACC8
#  define GET_DLOC_X_IND_RD()			\
	GET_1BYTE_ARG;				\
	GET_DLOC_X_IND_WR();			\
	GET_MEMORY8(arg, arg);
# else
#  define GET_DLOC_X_IND_RD()			\
	GET_1BYTE_ARG;				\
	GET_DLOC_X_IND_WR();			\
	GET_MEMORY16(arg, arg);
# endif
#endif

#undef GET_DISP8_S_RD

#ifdef ASM
# ifdef ACC8
#  define GET_DISP8_S_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DISP8_S_WR()			! \
	bl	get_mem_b0_8,link		! \
	nop
# else
#  define GET_DISP8_S_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DISP8_S_WR()			! \
	bl	get_mem_b0_16,link		! \
	nop
# endif
#else /* C */
# ifdef ACC8
#  define GET_DISP8_S_RD()			\
	GET_1BYTE_ARG;				\
	GET_DISP8_S_WR();			\
	GET_MEMORY8(arg, arg);
# else
#  define GET_DISP8_S_RD()			\
	GET_1BYTE_ARG;				\
	GET_DISP8_S_WR();			\
	GET_MEMORY16(arg, arg);
# endif
#endif


#ifdef ASM
# define MUST_FIX				\
	break
#endif

#undef GET_DLOC_RD

#ifdef ASM
# ifdef ACC8
#  define GET_DLOC_RD()				\
	ldb	1(scratch1),arg0		! \
	extru,=	direct,31,8,0			! \
	CYCLES_PLUS_1				! \
	addi	2,pc,pc				! \
	add	direct,arg0,arg0		! \
	bl	get_mem_b0_8,link		! \
	extru	arg0,31,16,arg0
# else
#  define GET_DLOC_RD()				\
	ldb	1(scratch1),arg0		! \
	extru,=	direct,31,8,0			! \
	CYCLES_PLUS_1				! \
	addi	2,pc,pc				! \
	add	direct,arg0,arg0		! \
	bl	get_mem_b0_16,link		! \
	extru	arg0,31,16,arg0
# endif
#else /* C */
# ifdef ACC8
#  define GET_DLOC_RD()				\
	GET_1BYTE_ARG;				\
	if(direct & 0xff) {			\
		CYCLES_PLUS_1;			\
	}					\
	pc += 2;				\
	GET_MEMORY8((direct + arg) & 0xffff, arg);
# else
#  define GET_DLOC_RD()				\
	GET_1BYTE_ARG;				\
	if(direct & 0xff) {			\
		CYCLES_PLUS_1;			\
	}					\
	pc += 2;				\
	GET_MEMORY16((direct + arg) & 0xffff, arg);
# endif
#endif


#undef GET_DLOC_L_IND_RD
#ifdef ASM
# ifdef ACC8
#  define GET_DLOC_L_IND_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_L_IND_WR()			! \
	bl	get_mem_long_8,link		! \
	nop
# else
#  define GET_DLOC_L_IND_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_L_IND_WR()			! \
	bl	get_mem_long_16,link		! \
	nop
# endif
#else /* C */
# ifdef ACC8
#  define GET_DLOC_L_IND_RD()			\
	GET_1BYTE_ARG;				\
	GET_DLOC_L_IND_WR();			\
	GET_MEMORY8(arg, arg);
# else
#  define GET_DLOC_L_IND_RD()			\
	GET_1BYTE_ARG;				\
	GET_DLOC_L_IND_WR();			\
	GET_MEMORY16(arg, arg);
# endif
#endif


#undef GET_IMM_MEM

#ifdef ASM
# ifdef ACC8
#  define GET_IMM_MEM()				\
	ldb	1(scratch1),ret0		! \
	addi	2,pc,pc
# else
#  define GET_IMM_MEM()				\
	ldb	2(scratch1),scratch2		! \
	CYCLES_PLUS_1				! \
	ldb	1(scratch1),ret0		! \
	addi	3,pc,pc				! \
	dep	scratch2,23,8,ret0
# endif
#else
# ifdef ACC8
#  define GET_IMM_MEM()				\
	GET_1BYTE_ARG;				\
	pc += 2;
# else
#  define GET_IMM_MEM()				\
	GET_2BYTE_ARG;				\
	CYCLES_PLUS_1;				\
	pc += 3;
# endif
#endif


#undef GET_ABS_RD

#ifdef ASM
# ifdef ACC8
#  define GET_ABS_RD()				\
	ldb	1(scratch1),arg0		! \
	CYCLES_PLUS_1				! \
	ldb	2(scratch1),scratch2		! \
	addi	3,pc,pc				! \
	dep	dbank,15,16,arg0		! \
	bl	get_mem_long_8,link		! \
	dep	scratch2,23,8,arg0
# else
#  define GET_ABS_RD()				\
	ldb	1(scratch1),arg0		! \
	CYCLES_PLUS_1				! \
	ldb	2(scratch1),scratch2		! \
	addi	3,pc,pc				! \
	dep	dbank,15,16,arg0		! \
	bl	get_mem_long_16,link		! \
	dep	scratch2,23,8,arg0
# endif
#else
# ifdef ACC8
#  define GET_ABS_RD()				\
	GET_2BYTE_ARG;				\
	GET_MEMORY8((dbank << 16) + arg, arg);	\
	CYCLES_PLUS_1;				\
	pc += 3;
# else
#  define GET_ABS_RD()				\
	GET_2BYTE_ARG;				\
	GET_MEMORY16((dbank << 16) + arg, arg);	\
	CYCLES_PLUS_1;				\
	pc += 3;
# endif
#endif


#undef GET_LONG_RD


#ifdef ASM
# ifdef ACC8
#  define GET_LONG_RD()				\
	ldb	1(scratch1),arg0		! \
	CYCLES_PLUS_2				! \
	ldb	2(scratch1),scratch2		! \
	addi	4,pc,pc				! \
	ldb	3(scratch1),scratch1		! \
	dep	scratch2,23,8,arg0		! \
	bl	get_mem_long_8,link		! \
	dep	scratch1,15,8,arg0
# else
#  define GET_LONG_RD()				\
	ldb	1(scratch1),arg0		! \
	CYCLES_PLUS_2				! \
	ldb	2(scratch1),scratch2		! \
	addi	4,pc,pc				! \
	ldb	3(scratch1),scratch1		! \
	dep	scratch2,23,8,arg0		! \
	bl	get_mem_long_16,link		! \
	dep	scratch1,15,8,arg0
# endif
#else /* C */
# ifdef ACC8
#  define GET_LONG_RD()				\
	GET_3BYTE_ARG;				\
	GET_MEMORY8(arg, arg);			\
	CYCLES_PLUS_2;				\
	pc += 4;
# else
#  define GET_LONG_RD()				\
	GET_3BYTE_ARG;				\
	GET_MEMORY16(arg, arg);			\
	CYCLES_PLUS_2;				\
	pc += 4;
# endif
#endif



#undef GET_DLOC_IND_Y_RD

#undef GET_DLOC_IND_Y_WR_SPECIAL2

#define GET_DLOC_IND_Y_WR_SPECIAL2()	\
	add	direct,arg0,arg0	! \
	bl	get_mem_b0_direct_page_16,link	! \
	extru	arg0,31,16,arg0		! \
	dep	dbank,15,8,ret0		! \
	extru,=	direct,31,8,0		! \
	CYCLES_PLUS_1			! \
	add	yreg,ret0,arg0			/* don't change this instr */
						/*  or add any after */
						/*  to preserve ret0 & arg0 */


#ifdef ASM
# ifdef ACC8
#  define GET_DLOC_IND_Y_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_IND_Y_WR_SPECIAL2()		! \
	xor	arg0,ret0,scratch1		! \
	extru,=	psr,27,1,0			! \
	extru,=	scratch1,23,8,0			! \
	CYCLES_PLUS_1				! \
	bl	get_mem_long_8,link		! \
	addi	2,pc,pc
# else
#  define GET_DLOC_IND_Y_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_IND_Y_WR_SPECIAL2()		! \
	xor	arg0,ret0,scratch1		! \
	extru,=	psr,27,1,0			! \
	extru,=	scratch1,23,8,0			! \
	CYCLES_PLUS_1				! \
	bl	get_mem_long_16,link		! \
	addi	2,pc,pc
# endif
#else /* C */
# ifdef ACC8
#  define GET_DLOC_IND_Y_RD()			\
	GET_1BYTE_ARG;							\
	GET_MEMORY_DIRECT_PAGE16((direct + arg) & 0xffff, tmp1);	\
	tmp1 += (dbank << 16);						\
	if(direct & 0xff) {						\
		CYCLES_PLUS_1;						\
	}								\
	tmp2 = tmp1 + yreg;						\
	if(((psr & 0x10) == 0) || ((tmp1 ^ tmp2) & 0xff00)) {		\
		CYCLES_PLUS_1;						\
	}								\
	pc += 2;							\
	GET_MEMORY8(tmp2, arg);
# else
#  define GET_DLOC_IND_Y_RD()			\
	GET_1BYTE_ARG;							\
	GET_MEMORY_DIRECT_PAGE16((direct + arg) & 0xffff, tmp1);	\
	tmp1 += (dbank << 16);						\
	if(direct & 0xff) {						\
		CYCLES_PLUS_1;						\
	}								\
	tmp2 = tmp1 + yreg;						\
	if(((psr & 0x10) == 0) || ((tmp1 ^ tmp2) & 0xff00)) {		\
		CYCLES_PLUS_1;						\
	}								\
	pc += 2;							\
	GET_MEMORY16(tmp2, arg);
# endif
#endif


#undef GET_DLOC_IND_RD

#ifdef ASM
# ifdef ACC8
#  define GET_DLOC_IND_RD()				\
	ldb	1(scratch1),arg0		! \
	extru,=	direct,31,8,0			! \
	CYCLES_PLUS_1				! \
	addi	2,pc,pc				! \
	add	direct,arg0,arg0		! \
	bl	get_mem_b0_direct_page_16,link	! \
	extru	arg0,31,16,arg0			! \
	copy	ret0,arg0			! \
	bl	get_mem_long_8,link		! \
	dep	dbank,15,16,arg0
# else
#  define GET_DLOC_IND_RD()				\
	ldb	1(scratch1),arg0		! \
	extru,=	direct,31,8,0			! \
	CYCLES_PLUS_1				! \
	addi	2,pc,pc				! \
	add	direct,arg0,arg0		! \
	bl	get_mem_b0_direct_page_16,link	! \
	extru	arg0,31,16,arg0			! \
	copy	ret0,arg0			! \
	bl	get_mem_long_16,link		! \
	dep	dbank,15,16,arg0
# endif
#else
# ifdef ACC8
#  define GET_DLOC_IND_RD()				\
	GET_1BYTE_ARG;					\
	pc += 2;					\
	if(direct & 0xff) {				\
		CYCLES_PLUS_1;				\
	}						\
	GET_MEMORY_DIRECT_PAGE16((direct + arg) & 0xffff, arg);	\
	GET_MEMORY8((dbank << 16) + arg, arg);
# else
#  define GET_DLOC_IND_RD()				\
	GET_1BYTE_ARG;					\
	pc += 2;					\
	if(direct & 0xff) {				\
		CYCLES_PLUS_1;				\
	}						\
	GET_MEMORY_DIRECT_PAGE16((direct + arg) & 0xffff, arg);	\
	GET_MEMORY16((dbank << 16) + arg, arg);
# endif
#endif


#undef GET_DLOC_X_RD

#ifdef ASM
# ifdef ACC8
#  define GET_DLOC_X_RD()				\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_INDEX_WR_A(xreg)		! \
	bl	get_mem_b0_8,link		! \
	GET_DLOC_INDEX_WR_B(xreg)
# else
#  define GET_DLOC_X_RD()				\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_INDEX_WR_A(xreg)		! \
	bl	get_mem_b0_16,link		! \
	GET_DLOC_INDEX_WR_B(xreg)
# endif
#else
# ifdef ACC8
#  define GET_DLOC_X_RD()						\
	GET_1BYTE_ARG;							\
	CYCLES_PLUS_1;							\
	if(direct & 0xff) {						\
		CYCLES_PLUS_1;						\
	}								\
	pc += 2;							\
	arg = (arg + xreg + direct) & 0xffff;				\
	if(psr & 0x100) {						\
		if((direct & 0xff) == 0) {				\
			arg = (direct & 0xff00) | (arg & 0xff);		\
		}							\
	}								\
	GET_MEMORY8(arg & 0xffff, arg);
# else
#  define GET_DLOC_X_RD()						\
	GET_1BYTE_ARG;							\
	CYCLES_PLUS_1;							\
	if(direct & 0xff) {						\
		CYCLES_PLUS_1;						\
	}								\
	pc += 2;							\
	arg = (arg + xreg + direct) & 0xffff;				\
	if(psr & 0x100) {						\
		if((direct & 0xff) == 0) {				\
			arg = (direct & 0xff00) | (arg & 0xff);		\
		}							\
	}								\
	GET_MEMORY16(arg, arg);
# endif
#endif


#undef GET_DISP8_S_IND_Y_RD

#ifdef ASM
# ifdef ACC8
#  define GET_DISP8_S_IND_Y_RD()		\
	ldb	1(scratch1),arg0		! \
	GET_DISP8_S_IND_Y_WR()			! \
	bl	get_mem_long_8,link		! \
	nop
# else
#  define GET_DISP8_S_IND_Y_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DISP8_S_IND_Y_WR()			! \
	bl	get_mem_long_16,link		! \
	nop
# endif
#else
# ifdef ACC8
#  define GET_DISP8_S_IND_Y_RD()			\
	GET_1BYTE_ARG;					\
	GET_DISP8_S_IND_Y_WR();				\
	GET_MEMORY8(arg, arg);
# else
#  define GET_DISP8_S_IND_Y_RD()			\
	GET_1BYTE_ARG;					\
	GET_DISP8_S_IND_Y_WR();				\
	GET_MEMORY16(arg, arg);
# endif
#endif


#undef GET_DLOC_L_IND_Y_RD

#ifdef ASM
# ifdef ACC8
#  define GET_DLOC_L_IND_Y_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_L_IND_Y_WR()			! \
	bl	get_mem_long_8,link		! \
	nop
# else
#  define GET_DLOC_L_IND_Y_RD()			\
	ldb	1(scratch1),arg0		! \
	GET_DLOC_L_IND_Y_WR()			! \
	bl	get_mem_long_16,link		! \
	nop
# endif
#else /* C */
# ifdef ACC8
#  define GET_DLOC_L_IND_Y_RD()			\
	GET_1BYTE_ARG;				\
	GET_DLOC_L_IND_Y_WR();			\
	GET_MEMORY8(arg, arg);
# else
#  define GET_DLOC_L_IND_Y_RD()			\
	GET_1BYTE_ARG;				\
	GET_DLOC_L_IND_Y_WR();			\
	GET_MEMORY16(arg, arg);
# endif
#endif


#undef GET_ABS_INDEX_ADDR_FOR_RD

#ifdef ASM
/* to get cycle right:  add 1 cycle	*/
/*   if (x is 16bit) || (carry into high byte), add another cycle */
/* So, if x==16bit, add 1.  If x==8bit then add 1 if carry */
#  define GET_ABS_INDEX_ADDR_FOR_RD(index_reg)	\
	ldb	1(scratch1),ret0		! \
	CYCLES_PLUS_1				! \
	ldb	2(scratch1),scratch1		! \
	dep	dbank,15,16,ret0		! \
	addi	3,pc,pc				! \
	dep	scratch1,23,8,ret0		! \
	add	ret0,index_reg,arg0		! \
	xor	arg0,ret0,scratch1		! \
	extru,=	psr,27,1,0			! \
	extru,=	scratch1,23,8,0			! \
	CYCLES_PLUS_1
#else /* C */
#  define GET_ABS_INDEX_ADDR_FOR_RD(index_reg)			\
	GET_2BYTE_ARG;						\
	CYCLES_PLUS_1;						\
	pc += 3;						\
	tmp1 = (dbank << 16) + arg;				\
	arg = tmp1 + index_reg;					\
	if(((psr & 0x10) == 0) || ((tmp1 ^ arg) & 0xff00)) {	\
		CYCLES_PLUS_1;					\
	}
#endif

#undef GET_ABS_Y_RD

#ifdef ASM
# ifdef ACC8
#  define GET_ABS_Y_RD()			\
	GET_ABS_INDEX_ADDR_FOR_RD(yreg)		! \
	bl	get_mem_long_8,link		! \
	extru	arg0,31,24,arg0
# else
#  define GET_ABS_Y_RD()			\
	GET_ABS_INDEX_ADDR_FOR_RD(yreg)		! \
	bl	get_mem_long_16,link		! \
	extru	arg0,31,24,arg0
# endif
#else /* C */
# ifdef ACC8
#  define GET_ABS_Y_RD()			\
	GET_ABS_INDEX_ADDR_FOR_RD(yreg);	\
	GET_MEMORY8(arg, arg);
# else
#  define GET_ABS_Y_RD()			\
	GET_ABS_INDEX_ADDR_FOR_RD(yreg);	\
	GET_MEMORY16(arg, arg);
# endif
#endif




#undef GET_ABS_X_RD
#undef GET_ABS_X_RD_WR

#ifdef ASM
# ifdef ACC8
#  define GET_ABS_X_RD()			\
	GET_ABS_INDEX_ADDR_FOR_RD(xreg)		! \
	bl	get_mem_long_8,link		! \
	extru	arg0,31,24,arg0

#  define GET_ABS_X_RD_WR()			\
	ldb	1(scratch1),ret0		! \
	addi	3,pc,pc				! \
	ldb	2(scratch1),scratch1		! \
	dep	dbank,15,16,ret0		! \
	CYCLES_PLUS_2				! \
	dep	scratch1,23,8,ret0		! \
	add	ret0,xreg,arg0			! \
	bl	get_mem_long_8,link		! \
	extru	arg0,31,24,arg0	
# else
#  define GET_ABS_X_RD()			\
	GET_ABS_INDEX_ADDR_FOR_RD(xreg)		! \
	bl	get_mem_long_16,link		! \
	extru	arg0,31,24,arg0

#  define GET_ABS_X_RD_WR()			\
	ldb	1(scratch1),ret0		! \
	addi	3,pc,pc				! \
	ldb	2(scratch1),scratch1		! \
	dep	dbank,15,16,ret0		! \
	CYCLES_PLUS_2				! \
	dep	scratch1,23,8,ret0		! \
	add	ret0,xreg,arg0			! \
	bl	get_mem_long_16,link		! \
	extru	arg0,31,24,arg0
# endif
#else /* C */
# ifdef ACC8
#  define GET_ABS_X_RD()					\
	GET_ABS_INDEX_ADDR_FOR_RD(xreg);			\
	GET_MEMORY8(arg, arg);

#  define GET_ABS_X_RD_WR()					\
	GET_2BYTE_ARG;						\
	pc += 3;						\
	CYCLES_PLUS_2;						\
	arg = (dbank << 16) + ((arg + xreg) & 0xffff);		\
	GET_MEMORY8(arg, arg);
# else
#  define GET_ABS_X_RD()					\
	GET_ABS_INDEX_ADDR_FOR_RD(xreg);			\
	GET_MEMORY16(arg, arg);

#  define GET_ABS_X_RD_WR()					\
	GET_2BYTE_ARG;						\
	pc += 3;						\
	CYCLES_PLUS_2;						\
	arg = (dbank << 16) + ((arg + xreg) & 0xffff);		\
	GET_MEMORY16(arg, arg);
# endif
#endif


#undef GET_LONG_X_RD

#ifdef ASM
# ifdef ACC8
#  define GET_LONG_X_RD()			\
	ldb	1(scratch1),ret0		! \
	ldb	2(scratch1),scratch2		! \
	CYCLES_PLUS_2				! \
	ldb	3(scratch1),scratch1		! \
	addi	4,pc,pc				! \
	dep	scratch2,23,8,ret0		! \
	dep	scratch1,15,8,ret0		! \
	add	ret0,xreg,arg0			! \
	bl	get_mem_long_8,link		! \
	extru	arg0,31,24,arg0
# else
#  define GET_LONG_X_RD()			\
	ldb	1(scratch1),ret0		! \
	ldb	2(scratch1),scratch2		! \
	CYCLES_PLUS_2				! \
	ldb	3(scratch1),scratch1		! \
	addi	4,pc,pc				! \
	dep	scratch2,23,8,ret0		! \
	dep	scratch1,15,8,ret0		! \
	add	ret0,xreg,arg0			! \
	bl	get_mem_long_16,link		! \
	extru	arg0,31,24,arg0
# endif
#else /* C */
# ifdef ACC8
#  define GET_LONG_X_RD()			\
	GET_3BYTE_ARG;				\
	arg = (arg + xreg) & 0xffffff;		\
	pc += 4;				\
	CYCLES_PLUS_2;				\
	GET_MEMORY8(arg, arg);
# else
#  define GET_LONG_X_RD()			\
	GET_3BYTE_ARG;				\
	arg = (arg + xreg) & 0xffffff;		\
	pc += 4;				\
	CYCLES_PLUS_2;				\
	GET_MEMORY16(arg, arg);
# endif
#endif


#define SET_NEG_ZERO16(val)	\
	zero = val;		\
	neg = (val) >> 15;

# define SET_NEG_ZERO8(val)	\
	zero = val;		\
	neg = (val) >> 7;

#define SET_CARRY8(val)		\
	psr = (psr & ~1) + (((val) >> 8) & 1);

#define SET_CARRY16(val)	\
	psr = (psr & ~1) + (((val) >> 16) & 1);

#if 0
# define NEGZERO8(val)		SET_NEG_ZERO8(val)
#else
# define NEGZERO16(val)		SET_NEG_ZERO16(val)
#endif

#define SET_INDEX_REG(src, dest)		\
	if(psr & 0x10) {			\
		dest = (src) & 0xff;		\
		SET_NEG_ZERO8(dest);		\
	} else {				\
		dest = (src) & 0xffff;		\
		SET_NEG_ZERO16(dest);		\
	}

#define LD_INDEX_INST(index_reg)		\
	if(psr & 0x10) {			\
		GET_MEMORY8(arg, arg);		\
	} else {				\
		GET_MEMORY16(arg, arg);		\
	}					\
	SET_INDEX_REG(arg, index_reg);

#define LDX_INST()	LD_INDEX_INST(xreg)
#define LDY_INST()	LD_INDEX_INST(yreg)

#undef ORA_INST

#ifdef ASM
# ifdef ACC8
#  define ORA_INST()				\
	ldi	0xff,scratch1			! \
	or	acc,ret0,arg0 			! \
	and	arg0,scratch1,zero		! \
	extru	arg0,24,1,neg			! \
	b	dispatch			! \
	dep	arg0,31,8,acc
# else
#  define ORA_INST()				\
	zdepi	-1,31,16,scratch1		! \
	or	acc,ret0,arg0 			! \
	and	arg0,scratch1,zero		! \
	extru	arg0,16,1,neg			! \
	b	dispatch			! \
	dep	arg0,31,16,acc
# endif
#else /* C */
# ifdef ACC8
#  define ORA_INST()				\
	tmp1 = (acc | arg) & 0xff;		\
	acc = (acc & 0xff00) + tmp1;		\
	SET_NEG_ZERO8(tmp1);
# else
#  define ORA_INST()				\
	acc = (acc | arg);			\
	SET_NEG_ZERO16(acc);
# endif
#endif

#undef AND_INST

#ifdef ASM
# ifdef ACC8
#  define AND_INST()				\
	ldi	0xff,scratch1			! \
	and	acc,ret0,arg0 			! \
	and	arg0,scratch1,zero		! \
	extru	arg0,24,1,neg			! \
	b	dispatch			! \
	dep	arg0,31,8,acc
# else
#  define AND_INST()				\
	zdepi	-1,31,16,scratch1		! \
	and	acc,ret0,arg0 			! \
	and	arg0,scratch1,zero		! \
	extru	arg0,16,1,neg			! \
	b	dispatch			! \
	dep	arg0,31,16,acc
# endif
#else /* C */
# ifdef ACC8
#  define AND_INST()				\
	tmp1 = (acc & arg) & 0xff;		\
	acc = (acc & 0xff00) + tmp1;		\
	SET_NEG_ZERO8(tmp1);
# else
#  define AND_INST()				\
	acc = (acc & arg);			\
	SET_NEG_ZERO16(acc);
# endif
#endif

#undef EOR_INST

#ifdef ASM
# ifdef ACC8
#  define EOR_INST()				\
	ldi	0xff,scratch1			! \
	xor	acc,ret0,arg0 			! \
	and	arg0,scratch1,zero		! \
	extru	arg0,24,1,neg			! \
	b	dispatch			! \
	dep	arg0,31,8,acc
# else
#  define EOR_INST()				\
	zdepi	-1,31,16,scratch1		! \
	xor	acc,ret0,arg0 			! \
	and	arg0,scratch1,zero		! \
	extru	arg0,16,1,neg			! \
	b	dispatch			! \
	dep	arg0,31,16,acc
# endif
#else /* C */
# ifdef ACC8
#  define EOR_INST()				\
	tmp1 = (acc ^ arg) & 0xff;		\
	acc = (acc & 0xff00) + tmp1;		\
	SET_NEG_ZERO8(tmp1);
# else
#  define EOR_INST()				\
	acc = (acc ^ arg);			\
	SET_NEG_ZERO16(acc);
# endif
#endif

# undef LDA_INST

#ifdef ASM
# ifdef ACC8
#  define LDA_INST()				\
	extru	ret0,31,8,zero			! \
	extru	ret0,24,1,neg			! \
	b	dispatch			! \
	dep	zero,31,8,acc
# else
#  define LDA_INST()				\
	extru	ret0,31,16,zero			! \
	extru	ret0,16,1,neg			! \
	b	dispatch			! \
	dep	zero,31,16,acc
# endif
#else /* C*/
# ifdef ACC8
#  define LDA_INST()				\
	acc = (acc & 0xff00) + (arg & 0xff);	\
	SET_NEG_ZERO8(arg & 0xff);
# else
#  define LDA_INST()				\
	acc = (arg & 0xffff);			\
	SET_NEG_ZERO16(acc);
# endif
#endif

# undef ADC_INST

#ifdef ASM
# ifdef ACC8
#  define ADC_INST()				\
	b	adc_binary_8			! \
	extru	acc,31,8,arg0
# else
#  define ADC_INST()				\
	b	adc_binary_16			! \
	extru	acc,31,16,arg0
# endif
#else /* C */
# ifdef ACC8
#  define ADC_INST()						\
	tmp1 = do_adc_sbc8(acc & 0xff, arg & 0xff, psr, 0);	\
	acc = (acc & 0xff00) + (tmp1 & 0xff);			\
	psr = (tmp1 >> 16);					\
	zero = !(psr & 0x2);					\
	neg = (psr >> 7) & 1;
# else
#  define ADC_INST()						\
	tmp1 = do_adc_sbc16(acc, arg & 0xffff, psr, 0);		\
	acc = (tmp1 & 0xffff);					\
	psr = (tmp1 >> 16);					\
	zero = !(psr & 0x2);					\
	neg = (psr >> 7) & 1;
# endif
#endif


# undef SBC_INST

#ifdef ASM
# ifdef ACC8
#  define SBC_INST()				\
	uaddcm	0,ret0,ret0			! \
	b	sbc_binary_8			! \
	extru	acc,31,8,arg0
# else
#  define SBC_INST()				\
	uaddcm	0,ret0,ret0			! \
	b	sbc_binary_16			! \
	extru	acc,31,16,arg0
# endif
#else /* C */
# ifdef ACC8
#  define SBC_INST()						\
	tmp1 = do_adc_sbc8(acc & 0xff, arg & 0xff, psr, 1);	\
	acc = (acc & 0xff00) + (tmp1 & 0xff);			\
	psr = (tmp1 >> 16);					\
	zero = !(psr & 0x2);					\
	neg = (psr >> 7) & 1;
# else
#  define SBC_INST()						\
	tmp1 = do_adc_sbc16(acc, arg & 0xffff, psr, 1);		\
	acc = (tmp1 & 0xffff);					\
	psr = (tmp1 >> 16);					\
	zero = !(psr & 0x2);					\
	neg = (psr >> 7) & 1;
# endif
#endif


# undef CMP_INST

#ifdef ASM
# ifdef ACC8
#  define CMP_INST()				! \
	extru	acc,31,8,scratch1		! \
	subi	0x100,ret0,ret0			! \
	ldi	0xff,scratch2			! \
	add	ret0,scratch1,ret0		! \
	extru	ret0,23,1,scratch1		! \
	and	ret0,scratch2,zero		! \
	extru	ret0,24,1,neg			! \
	b	dispatch			! \
	dep	scratch1,31,1,psr
# else
#  define CMP_INST()				! \
	ldil	l%0x10000,scratch3		! \
	zdepi	-1,31,16,scratch2		! \
	sub	scratch3,ret0,ret0		! \
	add	ret0,acc,ret0			! \
	extru	ret0,15,1,scratch1		! \
	and	ret0,scratch2,zero		! \
	extru	ret0,16,1,neg			! \
	b	dispatch			! \
	dep	scratch1,31,1,psr
# endif
#else /* C */
# ifdef ACC8
#  define CMP_INST()					\
	arg = (acc & 0xff) + (0x100 - arg);		\
	SET_CARRY8(arg);				\
	arg = arg & 0xff;				\
	SET_NEG_ZERO8(arg & 0xff);
# else
#  define CMP_INST()					\
	arg = (acc & 0xffff) + (0x10000 - arg);		\
	SET_CARRY16(arg);				\
	arg = arg & 0xffff;				\
	SET_NEG_ZERO16(arg & 0xffff);
# endif
#endif

# undef BIT_INST

#ifdef ASM
# ifdef ACC8
#  define BIT_INST()				\
	ldi	0xff,scratch1			! \
	and	acc,ret0,arg0 			! \
	extru	ret0,24,1,neg			! \
	and	arg0,scratch1,zero		! \
	extru	ret0,25,1,scratch2		! \
	b	dispatch			! \
	dep	scratch2,25,1,psr
# else
#  define BIT_INST()				\
	zdepi	-1,31,16,scratch1		! \
	and	acc,ret0,arg0 			! \
	extru	ret0,16,1,neg			! \
	and	arg0,scratch1,zero		! \
	extru	ret0,17,1,scratch2		! \
	b	dispatch			! \
	dep	scratch2,25,1,psr
# endif
#else /* C */
# ifdef ACC8
#  define BIT_INST()				\
	neg = arg >> 7;				\
	zero = (acc & arg & 0xff);		\
	psr = (psr & (~0x40)) | (arg & 0x40);
# else
#  define BIT_INST()				\
	neg = arg >> 15;			\
	zero = (acc & arg & 0xffff);		\
	psr = (psr & (~0x40)) | ((arg >> 8) & 0x40);
# endif
#endif

# undef STA_INST

#ifdef ASM
# ifdef ACC8
#  define STA_INST()				\
	extru	acc,31,8,arg1			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
#  define STA_INST()				\
	extru	acc,31,16,arg1			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C*/
# ifdef ACC8
#  define STA_INST()				\
	SET_MEMORY8(arg, acc);
# else
#  define STA_INST()				\
	SET_MEMORY16(arg, acc);
# endif
#endif


#undef TSB_INST

#ifdef ASM
# ifdef ACC8
/* Uses addr_latch as full apple addr to write data to */
#  define TSB_INST()				\
	or	acc,ret0,arg1			! \
	CYCLES_PLUS_1				! \
	extru	arg1,31,8,arg1			! \
	and	ret0,acc,zero			! \
	copy	addr_latch,arg0			! \
	extru	zero,31,8,zero			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
/* Uses addr_latch as full apple addr to write data to */
#  define TSB_INST()				\
	or	acc,ret0,arg1			! \
	CYCLES_PLUS_1				! \
	extru	arg1,31,16,arg1			! \
	and	ret0,acc,zero			! \
	copy	addr_latch,arg0			! \
	extru	zero,31,16,zero			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C*/
# ifdef ACC8
#  define TSB_INST()				\
	tmp1 = arg | acc;			\
	CYCLES_PLUS_1;				\
	zero = arg & acc;			\
	SET_MEMORY8(addr_latch, tmp1);
# else
#  define TSB_INST()				\
	tmp1 = arg | acc;			\
	CYCLES_PLUS_1;				\
	zero = arg & acc;			\
	SET_MEMORY16(addr_latch, tmp1);
# endif
#endif


#undef ASL_INST

#ifdef ASM
# ifdef ACC8
/* Uses addr_latch as full apple addr to write data to */
#  define ASL_INST()				\
	ldi	0xff,scratch1			! \
	copy	addr_latch,arg0			! \
	sh1add	ret0,0,scratch3			! \
	CYCLES_PLUS_1				! \
	extru	scratch3,24,1,neg		! \
	and	scratch3,scratch1,zero		! \
	extru	scratch3,23,1,scratch2		! \
	copy	zero,arg1			! \
	dep	scratch2,31,1,psr /* set carry */ ! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
/* Uses addr_latch as full apple addr to write data to */
#  define ASL_INST()				\
	zdepi	-1,31,16,scratch1		! \
	copy	addr_latch,arg0			! \
	sh1add	ret0,0,scratch3			! \
	CYCLES_PLUS_1				! \
	extru	scratch3,16,1,neg		! \
	and	scratch3,scratch1,zero		! \
	extru	scratch3,15,1,scratch2		! \
	copy	zero,arg1			! \
	dep	scratch2,31,1,psr /* set carry */ ! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C */
# ifdef ACC8
#  define ASL_INST()				\
	psr = (psr & 0x1fe) + ((arg >> 7) & 1);	\
	tmp1 = (arg << 1) & 0xff;		\
	CYCLES_PLUS_1;				\
	SET_NEG_ZERO8(tmp1);				\
	SET_MEMORY8(addr_latch, tmp1);
# else
#  define ASL_INST()				\
	psr = (psr & 0x1fe) + ((arg >> 15) & 1);\
	tmp1 = (arg << 1) & 0xffff;		\
	CYCLES_PLUS_1;				\
	SET_NEG_ZERO16(tmp1);			\
	SET_MEMORY16(addr_latch, tmp1);
# endif
#endif

# undef ROL_INST

#ifdef ASM
# ifdef ACC8
/* Uses addr_latch as full apple addr to write data to */
#  define ROL_INST()				\
	extru	psr,31,1,scratch2		! \
	ldi	0xff,scratch1			! \
	copy	addr_latch,arg0			! \
	sh1add	ret0,scratch2,scratch3		! \
	CYCLES_PLUS_1				! \
	extru	scratch3,24,1,neg		! \
	and	scratch3,scratch1,zero		! \
	extru	scratch3,23,1,scratch2		! \
	copy	zero,arg1			! \
	dep	scratch2,31,1,psr /* set carry */ ! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
/* Uses addr_latch as full apple addr to write data to */
#  define ROL_INST()				\
	extru	psr,31,1,scratch2		! \
	copy	addr_latch,arg0			! \
	zdepi	-1,31,16,scratch1		! \
	sh1add	ret0,scratch2,scratch3		! \
	CYCLES_PLUS_1				! \
	extru	scratch3,16,1,neg		! \
	and	scratch3,scratch1,zero		! \
	extru	scratch3,15,1,scratch2		! \
	copy	zero,arg1			! \
	dep	scratch2,31,1,psr /* set carry */ ! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C */
# ifdef ACC8
#  define ROL_INST()				\
	arg = (arg << 1) | (psr & 1);		\
	SET_MEMORY8(addr_latch, arg);		\
	CYCLES_PLUS_1;				\
	SET_NEG_ZERO8(arg & 0xff);		\
	SET_CARRY8(arg);
# else
#  define ROL_INST()				\
	arg = (arg << 1) | (psr & 1);		\
	SET_MEMORY16(addr_latch, arg);		\
	CYCLES_PLUS_1;				\
	SET_NEG_ZERO16(arg & 0xffff);		\
	SET_CARRY16(arg);
# endif
#endif

# undef LSR_INST

#ifdef ASM
# ifdef ACC8
/* Uses addr_latch as full apple addr to write data to */
#  define LSR_INST()				\
	copy	addr_latch,arg0			! \
	extru	ret0,30,7,zero			! \
	CYCLES_PLUS_1				! \
	ldi	0,neg				! \
	dep	ret0,31,1,psr /* set carry */	! \
	copy	zero,arg1			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
/* Uses addr_latch as full apple addr to write data to */
#  define LSR_INST()				\
	copy	addr_latch,arg0			! \
	extru	ret0,30,15,zero			! \
	CYCLES_PLUS_1				! \
	ldi	0,neg				! \
	dep	ret0,31,1,psr /* set carry */	! \
	copy	zero,arg1			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C */
# ifdef ACC8
#  define LSR_INST()					\
	SET_CARRY8(arg << 8);				\
	arg = (arg >> 1) & 0x7f;			\
	SET_MEMORY8(addr_latch, arg);			\
	CYCLES_PLUS_1;					\
	SET_NEG_ZERO8(arg);
# else
#  define LSR_INST()					\
	SET_CARRY16(arg << 16);				\
	arg = (arg >> 1) & 0x7fff;			\
	SET_MEMORY16(addr_latch, arg);			\
	CYCLES_PLUS_1;					\
	SET_NEG_ZERO16(arg);
# endif
#endif


# undef ROR_INST

#ifdef ASM
# ifdef ACC8
#  define ROR_INST()				! \
	extru	psr,31,1,neg			! \
	copy	addr_latch,arg0			! \
	extru	ret0,30,7,zero			! \
	CYCLES_PLUS_1				! \
	dep	neg,24,1,zero			! \
	copy	zero,arg1			! \
	dep	ret0,31,1,psr			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
#  define ROR_INST()				! \
	extru	psr,31,1,neg			! \
	copy	addr_latch,arg0			! \
	extru	ret0,30,15,zero			! \
	CYCLES_PLUS_1				! \
	dep	neg,16,1,zero			! \
	copy	zero,arg1			! \
	dep	ret0,31,1,psr			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C */
# ifdef ACC8
#  define ROR_INST()					\
	tmp1 = psr & 1;					\
	SET_CARRY8(arg << 8);				\
	arg = ((arg >> 1) & 0x7f) | (tmp1 << 7);	\
	SET_MEMORY8(addr_latch, arg);			\
	CYCLES_PLUS_1;					\
	SET_NEG_ZERO8(arg);
# else
#  define ROR_INST()					\
	tmp1 = psr & 1;					\
	SET_CARRY16(arg << 16);				\
	arg = ((arg >> 1) & 0x7fff) | (tmp1 << 15);	\
	SET_MEMORY16(addr_latch, arg);			\
	CYCLES_PLUS_1;					\
	SET_NEG_ZERO16(arg);
# endif
#endif

# undef TRB_INST

#ifdef ASM
# ifdef ACC8
/* Uses addr_latch as full apple addr to write data to */
#  define TRB_INST()				\
	andcm	ret0,acc,arg1			! \
	copy	addr_latch,arg0			! \
	and	ret0,acc,zero			! \
	extru	arg1,31,8,arg1			! \
	CYCLES_PLUS_1				! \
	extru	zero,31,8,zero			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
/* Uses addr_latch as full apple addr to write data to */
#  define TRB_INST()				\
	andcm	ret0,acc,arg1			! \
	CYCLES_PLUS_1				! \
	extru	arg1,31,16,arg1			! \
	and	ret0,acc,zero			! \
	copy	addr_latch,arg0			! \
	extru	zero,31,16,zero			! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C */
# ifdef ACC8
#  define TRB_INST()				\
	arg = arg & 0xff;			\
	tmp1 = arg & ~acc;			\
	CYCLES_PLUS_1;				\
	zero = arg & acc;			\
	SET_MEMORY8(addr_latch, tmp1);
# else
#  define TRB_INST()				\
	tmp1 = arg & ~acc;			\
	CYCLES_PLUS_1;				\
	zero = arg & acc;			\
	SET_MEMORY16(addr_latch, tmp1);
# endif
#endif

# undef DEC_INST

#ifdef ASM
# ifdef ACC8
#  define DEC_INST()				! \
	addi	-1,ret0,ret0			! \
	CYCLES_PLUS_1				! \
	extru	ret0,24,1,neg			! \
	copy	addr_latch,arg0			! \
	extru	ret0,31,8,zero			! \
	ldil	l%dispatch,link			! \
	copy	zero,arg1			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
#  define DEC_INST()				! \
	addi	-1,ret0,ret0			! \
	CYCLES_PLUS_1				! \
	extru	ret0,16,1,neg			! \
	copy	addr_latch,arg0			! \
	extru	ret0,31,16,zero			! \
	ldil	l%dispatch,link			! \
	copy	zero,arg1			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C */
# ifdef ACC8
#  define DEC_INST()				\
	CYCLES_PLUS_1;				\
	arg = (arg - 1) & 0xff;			\
	SET_MEMORY8(addr_latch, arg);		\
	SET_NEG_ZERO8(arg);
# else
#  define DEC_INST()				\
	CYCLES_PLUS_1;				\
	arg = (arg - 1) & 0xffff;		\
	SET_MEMORY16(addr_latch, arg);		\
	SET_NEG_ZERO16(arg);
# endif
#endif


# undef INC_INST

#ifdef ASM
# ifdef ACC8
#  define INC_INST()				! \
	addi	1,ret0,ret0			! \
	CYCLES_PLUS_1				! \
	extru	ret0,24,1,neg			! \
	copy	addr_latch,arg0			! \
	extru	ret0,31,8,zero			! \
	ldil	l%dispatch,link			! \
	copy	zero,arg1			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
#  define INC_INST()				! \
	addi	1,ret0,ret0			! \
	CYCLES_PLUS_1				! \
	extru	ret0,16,1,neg			! \
	copy	addr_latch,arg0			! \
	extru	ret0,31,16,zero			! \
	ldil	l%dispatch,link			! \
	copy	zero,arg1			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C */
# ifdef ACC8
#  define INC_INST()				\
	CYCLES_PLUS_1;				\
	arg = (arg + 1) & 0xff;			\
	SET_MEMORY8(addr_latch, arg);		\
	SET_NEG_ZERO8(arg);
# else
#  define INC_INST()				\
	CYCLES_PLUS_1;				\
	arg = (arg + 1) & 0xffff;		\
	SET_MEMORY16(addr_latch, arg);		\
	SET_NEG_ZERO16(arg);
# endif
#endif

# undef STZ_INST

#ifdef ASM
# ifdef ACC8
#  define STZ_INST()				\
	ldi	0,arg1				! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_8			! \
	ldo	r%dispatch(link),link
# else
#  define STZ_INST()				\
	ldi	0,arg1				! \
	ldil	l%dispatch,link			! \
	b	set_mem_long_16			! \
	ldo	r%dispatch(link),link
# endif
#else /* C */
# ifdef ACC8
#  define STZ_INST()				\
	SET_MEMORY8(arg, 0);
# else
#  define STZ_INST()				\
	SET_MEMORY16(arg, 0);
# endif
#endif


#undef COND_BR1
#undef COND_BR2
#undef COND_BR_UNTAKEN

#ifdef ASM
# define COND_BR1			\
	ldb	1(scratch1),arg0

/* be careful not to modify pc as first instr of COND_Br2 since it */
/*  is in the delay slot of a branch! */
# define COND_BR2			\
	extru	pc,31,8,scratch4	! \
	extrs	arg0,31,8,ret0		! \
	add	arg0,scratch4,scratch4	! \
	CYCLES_PLUS_1			! \
	depi	0,31,8,scratch4		! \
	addi	2,pc,pc			! \
	and,=	psr,scratch4,0		! \
	CYCLES_PLUS_1			! \
	b	dispatch		! \
	add	ret0,pc,pc


# define COND_BR_UNTAKEN		\
	b	dispatch		! \
	addi	2,pc,pc
#else /* C */
# define BRANCH_DISP8(cond)					\
	GET_1BYTE_ARG;						\
	tmp1 = (arg + (pc & 0xff)) & 0x100;			\
	pc += 2;						\
	if(cond) {						\
		pc = pc + arg - ((arg & 0x80) << 1);		\
		CYCLES_PLUS_1;					\
		if(tmp1 & psr) {				\
			CYCLES_PLUS_1;				\
		}						\
	}
#endif

#undef STY_INST
#undef STX_INST

#ifdef ASM
# define STY_INST()		\
	ldil	l%dispatch,link		! \
	b	set_mem_yreg		! \
	ldo	r%dispatch(link),link

# define STX_INST()		\
	ldil	l%dispatch,link		! \
	b	set_mem_xreg		! \
	ldo	r%dispatch(link),link
#else /* C */
# define STY_INST()				\
	if(psr & 0x10) {			\
		SET_MEMORY8(arg, yreg);		\
	} else {				\
		SET_MEMORY16(arg, yreg);	\
	}
# define STX_INST()				\
	if(psr & 0x10) {			\
		SET_MEMORY8(arg, xreg);		\
	} else {				\
		SET_MEMORY16(arg, xreg);	\
	}
#endif

#ifndef ASM

# define C_LDX_ABS_Y()		\
	GET_ABS_INDEX_ADDR_FOR_RD(yreg);	\
	LDX_INST();

# define C_LDY_ABS_X()	\
	GET_ABS_INDEX_ADDR_FOR_RD(xreg);	\
	LDY_INST();

# define C_LDX_ABS()		\
	GET_ABS_ADDR();		\
	LDX_INST();

# define C_LDY_ABS()		\
	GET_ABS_ADDR();		\
	LDY_INST();

# define C_LDX_DLOC()		\
	GET_DLOC_ADDR();	\
	LDX_INST();

# define C_LDY_DLOC()		\
	GET_DLOC_ADDR();	\
	LDY_INST();

# define C_LDY_DLOC_X()		\
	GET_DLOC_X_ADDR();	\
	LDY_INST();

# define C_LDX_DLOC_Y()		\
	GET_DLOC_Y_ADDR();	\
	LDX_INST();

# define CP_INDEX_VAL(index_reg)	\
	arg = 0x100 - arg + index_reg;	\
	if((psr & 0x10) == 0) {		\
		arg += 0xff00;		\
		SET_NEG_ZERO16(arg & 0xffff);	\
		SET_CARRY16(arg);	\
	} else {			\
		SET_NEG_ZERO8(arg & 0xff);\
		SET_CARRY8(arg);	\
	}

# define CP_INDEX_LOAD(index_reg)	\
	if((psr & 0x10) != 0) {		\
		GET_MEMORY8(arg, arg);	\
	} else {			\
		GET_MEMORY16(arg, arg);	\
	}				\
	CP_INDEX_VAL(index_reg)

# define CPX_INST()		\
	CP_INDEX_LOAD(xreg);

# define CPY_INST()		\
	CP_INDEX_LOAD(yreg);

# define C_CPX_IMM()		\
	pc += 2;		\
	if((psr & 0x10) == 0) {	\
		GET_2BYTE_ARG;	\
		CYCLES_PLUS_1;	\
		pc++;		\
	} else {		\
		GET_1BYTE_ARG;	\
	}			\
	CP_INDEX_VAL(xreg);

# define C_CPY_IMM()		\
	pc += 2;		\
	if((psr & 0x10) == 0) {	\
		GET_2BYTE_ARG;	\
		CYCLES_PLUS_1;	\
		pc++;		\
	} else {		\
		GET_1BYTE_ARG;	\
	}			\
	CP_INDEX_VAL(yreg);

# define C_CPX_DLOC()		\
	GET_DLOC_ADDR();	\
	CPX_INST();

# define C_CPY_DLOC()		\
	GET_DLOC_ADDR();	\
	CPY_INST();

# define C_CPX_ABS()		\
	GET_ABS_ADDR();		\
	CPX_INST();

# define C_CPY_ABS()		\
	GET_ABS_ADDR();		\
	CPY_INST();

#endif




/* This is here to make sure all the macros expand to no instrs */
/*  if defs_instr_end_8 - start != 0, then something did expand */

#ifdef ASM
# ifdef ACC8
	.export defs_instr_end_8,data
defs_instr_end_8	.word	0
# else
	.export defs_instr_end_16,data
defs_instr_end_16	.word	0
# endif
#endif
