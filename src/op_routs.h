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
	.export rcsdif_op_routs_h,data
rcsdif_op_routs_h
	.stringz "@(#)$Header: op_routs.h,v 1.27 97/03/31 22:55:03 kentd Exp $"
	.code
# endif

	.import	get_mem_b0_16,code
	.import	get_mem_b0_8,code

	.export	op_routs_start,data
op_routs_start	.word	0
#endif /* ASM */

#ifdef ASM
# define CMP_INDEX_REG_MEAT8(index_reg)		\
	extru	ret0,31,8,ret0			! \
	ldi	0xff,scratch3			! \
	subi	0x100,ret0,ret0			! \
	add	index_reg,ret0,ret0		! \
	extru	ret0,23,1,scratch1		! \
	and	ret0,scratch3,zero		! \
	extru	ret0,24,1,neg			! \
	b	dispatch			! \
	dep	scratch1,31,1,psr

# define CMP_INDEX_REG_MEAT16(index_reg)		\
	extru	ret0,31,16,ret0			! \
	ldil	l%0x10000,scratch2		! \
	zdepi	-1,31,16,scratch3		! \
	sub	scratch2,ret0,ret0		! \
	add	index_reg,ret0,ret0		! \
	extru	ret0,15,1,scratch1		! \
	and	ret0,scratch3,zero		! \
	extru	ret0,16,1,neg			! \
	b	dispatch			! \
	dep	scratch1,31,1,psr

# define CMP_INDEX_REG_LOAD(new_label, index_reg)	\
	bb,>=,n	psr,27,new_label		! \
	bl	get_mem_long_8,link		! \
	nop					! \
	CMP_INDEX_REG_MEAT8(index_reg)		! \
	.label	new_label			! \
	bl	get_mem_long_16,link		! \
	nop					! \
	CMP_INDEX_REG_MEAT16(index_reg)
#endif


#ifdef ASM
#define GET_DLOC_X_IND_WR()		\
	CYCLES_PLUS_1			! \
	add	xreg,arg0,arg0		! \
	addi	2,pc,pc			! \
	add	direct,arg0,arg0	! \
	extru,=	direct,31,8,0		! \
	CYCLES_PLUS_1			! \
	bl	get_mem_b0_direct_page_16,link	! \
	extru	arg0,31,16,arg0		! \
	copy	ret0,arg0		! \
	dep	dbank,15,8,arg0
#else /* C */
# define GET_DLOC_X_IND_WR()			\
	arg = arg + xreg + direct;		\
	pc += 2;				\
	if(direct & 0xff) {			\
		CYCLES_PLUS_1;			\
	}					\
	arg = get_mem_direct_page_16(arg);	\
	CYCLES_PLUS_1;				\
	arg = (dbank << 16) + arg;
#endif


#ifdef ASM
# define GET_DLOC_X_IND_ADDR()		\
	extru	ret0,31,8,arg0		! \
	GET_DLOC_X_IND_WR()
#else /* C */
# define GET_DLOC_X_IND_ADDR()		\
	CYCLES_PLUS_4;			\
	GET_DLOC_X_IND_WR()
#endif

#ifdef ASM
# define GET_DISP8_S_WR()		\
	CYCLES_PLUS_1			! \
	add	stack,arg0,arg0		! \
	addi	2,pc,pc			! \
	extru	arg0,31,16,arg0
#else /* C */
#define GET_DISP8_S_WR()		\
	arg = (arg + stack) & 0xffff;	\
	pc += 2;
#endif


#ifdef ASM
# define GET_DISP8_S_ADDR()		\
	extru	ret0,31,8,arg0		! \
	GET_DISP8_S_WR()
#else /* C */
# define GET_DISP8_S_ADDR()		\
	CYCLES_PLUS_2;			\
	GET_DISP8_S_WR()
#endif

#ifdef ASM
# define GET_DLOC_WR()			\
	addi	2,pc,pc			! \
	extru,=	direct,31,8,0		! \
	CYCLES_PLUS_1			! \
	add	direct,arg0,arg0	! \
	extru	arg0,31,16,arg0
#else /* C */
# define GET_DLOC_WR()			\
	arg = (arg + direct) & 0xffff;	\
	if(direct & 0xff) {		\
		CYCLES_PLUS_1;		\
	}				\
	pc += 2;
#endif

#ifdef ASM
# define GET_DLOC_ADDR()		\
	extru	ret0,31,8,arg0		! \
	GET_DLOC_WR()
#else /* C */
# define GET_DLOC_ADDR()		\
	CYCLES_PLUS_1;			\
	GET_DLOC_WR()
#endif

#ifdef ASM
# define GET_DLOC_L_IND_WR()		\
	add	direct,arg0,arg0	! \
	extru,=	direct,31,8,0		! \
	CYCLES_PLUS_1			! \
	addi	2,pc,pc			! \
	bl	get_mem_b0_24,link	! \
	extru	arg0,31,16,arg0		! \
	copy	ret0,arg0
#else /* C */
# define GET_DLOC_L_IND_WR()		\
	arg = (arg + direct) & 0xffff;	\
	if(direct & 0xff) {		\
		CYCLES_PLUS_1;		\
	}				\
	pc += 2;			\
	arg = get_mem_24(arg);
#endif


#ifdef ASM
# define GET_DLOC_L_IND_ADDR()		\
	extru	ret0,31,8,arg0		! \
	GET_DLOC_L_IND_WR()
#else /* C */
# define GET_DLOC_L_IND_ADDR()		\
	CYCLES_PLUS_4;			\
	GET_DLOC_L_IND_WR()
#endif

#ifdef ASM
# define GET_DLOC_IND_Y_ADDR_FOR_WR()	\
	extru	ret0,31,8,arg0		! \
	CYCLES_PLUS_1			! \
	GET_DLOC_IND_Y_WR_SPECIAL()
#else /* C */
# define GET_DLOC_IND_Y_ADDR()		\
	CYCLES_PLUS_4;			\
	GET_DLOC_IND_Y_WR()
#endif


#ifdef ASM
# define GET_DLOC_IND_WR()		\
	addi	2,pc,pc			! \
	add	direct,arg0,arg0	! \
	extru,=	direct,31,8,0		! \
	CYCLES_PLUS_1			! \
	bl	get_mem_b0_direct_page_16,link	! \
	extru	arg0,31,16,arg0		! \
	copy	ret0,arg0		! \
	dep	dbank,15,8,arg0
#else /* C */
# define GET_DLOC_IND_WR()		\
	arg = (arg + direct) & 0xffff;	\
	if(direct & 0xff) {		\
		CYCLES_PLUS_1;		\
	}				\
	arg = get_mem_direct_page_16(arg);	\
	arg = (dbank << 16) + arg;
#endif


#ifdef ASM
# define GET_DLOC_IND_ADDR()		\
	extru	ret0,31,8,arg0		! \
	GET_DLOC_IND_WR()
#else
# define GET_DLOC_IND_ADDR()		\
	CYCLES_PLUS_3;			\
	GET_DLOC_IND_WR();
#endif

#ifdef ASM
#define GET_DLOC_INDEX_WR(index_reg)	\
	CYCLES_PLUS_1			! \
	add	index_reg,arg0,arg0	! \
	extru	direct,23,8,scratch1	! \
	addi	2,pc,pc			! \
	extru,=	direct,31,8,0		! \
	CYCLES_PLUS_1			! \
	bb,>=	psr,23,.+16		! \
/* 4*/	add	direct,arg0,arg0	! \
/* 8*/	extru,<> direct,31,8,0		! \
/*12*/	dep	scratch1,23,8,arg0	! \
/*16*/	extru	arg0,31,16,arg0

#define GET_DLOC_Y_WR()			\
	GET_DLOC_INDEX_WR(yreg)

#define GET_DLOC_X_WR()			\
	GET_DLOC_INDEX_WR(xreg)

#define GET_DLOC_Y_ADDR()			\
	extru	ret0,31,8,arg0		! \
	GET_DLOC_Y_WR()

# define GET_DLOC_X_ADDR()			\
	extru	ret0,31,8,arg0		! \
	GET_DLOC_X_WR()

#else
# define GET_DLOC_INDEX_WR(index_reg)	\
	arg = arg + index_reg;		\
	pc += 2;			\
	if(direct & 0xff) {		\
		CYCLES_PLUS_1;		\
	}				\
	if((psr & 0x100) && ((direct & 0xff) == 0)) {	\
		arg = (arg & 0xff);	\
	}				\
	arg = (arg + direct) & 0xffff;

# define GET_DLOC_X_WR()	\
	GET_DLOC_INDEX_WR(xreg)
# define GET_DLOC_Y_WR()	\
	GET_DLOC_INDEX_WR(yreg)

# define GET_DLOC_X_ADDR()	\
	CYCLES_PLUS_2;		\
	GET_DLOC_INDEX_WR(xreg)

# define GET_DLOC_Y_ADDR()	\
	CYCLES_PLUS_2;		\
	GET_DLOC_INDEX_WR(yreg)
#endif


#ifdef ASM
# define GET_DISP8_S_IND_Y_WR()		\
	add	stack,arg0,arg0		! \
	bl	get_mem_b0_16,link	! \
	extru	arg0,31,16,arg0		! \
	dep	dbank,15,16,ret0	! \
	CYCLES_PLUS_2			! \
	add	ret0,yreg,arg0		! \
	addi	2,pc,pc			! \
	extru	arg0,31,24,arg0

# define GET_DISP8_S_IND_Y_ADDR()	\
	extru	ret0,31,8,arg0		! \
	GET_DISP8_S_IND_Y_WR()
#else /* C */

# define GET_DISP8_S_IND_Y_WR()		\
	arg = (stack + arg) & 0xffff;	\
	arg = get_memory16(arg) + (dbank << 16);	\
	pc += 2;			\
	arg = (arg + yreg) & 0xffffff;

# define GET_DISP8_S_IND_Y_ADDR()	\
	CYCLES_PLUS_5;			\
	GET_DISP8_S_IND_Y_WR()
#endif


#ifdef ASM
# define GET_DLOC_L_IND_Y_WR()		\
	addi	2,pc,pc			! \
	add	direct,arg0,arg0	! \
	bl	get_mem_b0_24,link	! \
	extru	arg0,31,16,arg0		! \
	add	ret0,yreg,arg0		! \
	extru,=	direct,31,8,0		! \
	CYCLES_PLUS_1			! \
	extru	arg0,31,24,arg0

# define GET_DLOC_L_IND_Y_ADDR()	\
	extru	ret0,31,8,arg0		! \
	GET_DLOC_L_IND_Y_WR()
#else /* C */

# define GET_DLOC_L_IND_Y_WR()		\
	arg = (direct + arg) & 0xffff;	\
	if(direct & 0xff) {		\
		CYCLES_PLUS_1;		\
	}				\
	arg = get_memory24(arg);	\
	pc += 2;			\
	arg = (arg + yreg) & 0xffffff;

# define GET_DLOC_L_IND_Y_ADDR()	\
	CYCLES_PLUS_4;			\
	GET_DLOC_L_IND_Y_WR()
#endif


#ifdef ASM
# define GET_ABS_ADDR()			\
	extru	ret0,31,16,arg0		! \
	CYCLES_PLUS_1			! \
	dep	dbank,15,8,arg0		! \
	addi	3,pc,pc

# define GET_LONG_ADDR()		\
	extru	ret0,31,24,arg0		! \
	CYCLES_PLUS_2			! \
	addi	4,pc,pc
#else /* C */

# define GET_ABS_ADDR()			\
	CYCLES_PLUS_2;			\
	arg = arg + (dbank << 16);	\
	pc += 3;

# define GET_LONG_ADDR()		\
	CYCLES_PLUS_3;			\
	pc += 4;
#endif

#ifdef ASM
#define GET_ABS_INDEX_ADDR_FOR_WR(index_reg)	\
	extru	ret0,31,16,arg0		! \
	addi	3,pc,pc			! \
	dep	dbank,15,8,arg0		! \
	CYCLES_PLUS_2			! \
	add	arg0,index_reg,arg0	! \
	extru	arg0,31,24,arg0

#define GET_LONG_X_ADDR_FOR_WR()	\
	extru	ret0,31,24,arg0		! \
	addi	4,pc,pc			! \
	add	arg0,xreg,arg0		! \
	CYCLES_PLUS_2			! \
	extru	arg0,31,24,arg0
#else /* C */

#define GET_ABS_X_ADDR_FOR_WR()		\
	arg = arg + (dbank << 16);	\
	pc += 3;			\
	CYCLES_PLUS_3;			\
	arg = (arg + xreg) & 0xffffff;

#define GET_LONG_X_ADDR_FOR_WR()		\
	pc += 4;			\
	arg = (arg + xreg) & 0xffffff;	\
	CYCLES_PLUS_3;

#endif /* ASM */


#ifdef ASM
	.export	op_routs_end,data
op_routs_end	.word	0


#define GET_DLOC_IND_Y_WR_SPECIAL()	\
	add	direct,arg0,arg0	! \
	extru,=	direct,31,8,0		! \
	CYCLES_PLUS_1			! \
	bl	get_mem_b0_direct_page_16,link	! \
	extru	arg0,31,16,arg0		! \
	dep	dbank,15,8,ret0		! \
	addi	2,pc,pc			! \
	add	yreg,ret0,arg0			/* don't change this instr */
						/*  or add any after */
						/*  to preserve ret0 & arg0 */


/* cycle calc:  if yreg is 16bit or carry into 2nd byte, inc cycle */
/* So, if y==16bit, add 1.  If x==8bit, add 1 if carry */
get_dloc_ind_y_rd_8
	stw	link,STACK_SAVE_OP_LINK(sp)
	GET_DLOC_IND_Y_WR_SPECIAL()
	xor	arg0,ret0,scratch1
	extru,=	psr,27,1,0
	extru,=	scratch1,23,8,0
	CYCLES_PLUS_1
	b	get_mem_long_8
	ldw	STACK_SAVE_OP_LINK(sp),link

get_dloc_ind_y_rd_16
	stw	link,STACK_SAVE_OP_LINK(sp)
	GET_DLOC_IND_Y_WR_SPECIAL()
	xor	arg0,ret0,scratch1
	extru,=	psr,27,1,0
	extru,=	scratch1,23,8,0
	CYCLES_PLUS_1
	b	get_mem_long_16
	ldw	STACK_SAVE_OP_LINK(sp),link



#endif /* ASM */

