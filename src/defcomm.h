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
const char rcsdif_defcomm_h[] = "@(#)$Header: defcomm.h,v 1.75 97/09/13 23:49:30 kentd Exp $";
#endif

#define USE_XIMAGE_CHANGED

#if 0
# define CHECK_BREAKPOINTS
#endif

#define SHIFT_PER_CHANGE	3
#define CHANGE_SHIFT		(5 + SHIFT_PER_CHANGE)

#define SLOW_MEM_CH_SIZE	(0x10000 >> CHANGE_SHIFT)

#define MAXNUM_HEX_PER_LINE	32

#define MEM_SIZE_EXP	(4*1024*1024)
#define MEM_SIZE	(128*1024 + MEM_SIZE_EXP)

#define DEF_CYCLES	(40*1000)

#define HALT_STEP	0xca0
#define HALT_EVENT	0x600

#define MAX_BP_INDEX		0x100
#define MAX_BP_PER_INDEX	3	/* 4 word32s total = 16 bytes */
#define SIZE_BREAKPT_ENTRY_BITS	4	/* 16 bytes = 4 bits */


#define BANK_IO_BIT		31
#define BANK_SHADOW_BIT		30
#define BANK_SHADOW2_BIT	29
#define BANK_IO			(1 << (31 - BANK_IO_BIT))
#define BANK_SHADOW		(1 << (31 - BANK_SHADOW_BIT))
#define BANK_SHADOW2		(1 << (31 - BANK_SHADOW2_BIT))


#define BANK_BAD_MEM		0xff


#define LEN_FIFO_BUF	160
#define LEN_KBD_BUF	160

#define FIFO_OK         0x1
#define FIFO_INIT       0x2
#define FIFO_END        0x3
#define FIFO_40COLS     0x4
#define FIFO_80COLS     0x5
#define FIFO_SENDCHAR   0x6
#define FIFO_SENDKEY	0x7
#define FIFO_REFRESH    0x8

#define B_OP_SIZE	2
#define B_OP_D_SIZE	5
#define B_OP_DTYPE	12
#define SIZE_OP_DTYPE	7



#define ENGINE_FDBL_1		0x00
#define ENGINE_FCYCLES		0x00
#define ENGINE_FCYCLES_STOP	0x04
#define ENGINE_FPLUS_PTR	0x08
#define ENGINE_REG_KBANK	0x0c
#define ENGINE_REG_PC		0x10
#define ENGINE_REG_ACC		0x14
#define ENGINE_REG_XREG		0x18
#define ENGINE_REG_YREG		0x1c
#define ENGINE_REG_STACK	0x20
#define ENGINE_REG_DBANK	0x24
#define ENGINE_REG_DIRECT	0x28
#define ENGINE_REG_PSR		0x2c

#define LOG_PC_DBANK_KBANK_PC	0x00
#define LOG_PC_INSTR		0x04
#define LOG_PC_PSR_ACC		0x08
#define LOG_PC_XREG_YREG	0x0c
#define LOG_PC_STACK_DIRECT	0x10

#define LOG_PC_SIZE		0x14


#define FPLUS_DBL_1		0x00
#define FPLUS_1			0x00
#define FPLUS_2			0x04
#define FPLUS_DBL_2		0x08
#define FPLUS_3			0x08
#define FPLUS_X_MINUS_1		0x0c

#define RET_BREAK	0x1
#define RET_COP		0x2
#define RET_WDM		0x3
#define RET_MVP		0x4
#define RET_MVN		0x5
#define RET_WAI		0x6
#define RET_STP		0x7
#define RET_ADD_DEC_8	0x8
#define RET_ADD_DEC_16	0x9
#define RET_C700	0xa
#define RET_C70A	0xb
#define RET_C70D	0xc
#define RET_IRQ		0xd


#define MODE_BORDER		0
#define MODE_TEXT		1
#define MODE_GR			2
#define MODE_HGR		3
#define MODE_SUPER_HIRES	4

#define BIT_ALL_STAT_TEXT		0
#define BIT_ALL_STAT_VID80		1
#define BIT_ALL_STAT_ST80		2
#define BIT_ALL_STAT_COLOR_C021		3
#define BIT_ALL_STAT_MIX_T_GR		4
#define BIT_ALL_STAT_DIS_COLOR_DHIRES	5	/* special val, c029 */
#define BIT_ALL_STAT_PAGE2		6	/* special val, statereg */
#define BIT_ALL_STAT_SUPER_HIRES	7	/* special, c029 */
#define BIT_ALL_STAT_HIRES		8
#define BIT_ALL_STAT_ANNUNC3		9
#define BIT_ALL_STAT_BG_COLOR		10	/* 4 bits */
#define BIT_ALL_STAT_TEXT_COLOR		14	/* 4 bits */
						/* Text must be just above */
						/* bg to match c022 reg */
#define BIT_ALL_STAT_ALTCHARSET		18
#define BIT_ALL_STAT_FLASH_STATE	19
#define BIT_ALL_STAT_A2VID_PALETTE	20	/* 4 bits */

#define ALL_STAT_SUPER_HIRES		(1 << (BIT_ALL_STAT_SUPER_HIRES))
#define ALL_STAT_TEXT			(1 << (BIT_ALL_STAT_TEXT))
#define ALL_STAT_VID80			(1 << (BIT_ALL_STAT_VID80))
#define ALL_STAT_PAGE2			(1 << (BIT_ALL_STAT_PAGE2))
#define ALL_STAT_ST80			(1 << (BIT_ALL_STAT_ST80))
#define ALL_STAT_COLOR_C021		(1 << (BIT_ALL_STAT_COLOR_C021))
#define ALL_STAT_DIS_COLOR_DHIRES	(1 << (BIT_ALL_STAT_DIS_COLOR_DHIRES))
#define ALL_STAT_MIX_T_GR		(1 << (BIT_ALL_STAT_MIX_T_GR))
#define ALL_STAT_HIRES			(1 << (BIT_ALL_STAT_HIRES))
#define ALL_STAT_ANNUNC3		(1 << (BIT_ALL_STAT_ANNUNC3))
#define ALL_STAT_TEXT_COLOR		(0xf << (BIT_ALL_STAT_TEXT_COLOR))
#define ALL_STAT_BG_COLOR		(0xf << (BIT_ALL_STAT_BG_COLOR))
#define ALL_STAT_ALTCHARSET		(1 << (BIT_ALL_STAT_ALTCHARSET))
#define ALL_STAT_FLASH_STATE		(1 << (BIT_ALL_STAT_FLASH_STATE))
#define ALL_STAT_A2VID_PALETTE		(0xf << (BIT_ALL_STAT_A2VID_PALETTE))

#define BORDER_WIDTH		32

#define EFF_BORDER_WIDTH	(BORDER_WIDTH + (640-560))

#define BASE_MARGIN_TOP		32
#define BASE_MARGIN_BOTTOM	32
#define BASE_MARGIN_LEFT	BORDER_WIDTH
#define BASE_MARGIN_RIGHT	BORDER_WIDTH

#define A2_BORDER_TOP		0
#define A2_BORDER_BOTTOM	0
#define A2_BORDER_LEFT		0
#define A2_BORDER_RIGHT		0
#define A2_WINDOW_WIDTH		(640 + A2_BORDER_LEFT + A2_BORDER_RIGHT)
#define A2_WINDOW_HEIGHT	(400 + A2_BORDER_TOP + A2_BORDER_BOTTOM)

#if 0
#define A2_DATA_OFF		(A2_BORDER_TOP*A2_WINDOW_WIDTH + A2_BORDER_LEFT)
#endif

#define X_A2_WINDOW_WIDTH	(A2_WINDOW_WIDTH + BASE_MARGIN_LEFT + \
							BASE_MARGIN_RIGHT)
#define X_A2_WINDOW_HEIGHT	(A2_WINDOW_HEIGHT + BASE_MARGIN_TOP + \
							BASE_MARGIN_BOTTOM)

#define STATUS_WINDOW_HEIGHT	(7*13)

#define BASE_WINDOW_WIDTH	(X_A2_WINDOW_WIDTH)
#define BASE_WINDOW_HEIGHT	(X_A2_WINDOW_HEIGHT + STATUS_WINDOW_HEIGHT)


#define A2_BORDER_COLOR_NUM	0xfe

#if 0
#define A2_TEXT_COLOR_ALT_NUM	0x01
#define A2_BG_COLOR_ALT_NUM	0x00
#define A2_TEXT_COLOR_PRIM_NUM	0x02
#define A2_BG_COLOR_PRIM_NUM	0x00
#define A2_TEXT_COLOR_FLASH_NUM	0x0c
#define A2_BG_COLOR_FLASH_NUM	0x08
#endif

