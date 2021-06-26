const char rcsid_moremem_c[] = "@(#)$KmKId: moremem.c,v 1.269 2021-06-26 01:42:26+00 kentd Exp $";

/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002-2021 by Kent Dickey		*/
/*									*/
/*	This code is covered by the GNU GPL v3				*/
/*	See the file COPYING.txt or https://www.gnu.org/licenses/	*/
/*	This program is provided with no warranty			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

#include "defc.h"

extern char g_kegs_version_str[];

extern byte *g_memory_ptr;
extern byte *g_dummy_memory1_ptr;
extern byte *g_slow_memory_ptr;
extern byte *g_rom_fc_ff_ptr;
extern byte *g_rom_cards_ptr;

extern word32 slow_mem_changed[];

extern int g_num_breakpoints;
extern Break_point g_break_pts[];

extern Page_info page_info_rd_wr[];

extern int Verbose;
extern int g_rom_version;
extern int g_user_page2_shadow;


/* from iwm.c */
int	g_num_shadow_all_banks = 0;

#define IOR(val) ( (val) ? 0x80 : 0x00 )


extern int g_cur_a2_stat;

int	g_em_emubyte_cnt = 0;
int	g_paddle_buttons = 0;
int	g_irq_pending = 0;

int	g_c023_val = 0;
int	g_c029_val_some = 0x41;
int	g_c02b_val = 0x08;
int	g_c02d_int_crom = 0;
int	g_c031_disk35 = 0;
int	g_c033_data = 0;
int	g_c034_val = 0;
int	g_c035_shadow_reg = 0x08;
int	g_c036_val_speed = 0x80;
word32	g_c03ef_doc_ptr = 0;
int	g_c041_val = 0;		/* C041_EN_25SEC_INTS, C041_EN_MOVE_INTS */
int	g_c046_val = 0;
int	g_c05x_annuncs = 0;
int	g_c068_statereg = 0;
int	g_c08x_wrdefram = 0;
int	g_zipgs_unlock = 0;
int	g_zipgs_reg_c059 = 0x5f;
	// 7=LC cache dis, 6==5ms paddle del en, 5==5ms ext del en,
	// 4==5ms c02e enab, 3==CPS follow enab, 2-0: 111
int	g_zipgs_reg_c05a = 0x0f;
	// 7:4 = current ZIP speed, 0=100%, 1=93.75%, F=6.25%
	// 3:0: always 1111
int	g_zipgs_reg_c05b = 0x40;
	// 7==1ms clock, 6==cshupd: tag data at c05f updated
	// 5==LC cache disable, 4==bd is disabled, 3==delay in effect,
	// 2==rombank, 1-0==ram size (00:8K, 01=16K, 10=32K, 11=64K)
int	g_zipgs_reg_c05c = 0x00;
	// 7:1==slot delay enable (for 52-54ms), 0==speaker 5ms delay

word32	g_c06c_latched_cyc = 0;

#define EMUSTATE(a)	{ #a, &a }

Emustate_intlist g_emustate_intlist[] = {
	EMUSTATE(g_cur_a2_stat),
	EMUSTATE(g_paddle_buttons),

	EMUSTATE(g_em_emubyte_cnt),
	EMUSTATE(g_irq_pending),
	EMUSTATE(g_c023_val),
	EMUSTATE(g_c029_val_some),
	EMUSTATE(g_c02b_val),
	EMUSTATE(g_c02d_int_crom),
	EMUSTATE(g_c031_disk35),
	EMUSTATE(g_c033_data),
	EMUSTATE(g_c034_val),
	EMUSTATE(g_c035_shadow_reg),
	EMUSTATE(g_c036_val_speed),
	EMUSTATE(g_c041_val),
	EMUSTATE(g_c046_val),
	EMUSTATE(g_c05x_annuncs),
	EMUSTATE(g_c068_statereg),
	EMUSTATE(g_c08x_wrdefram),
	EMUSTATE(g_zipgs_unlock),
	EMUSTATE(g_zipgs_reg_c059),
	EMUSTATE(g_zipgs_reg_c05a),
	EMUSTATE(g_zipgs_reg_c05b),
	EMUSTATE(g_zipgs_reg_c05c),
	{ 0, 0, }
};

extern double g_paddle_trig_dcycs;
extern double g_last_vbl_dcycs;

Emustate_dbllist g_emustate_dbllist[] = {
	EMUSTATE(g_paddle_trig_dcycs),
	EMUSTATE(g_last_vbl_dcycs),
	{ 0, 0, }
};

extern word32 g_mem_size_total;

Emustate_word32list g_emustate_word32list[] = {
	EMUSTATE(g_mem_size_total),
	EMUSTATE(g_c03ef_doc_ptr),
	{ 0, 0, }
};


#define UNIMPL_READ	\
	halt_printf("UNIMP READ to addr %08x\n", loc);	\
	return 0;

#define UNIMPL_WRITE	\
	halt_printf("UNIMP WRITE to addr %08x, val: %04x\n", loc, val);	\
	return;

void
fixup_brks()
{
	word32	page, tmp, tmp2, end_page;
	Pg_info	val;
	int	is_wr_only, num;
	int	i;

	num = g_num_breakpoints;
	for(i = 0; i < num; i++) {
		page = (g_break_pts[i].start_addr >> 8) & 0xffff;
		end_page = (g_break_pts[i].end_addr >> 8) & 0xffff;
		is_wr_only = (g_break_pts[i].start_addr >> 24) & 1;
		while(page <= end_page) {
			if(!is_wr_only) {
				val = GET_PAGE_INFO_RD(page);
				tmp = PTR2WORD(val) & 0xff;
				tmp2 = tmp | BANK_IO_TMP | BANK_BREAK;
				SET_PAGE_INFO_RD(page, val - tmp + tmp2);
			}
			val = GET_PAGE_INFO_WR(page);
			tmp = PTR2WORD(val) & 0xff;
			tmp2 = tmp | BANK_IO_TMP | BANK_BREAK;
			SET_PAGE_INFO_WR(page, val - tmp + tmp2);
			page++;
		}
	}
}

void
fixup_hires_on()
{
	if((g_cur_a2_stat & ALL_STAT_ST80) == 0) {
		return;
	}

	fixup_bank0_2000_4000();
	fixup_brks();
}

void
fixup_bank0_2000_4000()
{
	byte	*mem0rd;
	byte	*mem0wr;

	mem0rd = &(g_memory_ptr[0x2000]);
	mem0wr = mem0rd;
	if((g_cur_a2_stat & ALL_STAT_ST80) && (g_cur_a2_stat & ALL_STAT_HIRES)){
		if(g_cur_a2_stat & ALL_STAT_PAGE2) {
			mem0rd += 0x10000;
			mem0wr += 0x10000;
			if((g_c035_shadow_reg & 0x12) == 0 ||
					(g_c035_shadow_reg & 0x8) == 0) {
				mem0wr += BANK_SHADOW2;
			}
		} else if((g_c035_shadow_reg & 0x02) == 0) {
			mem0wr += BANK_SHADOW;
		}
	} else {
		if(RAMRD) {
			mem0rd += 0x10000;
		}
		if(RAMWRT) {
			mem0wr += 0x10000;
			if((g_c035_shadow_reg & 0x12) == 0 ||
					(g_c035_shadow_reg & 0x8) == 0) {
				mem0wr += BANK_SHADOW2;
			}
		} else if((g_c035_shadow_reg & 0x02) == 0) {
			mem0wr += BANK_SHADOW;
		}
	}

	fixup_any_bank_any_page(0x20, 0x20, mem0rd, mem0wr);
}

void
fixup_bank0_0400_0800()
{
	byte	*mem0rd;
	byte	*mem0wr;
	int	shadow;

	mem0rd = &(g_memory_ptr[0x400]);
	mem0wr = mem0rd;
	shadow = BANK_SHADOW;
	if(g_cur_a2_stat & ALL_STAT_ST80) {
		if(g_cur_a2_stat & ALL_STAT_PAGE2) {
			shadow = BANK_SHADOW2;
			mem0rd += 0x10000;
			mem0wr += 0x10000;
		}
	} else {
		if(RAMWRT) {
			shadow = BANK_SHADOW2;
			mem0wr += 0x10000;
		}
		if(RAMRD) {
			mem0rd += 0x10000;
		}
	}
	if((g_c035_shadow_reg & 0x01) == 0) {
		mem0wr += shadow;
	}

	fixup_any_bank_any_page(0x4, 4, mem0rd, mem0wr);
}

void
fixup_any_bank_any_page(int start_page, int num_pages, byte *mem0rd,
		byte *mem0wr)
{
	int	i;

	for(i = 0; i < num_pages; i++) {
		SET_PAGE_INFO_RD(i + start_page, mem0rd);
		mem0rd += 0x100;
	}

	for(i = 0; i < num_pages; i++) {
		SET_PAGE_INFO_WR(i + start_page, mem0wr);
		mem0wr += 0x100;
	}

}

void
fixup_intcx()
{
	byte	*rom10000;
	byte	*rom_inc;
	word32	intcx, mask;
	int	no_io_shadow, off, start_k;
	int	j, k;

	rom10000 = &(g_rom_fc_ff_ptr[0x30000]);

	no_io_shadow = (g_c035_shadow_reg & 0x40);

	start_k = 0;
	if(no_io_shadow) {
		/* if not shadowing, banks 0 and 1 are not affected by intcx */
		start_k = 2;
	}

	intcx = (g_c068_statereg & 1);	// set means use internal rom
	// printf("fixup_intcx, intcx:%d, no_io:%d, c02d:%02x\n", intcx,
	//				no_io_shadow, g_c02d_int_crom);
	for(k = start_k; k < 4; k++) {
		off = k;
		if(k >= 2) {
			off += (0xe0 - 2);
		}
		/* step off through 0x00, 0x01, 0xe0, 0xe1 */

		off = off << 8;
		SET_PAGE_INFO_RD(0xc0 + off, SET_BANK_IO);

		for(j = 0xc1; j < 0xc8; j++) {
			mask = 1 << (j & 0xf);
			rom_inc = SET_BANK_IO;
			if(((g_c02d_int_crom & mask) == 0) || intcx) {
				rom_inc = rom10000 + (j << 8);
			} else {
				// User-slot rom
				rom_inc = &(g_rom_cards_ptr[0]) +
							((j - 0xc0) << 8);
				if(j == 0xc4) {		// Mockingboard
					rom_inc = SET_BANK_IO;
				}
			}
			SET_PAGE_INFO_RD(j + off, rom_inc);
		}
		for(j = 0xc8; j < 0xd0; j++) {
			/* c800 - cfff */
			if(((g_c02d_int_crom & (1 << 3)) == 0) || intcx) {
				rom_inc = rom10000 + (j << 8);
			} else {
				/* c800 space not necessarily mapped */
				//rom_inc = rom10000 + (j << 8);
				rom_inc = SET_BANK_IO;
			}
			SET_PAGE_INFO_RD(j + off, rom_inc);
		}
		for(j = 0xc0; j < 0xd0; j++) {
			SET_PAGE_INFO_WR(j + off, SET_BANK_IO);
		}
	}

	if(!no_io_shadow) {
		SET_PAGE_INFO_RD(0xc7, SET_BANK_IO);		/* smartport */
	}

	fixup_brks();
}

void
fixup_wrdefram(int new_wrdefram)
{
	byte	*mem0wr;
	byte	*wrptr;
	int	j;

	g_c08x_wrdefram = new_wrdefram;
	if(g_c035_shadow_reg & 0x40) {
		/* do nothing */
		return;
	}

	/* if shadowing, banks 0 and 1 are affected by wrdefram */
	mem0wr = &(g_memory_ptr[0]);
	if(!new_wrdefram) {
		mem0wr += (BANK_IO_TMP | BANK_IO2_TMP);
	}

	wrptr = mem0wr + 0x1e000;
	for(j = 0x1e0; j < 0x200; j++) {
		SET_PAGE_INFO_WR(j, wrptr);
		wrptr += 0x100;
	}

	wrptr = mem0wr + 0x0e000;
	if(ALTZP) {
		wrptr += 0x10000;
	}
	for(j = 0xe0; j < 0x100; j++) {
		SET_PAGE_INFO_WR(j, wrptr);
		wrptr += 0x100;
	}

	wrptr = mem0wr + 0x1d000;
	if(! LCBANK2) {
		wrptr -= 0x1000;
	}
	for(j = 0x1d0; j < 0x1e0; j++) {
		SET_PAGE_INFO_WR(j, wrptr);
		wrptr += 0x100;
	}

	wrptr = mem0wr + 0xd000;
	if(! LCBANK2) {
		wrptr -= 0x1000;
	}
	if(ALTZP) {
		wrptr += 0x10000;
	}
	for(j = 0xd0; j < 0xe0; j++) {
		SET_PAGE_INFO_WR(j, wrptr);
		wrptr += 0x100;
	}

	fixup_brks();
}

void
fixup_st80col(double dcycs)
{
	int	cur_a2_stat;

	cur_a2_stat = g_cur_a2_stat;

	fixup_bank0_0400_0800();

	if(cur_a2_stat & ALL_STAT_HIRES) {
		/* fixup no matter what PAGE2 since PAGE2 and RAMRD/WR */
		/*  can work against each other */
		fixup_bank0_2000_4000();
	}

	if(cur_a2_stat & ALL_STAT_PAGE2) {
		change_display_mode(dcycs);
	}

	fixup_brks();
}

void
fixup_altzp()
{
	byte	*mem0rd, *mem0wr;
	int	rdrom, c08x_wrdefram;
	int	altzp;

	altzp = ALTZP;
	mem0rd = &(g_memory_ptr[0]);
	if(altzp) {
		mem0rd += 0x10000;
	}
	SET_PAGE_INFO_RD(0, mem0rd);
	SET_PAGE_INFO_RD(1, mem0rd + 0x100);
	SET_PAGE_INFO_WR(0, mem0rd);
	SET_PAGE_INFO_WR(1, mem0rd + 0x100);

	mem0rd = &(g_memory_ptr[0xd000]);
	mem0wr = mem0rd;
	c08x_wrdefram = g_c08x_wrdefram;
	rdrom = RDROM;

	if(g_c035_shadow_reg & 0x40) {
		if(ALTZP) {
			mem0rd += 0x10000;
		}
		fixup_any_bank_any_page(0xd0, 0x10, mem0rd - 0x1000,
						mem0rd - 0x1000);
		c08x_wrdefram = 1;
		rdrom = 0;
	} else {
		if(!c08x_wrdefram) {
			mem0wr += (BANK_IO_TMP | BANK_IO2_TMP);
		}
		if(ALTZP) {
			mem0rd += 0x10000;
			mem0wr += 0x10000;
		}
		if(! LCBANK2) {
			mem0rd -= 0x1000;
			mem0wr -= 0x1000;
		}
		if(rdrom) {
			mem0rd = &(g_rom_fc_ff_ptr[0x3d000]);
		}
		fixup_any_bank_any_page(0xd0, 0x10, mem0rd, mem0wr);
	}

	mem0rd = &(g_memory_ptr[0xe000]);
	mem0wr = mem0rd;
	if(!c08x_wrdefram) {
		mem0wr += (BANK_IO_TMP | BANK_IO2_TMP);
	}
	if(ALTZP) {
		mem0rd += 0x10000;
		mem0wr += 0x10000;
	}
	if(rdrom) {
		mem0rd = &(g_rom_fc_ff_ptr[0x3e000]);
	}
	fixup_any_bank_any_page(0xe0, 0x20, mem0rd, mem0wr);

	/* No need for fixup_brks since called from set_statereg() */
}

void
fixup_page2(double dcycs)
{
	if((g_cur_a2_stat & ALL_STAT_ST80)) {
		fixup_bank0_0400_0800();
		if((g_cur_a2_stat & ALL_STAT_HIRES)) {
			fixup_bank0_2000_4000();
		}
	} else {
		change_display_mode(dcycs);
	}
}

void
fixup_ramrd()
{
	byte	*mem0rd;
	int	cur_a2_stat;
	int	j;

	cur_a2_stat = g_cur_a2_stat;

	if((cur_a2_stat & ALL_STAT_ST80) == 0) {
		fixup_bank0_0400_0800();
	}
	if( ((cur_a2_stat & ALL_STAT_ST80) == 0) ||
				((cur_a2_stat & ALL_STAT_HIRES) == 0) ) {
		fixup_bank0_2000_4000();
	}

	mem0rd = &(g_memory_ptr[0x0000]);
	if(RAMRD) {
		mem0rd += 0x10000;
	}

	SET_PAGE_INFO_RD(2, mem0rd + 0x200);
	SET_PAGE_INFO_RD(3, mem0rd + 0x300);

	for(j = 8; j < 0x20; j++) {
		SET_PAGE_INFO_RD(j, mem0rd + j*0x100);
	}

	for(j = 0x40; j < 0xc0; j++) {
		SET_PAGE_INFO_RD(j, mem0rd + j*0x100);
	}

	/* No need for fixup_brks since only called from set_statereg() */
}

void
fixup_ramwrt()
{
	byte	*mem0wr;
	int	cur_a2_stat;
	int	shadow;
	int	ramwrt;
	int	j;

	cur_a2_stat = g_cur_a2_stat;

	if((cur_a2_stat & ALL_STAT_ST80) == 0) {
		fixup_bank0_0400_0800();
	}
	if( ((cur_a2_stat & ALL_STAT_ST80) == 0) ||
				((cur_a2_stat & ALL_STAT_HIRES) == 0) ) {
		fixup_bank0_2000_4000();
	}

	mem0wr = &(g_memory_ptr[0x0000]);
	ramwrt = RAMWRT;
	if(ramwrt) {
		mem0wr += 0x10000;
	}

	SET_PAGE_INFO_WR(2, mem0wr + 0x200);
	SET_PAGE_INFO_WR(3, mem0wr + 0x300);

	shadow = BANK_SHADOW;
	if(ramwrt) {
		shadow = BANK_SHADOW2;
	}
	if( ((g_c035_shadow_reg & 0x20) != 0) ||
			((g_rom_version == 1) && !g_user_page2_shadow)) {
		shadow = 0;
	}
	for(j = 8; j < 0x0c; j++) {
		SET_PAGE_INFO_WR(j, mem0wr + j*0x100 + shadow);
	}

	for(j = 0xc; j < 0x20; j++) {
		SET_PAGE_INFO_WR(j, mem0wr + j*0x100);
	}

	shadow = 0;
	if(ramwrt) {
		if((g_c035_shadow_reg & 0x14) == 0 ||
					(g_c035_shadow_reg & 0x08) == 0) {
			shadow = BANK_SHADOW2;
		}
	} else if((g_c035_shadow_reg & 0x04) == 0) {
		shadow = BANK_SHADOW;
	}
	for(j = 0x40; j < 0x60; j++) {
		SET_PAGE_INFO_WR(j, mem0wr + j*0x100 + shadow);
	}

	shadow = 0;
	if(ramwrt && (g_c035_shadow_reg & 0x08) == 0) {
		/* shr shadowing */
		shadow = BANK_SHADOW2;
	}
	for(j = 0x60; j < 0xa0; j++) {
		SET_PAGE_INFO_WR(j, mem0wr + j*0x100 + shadow);
	}

	for(j = 0xa0; j < 0xc0; j++) {
		SET_PAGE_INFO_WR(j, mem0wr + j*0x100);
	}

	/* No need for fixup_brks() since only called from set_statereg() */
}

void
fixup_lcbank2()
{
	byte	*mem0rd, *mem0wr;
	int	lcbank2, c08x_wrdefram, rdrom;
	int	off;
	int	k;

	for(k = 0; k < 4; k++) {
		off = k;
		if(k >= 2) {
			off += (0xe0 - 2);
		}
		/* step off through 0x00, 0x01, 0xe0, 0xe1 */

		if(k < 2) {
			mem0rd = &(g_memory_ptr[k << 16]);
		} else {
			mem0rd = &(g_slow_memory_ptr[(k & 1) << 16]);
		}
		if((k == 0) && ALTZP) {
			mem0rd += 0x10000;
		}
		lcbank2 = LCBANK2;
		c08x_wrdefram = g_c08x_wrdefram;
		rdrom = RDROM;
		if((k < 2) && (g_c035_shadow_reg & 0x40)) {
			lcbank2 = 0;
			c08x_wrdefram = 1;
			rdrom = 0;
		}
		if(! lcbank2) {
			mem0rd -= 0x1000;	/* lcbank1, use 0xc000-cfff */
		}
		mem0wr = mem0rd;
		if((k < 2) && !c08x_wrdefram) {
			mem0wr += (BANK_IO_TMP | BANK_IO2_TMP);
		}
		if((k < 2) && rdrom) {
			mem0rd = &(g_rom_fc_ff_ptr[0x30000]);
		}
		fixup_any_bank_any_page(off*0x100 + 0xd0, 0x10,
					mem0rd + 0xd000, mem0wr + 0xd000);
	}

	/* No need for fixup_brks() since only called from set_statereg(), */
	/*  or from other routines which will handle it */
}

void
fixup_rdrom()
{
	byte	*mem0rd;
	int	j, k;

	/* fixup_lcbank2 handles 0xd000-dfff for rd & wr*/
	fixup_lcbank2();

	for(k = 0; k < 2; k++) {
		/* k is the bank */
		mem0rd = &(g_memory_ptr[k << 16]);
		if((k == 0) && ALTZP) {
			mem0rd += 0x10000;
		}
		if((g_c035_shadow_reg & 0x40) == 0) {
			if(RDROM) {
				mem0rd = &(g_rom_fc_ff_ptr[0x30000]);
			}
		}
		for(j = 0xe0; j < 0x100; j++) {
			SET_PAGE_INFO_RD(j + k*0x100, mem0rd + j*0x100);
		}
	}

	/* No need for fixup_brks() since only called from set_statereg() */
}

void
set_statereg(double dcycs, int val)
{
	int	xor;

	xor = val ^ g_c068_statereg;
	g_c068_statereg = val;
	if(xor == 0) {
		return;
	}

	if(xor & 0x80) {
		/* altzp */
		fixup_altzp();
	}
	if(xor & 0x40) {
		/* page2 */
		g_cur_a2_stat = (g_cur_a2_stat & ~ALL_STAT_PAGE2) |
						(val & ALL_STAT_PAGE2);
		fixup_page2(dcycs);
	}

	if(xor & 0x20) {
		/* RAMRD */
		fixup_ramrd();
	}

	if(xor & 0x10) {
		/* RAMWRT */
		fixup_ramwrt();
	}

	if(xor & 0x08) {
		/* RDROM */
		fixup_rdrom();
	}

	if(xor & 0x04) {
		/* LCBANK2 */
		fixup_lcbank2();
	}

	if(xor & 0x02) {
		/* ROMBANK */
		halt_printf("Just set rombank = %d\n", ROMB);
	}

	if(xor & 0x01) {
		fixup_intcx();
	}

	if(xor) {
		fixup_brks();
	}
}

void
fixup_shadow_txt1()
{
	byte	*mem0wr;
	int	j;

	fixup_bank0_0400_0800();

	mem0wr = &(g_memory_ptr[0x10000]);
	if((g_c035_shadow_reg & 0x01) == 0) {
		mem0wr += BANK_SHADOW2;
	}
	for(j = 4; j < 8; j++) {
		SET_PAGE_INFO_WR(0x100 + j, mem0wr + j*0x100);
	}
}

void
fixup_shadow_txt2()
{
	byte	*mem0wr;
	int	shadow;
	int	j;

	/* bank 0 */
	mem0wr = &(g_memory_ptr[0x00000]);
	shadow = BANK_SHADOW;
	if(RAMWRT) {
		mem0wr += 0x10000;
		shadow = BANK_SHADOW2;
	}
	if(((g_c035_shadow_reg & 0x20) == 0) &&
			((g_rom_version != 1) || g_user_page2_shadow)) {
		mem0wr += shadow;
	}
	for(j = 8; j < 0xc; j++) {
		SET_PAGE_INFO_WR(j, mem0wr + j*0x100);
	}

	/* and bank 1 */
	mem0wr = &(g_memory_ptr[0x10000]);
	if(((g_c035_shadow_reg & 0x20) == 0) &&
			((g_rom_version != 1) || g_user_page2_shadow)) {
		mem0wr += BANK_SHADOW2;
	}
	for(j = 8; j < 0xc; j++) {
		SET_PAGE_INFO_WR(0x100 + j, mem0wr + j*0x100);
	}
}

void
fixup_shadow_hires1()
{
	byte	*mem0wr;
	int	j;

	fixup_bank0_2000_4000();

	/* and bank 1 */
	mem0wr = &(g_memory_ptr[0x10000]);
	if((g_c035_shadow_reg & 0x12) == 0 || (g_c035_shadow_reg & 0x8) == 0) {
		mem0wr += BANK_SHADOW2;
	}
	for(j = 0x20; j < 0x40; j++) {
		SET_PAGE_INFO_WR(0x100 + j, mem0wr + j*0x100);
	}
}

void
fixup_shadow_hires2()
{
	byte	*mem0wr;
	int	j;

	/* bank 0 */
	mem0wr = &(g_memory_ptr[0x00000]);
	if(RAMWRT) {
		mem0wr += 0x10000;
		if((g_c035_shadow_reg & 0x14) == 0 ||
					(g_c035_shadow_reg & 0x8) == 0) {
			mem0wr += BANK_SHADOW2;
		}
	} else if((g_c035_shadow_reg & 0x04) == 0) {
		mem0wr += BANK_SHADOW;
	}
	for(j = 0x40; j < 0x60; j++) {
		SET_PAGE_INFO_WR(j, mem0wr + j*0x100);
	}

	/* and bank 1 */
	mem0wr = &(g_memory_ptr[0x10000]);
	if((g_c035_shadow_reg & 0x14) == 0 || (g_c035_shadow_reg & 0x8) == 0) {
		mem0wr += BANK_SHADOW2;
	}
	for(j = 0x40; j < 0x60; j++) {
		SET_PAGE_INFO_WR(0x100 + j, mem0wr + j*0x100);
	}
}

void
fixup_shadow_shr()
{
	byte	*mem0wr;
	int	j;

	/* bank 0, only pages 0x60 - 0xa0 */
	mem0wr = &(g_memory_ptr[0x00000]);
	if(RAMWRT) {
		mem0wr += 0x10000;
		if((g_c035_shadow_reg & 0x8) == 0) {
			mem0wr += BANK_SHADOW2;
		}
	}
	for(j = 0x60; j < 0xa0; j++) {
		SET_PAGE_INFO_WR(j, mem0wr + j*0x100);
	}

	/* and bank 1, only pages 0x60 - 0xa0 */
	mem0wr = &(g_memory_ptr[0x10000]);
	if((g_c035_shadow_reg & 0x8) == 0) {
		mem0wr += BANK_SHADOW2;
	}
	for(j = 0x60; j < 0xa0; j++) {
		SET_PAGE_INFO_WR(0x100 + j, mem0wr + j*0x100);
	}
}

void
fixup_shadow_iolc()
{
	byte	*mem0rd;
	int	k;

	if(g_c035_shadow_reg & 0x40) {
		/* Disable language card area */
		for(k = 0; k < 2; k++) {
			mem0rd = &(g_memory_ptr[k << 16]);
			fixup_any_bank_any_page((k << 8) + 0xc0, 0x10,
				mem0rd + 0xd000, mem0rd + 0xd000);
			if(k == 0 && ALTZP) {
				mem0rd += 0x10000;
			}
			fixup_any_bank_any_page((k << 8) + 0xd0, 0x10,
				mem0rd + 0xc000, mem0rd + 0xc000);
			fixup_any_bank_any_page((k << 8) + 0xe0, 0x20,
				mem0rd + 0xe000, mem0rd + 0xe000);
		}
	} else {
		/* 0xc000 area */
		fixup_intcx();

		/* 0xd000 area */
		/* fixup_lcbank2(); -- not needed since fixup_rdrom does it */

		/* Fix 0xd000-0xffff for reads, banks 0 and 1 */
		fixup_rdrom();		/* which calls fixup_lcbank2 */

		/* Fix 0xd000-0xffff for writes, banks 0 and 1 */
		fixup_wrdefram(g_c08x_wrdefram);
	}
}

void
update_shadow_reg(int val)
{
	int	xor;

	if(g_c035_shadow_reg == val) {
		return;
	}

	xor = g_c035_shadow_reg ^ val;
	g_c035_shadow_reg = val;

	if(xor & 8) {
		fixup_shadow_hires1();
		fixup_shadow_hires2();
		fixup_shadow_shr();
		xor = xor & (~0x16);
	}
	if(xor & 0x10) {
		fixup_shadow_hires1();
		fixup_shadow_hires2();
		xor = xor & (~0x6);
	}
	if(xor & 2) {
		fixup_shadow_hires1();
	}
	if(xor & 4) {
		fixup_shadow_hires2();
	}
	if(xor & 1) {
		fixup_shadow_txt1();
	}
	if((xor & 0x20) && ((g_rom_version != 1) || g_user_page2_shadow)) {
		fixup_shadow_txt2();
	}
	if(xor & 0x40) {
		fixup_shadow_iolc();
	}
	if(xor) {
		fixup_brks();
	}
}

void
fixup_shadow_all_banks()
{
	byte	*mem0rd;
	int	shadow;
	int	num_banks;
	int	j, k;

	/* Assume Ninja Force Megademo */
	/* only do banks 3 - num_banks by 2, shadowing into e1 */

	shadow = 0;
	if((g_c036_val_speed & 0x10) && ((g_c035_shadow_reg & 0x08) == 0)) {
		shadow = BANK_SHADOW2;
	}
	num_banks = g_mem_size_total >> 16;
	for(k = 3; k < num_banks; k += 2) {
		mem0rd = &(g_memory_ptr[k*0x10000 + 0x2000]) + shadow;
		for(j = 0x20; j < 0xa0; j++) {
			SET_PAGE_INFO_WR(k*0x100 + j, mem0rd);
			mem0rd += 0x100;
		}
	}

	fixup_brks();
}

void
setup_pageinfo()
{
	byte	*mem0rd;
	word32	mem_size_pages;

	/* first, set all of memory to point to itself */
	mem_size_pages = g_mem_size_total >> 8;
	mem0rd = &(g_memory_ptr[0]);
	fixup_any_bank_any_page(0, mem_size_pages, mem0rd, mem0rd);

	/* mark unused memory as BAD_MEM */
	fixup_any_bank_any_page(mem_size_pages, 0xfc00-mem_size_pages,
			BANK_BAD_MEM, BANK_BAD_MEM);

	fixup_shadow_all_banks();

	/* ROM */
	mem0rd = &(g_rom_fc_ff_ptr[0]);
	fixup_any_bank_any_page(0xfc00, 0x400, mem0rd,
				mem0rd + (BANK_IO_TMP | BANK_IO2_TMP));

	/* banks e0, e1 */
	mem0rd = &(g_slow_memory_ptr[0]);
	fixup_any_bank_any_page(0xe000, 0x04, mem0rd + 0x0000, mem0rd + 0x0000);
	fixup_any_bank_any_page(0xe004, 0x08, mem0rd + 0x0400,
						mem0rd + 0x0400 + BANK_SHADOW);
	fixup_any_bank_any_page(0xe00c, 0x14, mem0rd + 0x0c00, mem0rd + 0x0c00);
	fixup_any_bank_any_page(0xe020, 0x40, mem0rd + 0x2000,
						mem0rd + 0x2000 + BANK_SHADOW);
	fixup_any_bank_any_page(0xe060, 0xa0, mem0rd + 0x6000, mem0rd + 0x6000);

	mem0rd = &(g_slow_memory_ptr[0x10000]);
	fixup_any_bank_any_page(0xe100, 0x04, mem0rd + 0x0000, mem0rd + 0x0000);
	fixup_any_bank_any_page(0xe104, 0x08, mem0rd + 0x0400,
						mem0rd + 0x0400 + BANK_SHADOW2);
	fixup_any_bank_any_page(0xe10c, 0x14, mem0rd + 0x0c00, mem0rd + 0x0c00);
	fixup_any_bank_any_page(0xe120, 0x80, mem0rd + 0x2000,
						mem0rd + 0x2000 + BANK_SHADOW2);
	fixup_any_bank_any_page(0xe1a0, 0x60, mem0rd + 0xa000, mem0rd + 0xa000);

	fixup_intcx();	/* correct banks 0xe0,0xe1, 0xc000-0xcfff area */
	fixup_lcbank2(); /* correct 0xd000-0xdfff area */

	fixup_bank0_2000_4000();
	fixup_bank0_0400_0800();
	fixup_wrdefram(g_c08x_wrdefram);
	fixup_altzp();
	fixup_ramrd();
	fixup_ramwrt();
	fixup_rdrom();
	fixup_shadow_txt1();
	fixup_shadow_txt2();
	fixup_shadow_hires1();
	fixup_shadow_hires2();
	fixup_shadow_shr();
	fixup_shadow_iolc();
	fixup_brks();
}

void
show_bankptrs_bank0rdwr()
{
	show_bankptrs(0);
	show_bankptrs(1);
	show_bankptrs(0xe0);
	show_bankptrs(0xe1);
	printf("statereg: %02x\n", g_c068_statereg);
}

void
show_bankptrs(int bnk)
{
	int i;
	Pg_info rd, wr;
	byte *ptr_rd, *ptr_wr;

	printf("g_memory_ptr: %p, dummy_mem: %p, slow_mem_ptr: %p\n",
		g_memory_ptr, g_dummy_memory1_ptr, g_slow_memory_ptr);
	printf("g_rom_fc_ff_ptr: %p\n", g_rom_fc_ff_ptr);

	printf("Showing bank_info array for %02x\n", bnk);
	for(i = 0; i < 256; i++) {
		rd = GET_PAGE_INFO_RD(bnk*0x100 + i);
		wr = GET_PAGE_INFO_WR(bnk*0x100 + i);
		ptr_rd = (byte *)rd;
		ptr_wr = (byte *)wr;
		printf("%04x rd: ", bnk*256 + i);
		show_addr(ptr_rd);
		printf(" wr: ");
		show_addr(ptr_wr);
		printf("\n");
	}
}

void
show_addr(byte *ptr)
{
	word32	mem_size;

	mem_size = g_mem_size_total;
	if(ptr >= g_memory_ptr && ptr < &g_memory_ptr[mem_size]) {
		printf("%p--memory[%06x]", ptr,
					(word32)(ptr - g_memory_ptr));
	} else if(ptr >= g_rom_fc_ff_ptr && ptr < &g_rom_fc_ff_ptr[256*1024]) {
		printf("%p--rom_fc_ff[%06x]", ptr,
					(word32)(ptr - g_rom_fc_ff_ptr));
	} else if(ptr >= g_slow_memory_ptr && ptr<&g_slow_memory_ptr[128*1024]){
		printf("%p--slow_memory[%06x]", ptr,
					(word32)(ptr - g_slow_memory_ptr));
	} else if(ptr >=g_dummy_memory1_ptr && ptr < &g_dummy_memory1_ptr[256]){
		printf("%p--dummy_memory[%06x]", ptr,
					(word32)(ptr - g_dummy_memory1_ptr));
	} else {
		printf("%p--unknown", ptr);
	}
}


#define CALC_DCYCS_FROM_CYC_PTR(dcycs, cyc_ptr)			\
	dcycs = g_last_vbl_dcycs + *cyc_ptr;


int
io_read(word32 loc, double *cyc_ptr)
{
	double	dcycs;
	dword64	word64_tmp;
	int	new_lcbank2, new_wrdefram, tmp;
	int	i;

	CALC_DCYCS_FROM_CYC_PTR(dcycs, cyc_ptr);

/* IO space */
	switch((loc >> 8) & 0xf) {
	case 0: /* 0xc000 - 0xc0ff */
		switch(loc & 0xff) {
		/* 0xc000 - 0xc00f */
		case 0x00: case 0x01: case 0x02: case 0x03:
		case 0x04: case 0x05: case 0x06: case 0x07:
		case 0x08: case 0x09: case 0x0a: case 0x0b:
		case 0x0c: case 0x0d: case 0x0e: case 0x0f:
			return(adb_read_c000());

		/* 0xc010 - 0xc01f */
		case 0x10: /* c010 */
			return(adb_access_c010());
		case 0x11: /* c011 = RDLCBANK2 */
			return IOR(LCBANK2);
		case 0x12: /* c012= RDLCRAM */
			return IOR(!RDROM);
		case 0x13: /* c013=rdramd */
			return IOR(RAMRD);
		case 0x14: /* c014=rdramwrt */
			return IOR(RAMWRT);
		case 0x15: /* c015 = RDCXROM = INTCX */
			return IOR(g_c068_statereg & 1);
		case 0x16: /* c016: ALTZP */
			return IOR(ALTZP);
		case 0x17: /* c017: rdc3rom */
			return IOR(g_c02d_int_crom & (1 << 3));
		case 0x18: /* c018: rd80c0l */
			return IOR((g_cur_a2_stat & ALL_STAT_ST80));
		case 0x19: /* c019: rdvblbar */
			tmp = in_vblank(dcycs);
			return IOR(tmp);
		case 0x1a: /* c01a: rdtext */
			return IOR(g_cur_a2_stat & ALL_STAT_TEXT);
		case 0x1b: /* c01b: rdmix */
			return IOR(g_cur_a2_stat & ALL_STAT_MIX_T_GR);
		case 0x1c: /* c01c: rdpage2 */
			return IOR(g_cur_a2_stat & ALL_STAT_PAGE2);
		case 0x1d: /* c01d: rdhires */
			return IOR(g_cur_a2_stat & ALL_STAT_HIRES);
		case 0x1e: /* c01e: altcharset on? */
			return IOR(g_cur_a2_stat & ALL_STAT_ALTCHARSET);
		case 0x1f: /* c01f: rd80vid */
			return IOR(g_cur_a2_stat & ALL_STAT_VID80);

		/* 0xc020 - 0xc02f */
		case 0x20: /* 0xc020 */
			/* Click cassette port */
			return float_bus(dcycs);
		case 0x21: /* 0xc021 */
			/* Not documented, but let's return COLOR_C021 */
			return IOR(g_cur_a2_stat & ALL_STAT_COLOR_C021);
		case 0x22: /* 0xc022 */
			return (g_cur_a2_stat >> BIT_ALL_STAT_BG_COLOR) & 0xff;
		case 0x23: /* 0xc023 */
			return g_c023_val;
		case 0x24: /* 0xc024 */
			return mouse_read_c024(dcycs);
		case 0x25: /* 0xc025 */
			return adb_read_c025();
		case 0x26: /* 0xc026 */
			return adb_read_c026();
		case 0x27: /* 0xc027 */
			return adb_read_c027();
		case 0x28: /* 0xc028 */
			return 0;
		case 0x29: /* 0xc029 */
			return((g_cur_a2_stat & 0xa0) | g_c029_val_some);
		case 0x2a: /* 0xc02a */
#if 0
			printf("Reading c02a...returning 0\n");
#endif
			return 0;
		case 0x2b: /* 0xc02b */
			return g_c02b_val;
		case 0x2c: /* 0xc02c */
			/* printf("reading c02c, returning 0\n"); */
			return 0;
		case 0x2d: /* 0xc02d */
			tmp = g_c02d_int_crom;
			return tmp;
		case 0x2e: /* 0xc02e */
		case 0x2f: /* 0xc02f */
			return read_vid_counters(loc, dcycs);

		/* 0xc030 - 0xc03f */
		case 0x30: /* 0xc030 */
			/* click speaker */
			return doc_read_c030(dcycs);
		case 0x31: /* 0xc031 */
			/* 3.5" control */
			return g_c031_disk35;
		case 0x32: /* 0xc032 */
			/* scan int */
			return 0;
		case 0x33: /* 0xc033 = CLOCKDATA*/
			return g_c033_data;
		case 0x34: /* 0xc034 = CLOCKCTL */
			return g_c034_val;
		case 0x35: /* 0xc035 */
			return g_c035_shadow_reg;
		case 0x36: /* 0xc036 = CYAREG */
			return g_c036_val_speed;
		case 0x37: /* 0xc037 */
			return 0;
		case 0x38: /* 0xc038 */
			return scc_read_reg(1, dcycs);
		case 0x39: /* 0xc039 */
			return scc_read_reg(0, dcycs);
		case 0x3a: /* 0xc03a */
			return scc_read_data(1, dcycs);
		case 0x3b: /* 0xc03b */
			return scc_read_data(0, dcycs);
		case 0x3c: /* 0xc03c */
			/* doc control */
			return doc_read_c03c(dcycs);
		case 0x3d: /* 0xc03d */
			return doc_read_c03d(dcycs);
		case 0x3e: /* 0xc03e */
			return (g_c03ef_doc_ptr & 0xff);
		case 0x3f: /* 0xc03f */
			return (g_c03ef_doc_ptr >> 8);

		/* 0xc040 - 0xc04f */
		case 0x40: /* 0xc040 */
			/* cassette */
			return 0;
		case 0x41: /* 0xc041 */
			return g_c041_val;
		case 0x45: /* 0xc045 */
			halt_printf("Mega II mouse read: c045\n");
			return 0;
		case 0x46: /* 0xc046 */
			tmp = g_c046_val;
			g_c046_val = (tmp & 0xbf) + ((tmp & 0x80) >> 1);
			return tmp;
		case 0x47: /* 0xc047 */
			remove_irq(IRQ_PENDING_C046_25SEC |
							IRQ_PENDING_C046_VBL);
			g_c046_val &= 0xe7;	/* clear vbl_int, 1/4sec int*/
			return 0;
		case 0x42: /* 0xc042 */
		case 0x43: /* 0xc043 */
			return 0;
		case 0x4f: /* 0xc04f */
			/* for information on c04f, see: */
			/* www.sheppyware.net/tech/hardware/softswitches.html */
			/* write to $c04f to start.  Then read $c04f to get */
			/* emulator ($16=sweet16, $fe=bernie II). */
			/* Then read again to get version: $21 == 2.1 */
			switch(g_em_emubyte_cnt) {
			case 1:
				g_em_emubyte_cnt = 2;
				return 'K';
			case 2:
				g_em_emubyte_cnt = 0;
				tmp = g_kegs_version_str[0] - '0';
				i = g_kegs_version_str[2] - '0';
				return ((tmp & 0xf) << 4) + (i & 0xf);
			default:
				g_em_emubyte_cnt = 0;
				return 0;
			}
		case 0x44: /* 0xc044 */
		case 0x48: /* 0xc048 */
		case 0x49: /* 0xc049 */
		case 0x4a: /* 0xc04a */
		case 0x4b: /* 0xc04b */
		case 0x4c: /* 0xc04c */
		case 0x4d: /* 0xc04d */
		case 0x4e: /* 0xc04e */
			UNIMPL_READ;

		/* 0xc050 - 0xc05f */
		case 0x50: /* 0xc050 */
			if(g_cur_a2_stat & ALL_STAT_TEXT) {
				g_cur_a2_stat &= (~ALL_STAT_TEXT);
				change_display_mode(dcycs);
			}
			return float_bus(dcycs);
		case 0x51: /* 0xc051 */
			if((g_cur_a2_stat & ALL_STAT_TEXT) == 0) {
				g_cur_a2_stat |= (ALL_STAT_TEXT);
				change_display_mode(dcycs);
			}
			return float_bus(dcycs);
		case 0x52: /* 0xc052 */
			if(g_cur_a2_stat & ALL_STAT_MIX_T_GR) {
				g_cur_a2_stat &= (~ALL_STAT_MIX_T_GR);
				change_display_mode(dcycs);
			}
			return float_bus(dcycs);
		case 0x53: /* 0xc053 */
			if((g_cur_a2_stat & ALL_STAT_MIX_T_GR) == 0) {
				g_cur_a2_stat |= (ALL_STAT_MIX_T_GR);
				change_display_mode(dcycs);
			}
			return float_bus(dcycs);
		case 0x54: /* 0xc054 */
			set_statereg(dcycs, g_c068_statereg & (~0x40));
			return float_bus(dcycs);
		case 0x55: /* 0xc055 */
			set_statereg(dcycs, g_c068_statereg | 0x40);
			return float_bus(dcycs);
		case 0x56: /* 0xc056 */
			if(g_cur_a2_stat & ALL_STAT_HIRES) {
				g_cur_a2_stat &= (~ALL_STAT_HIRES);
				fixup_hires_on();
				change_display_mode(dcycs);
			}
			return float_bus(dcycs);
		case 0x57: /* 0xc057 */
			if((g_cur_a2_stat & ALL_STAT_HIRES) == 0) {
				g_cur_a2_stat |= (ALL_STAT_HIRES);
				fixup_hires_on();
				change_display_mode(dcycs);
			}
			return float_bus(dcycs);
		case 0x58: /* 0xc058 */
			if(g_zipgs_unlock < 4) {
				g_c05x_annuncs &= (~1);
			}
			return 0;
		case 0x59: /* 0xc059 */
			if(g_zipgs_unlock >= 4) {
				return g_zipgs_reg_c059;
			} else {
				g_c05x_annuncs |= 1;
			}
			return 0;
		case 0x5a: /* 0xc05a */
			if(g_zipgs_unlock >= 4) {
				return g_zipgs_reg_c05a;
			} else {
				g_c05x_annuncs &= (~2);
			}
			return 0;
		case 0x5b: /* 0xc05b */
			if(g_zipgs_unlock >= 4) {
				word64_tmp = (dword64)dcycs;
				tmp = (word64_tmp >> 9) & 1;
				return (tmp << 7) + (g_zipgs_reg_c05b & 0x7f);
			} else {
				g_c05x_annuncs |= 2;
			}
			return 0;
		case 0x5c: /* 0xc05c */
			if(g_zipgs_unlock >= 4) {
				return g_zipgs_reg_c05c;
			} else {
				g_c05x_annuncs &= (~4);
			}
			return 0;
		case 0x5d: /* 0xc05d */
			if(g_zipgs_unlock >= 4) {
				halt_printf("Reading ZipGS $c05d!\n");
			} else {
				g_c05x_annuncs |= 4;
			}
			return 0;
		case 0x5e: /* 0xc05e */
			if(g_zipgs_unlock >= 4) {
				halt_printf("Reading ZipGS $c05e!\n");
			} else if(g_cur_a2_stat & ALL_STAT_ANNUNC3) {
				g_cur_a2_stat &= (~ALL_STAT_ANNUNC3);
				change_display_mode(dcycs);
			}
			return 0;
		case 0x5f: /* 0xc05f */
			if(g_zipgs_unlock >= 4) {
				halt_printf("Reading ZipGS $c05f!\n");
			} else if((g_cur_a2_stat & ALL_STAT_ANNUNC3) == 0) {
				g_cur_a2_stat |= (ALL_STAT_ANNUNC3);
				change_display_mode(dcycs);
			}
			return 0;


		/* 0xc060 - 0xc06f */
		case 0x60: /* 0xc060 */
			return IOR(g_paddle_buttons & 8);
		case 0x61: /* 0xc061 */
			return IOR(adb_is_cmd_key_down() ||
							g_paddle_buttons & 1);
		case 0x62: /* 0xc062 */
			return IOR(adb_is_option_key_down() ||
							g_paddle_buttons & 2);
		case 0x63: /* 0xc063 */
			return IOR(g_paddle_buttons & 4);
		case 0x64: /* 0xc064 */
			return read_paddles(dcycs, 0);
		case 0x65: /* 0xc065 */
			return read_paddles(dcycs, 1);
		case 0x66: /* 0xc066 */
			return read_paddles(dcycs, 2);
		case 0x67: /* 0xc067 */
			return read_paddles(dcycs, 3);
		case 0x68: /* 0xc068 = STATEREG */
			return g_c068_statereg;
		case 0x69: /* 0xc069 */
			/* Reserved reg, return 0 */
			return 0;
		case 0x6a: /* 0xc06a */
		case 0x6b: /* 0xc06b */
			UNIMPL_READ;
		case 0x6c: /* 0xc06c */
		case 0x6d: /* 0xc06d */
		case 0x6e: /* 0xc06e */
		case 0x6f: /* 0xc06f */
			return g_c06c_latched_cyc >> (8 * (loc & 3)) & 0xff;

		/* 0xc070 - 0xc07f */
		case 0x70: /* c070 */
			paddle_trigger(dcycs);
			return 0;
		case 0x71:	/* 0xc071 */
		case 0x72: case 0x73:
		case 0x74: case 0x75: case 0x76: case 0x77:
		case 0x78: case 0x79: case 0x7a: case 0x7b:
		case 0x7c: case 0x7d: case 0x7e: case 0x7f:
			return g_rom_fc_ff_ptr[3*65536 + 0xc000 + (loc & 0xff)];

		/* 0xc080 - 0xc08f */
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b:
		case 0x8c: case 0x8d: case 0x8e: case 0x8f:
			new_lcbank2 = ((loc & 0x8) >> 1) ^ 0x4;
			new_wrdefram = (loc & 1);
			if(new_wrdefram != g_c08x_wrdefram) {
				fixup_wrdefram(new_wrdefram);
			}
			switch(loc & 0x3) {
			case 0x1: /* 0xc081 */
			case 0x2: /* 0xc082 */
				/* Read rom, set lcbank2 */
				set_statereg(dcycs, (g_c068_statereg & ~(0x04))|
					(new_lcbank2 | 0x08));
				break;
			case 0x0: /* 0xc080 */
			case 0x3: /* 0xc083 */
				/* Read ram (clear RDROM), set lcbank2 */
				set_statereg(dcycs, (g_c068_statereg & ~(0x0c))|
					(new_lcbank2));
				break;
			}
			return float_bus(dcycs);
		/* 0xc090 - 0xc09f */
		case 0x90: case 0x91: case 0x92: case 0x93:
		case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b:
		case 0x9c: case 0x9d: case 0x9e: case 0x9f:
			/* UNIMPL_READ; */
			return 0;
		/* 0xc0a0 - 0xc0af */
		case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf:
			return 0;
			/* UNIMPL_READ; */

		/* 0xc0b0 - 0xc0bf */
		case 0xb0:
			/* c0b0: female voice tool033 look at this */
			return 0;
		case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			/* UNIMPL_READ; */
			return 0;
		/* c0b8: Second Sight card stuff: return 0 */
		case 0xb8:
			return 0;
			break;

		/* 0xc0c0 - 0xc0cf */
		case 0xc0: case 0xc1: case 0xc2: case 0xc3:
		case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb:
		case 0xcc: case 0xcd: case 0xce: case 0xcf:
			return 0;
		/* 0xc0d0 - 0xc0df */
		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
		case 0xd4: case 0xd5: case 0xd6: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb:
		case 0xdc: case 0xdd: case 0xde: case 0xdf:
			return 0;
		/* 0xc0e0 - 0xc0ef */
		case 0xe0: case 0xe1: case 0xe2: case 0xe3:
		case 0xe4: case 0xe5: case 0xe6: case 0xe7:
		case 0xe8: case 0xe9: case 0xea: case 0xeb:
		case 0xec: case 0xed: case 0xee: case 0xef:
			return read_iwm(loc, dcycs);
		/* 0xc0f0 - 0xc0ff */
		case 0xf0: case 0xf1: case 0xf2: case 0xf3:
		case 0xf4: case 0xf5: case 0xf6: case 0xf7:
		case 0xf8: case 0xf9: case 0xfa: case 0xfb:
		case 0xfc: case 0xfd: case 0xfe: case 0xff:
			return 0;

		default:
			printf("loc: %04x bad\n", loc);
			UNIMPL_READ;
		}
	case 1: case 2: case 3: case 5: case 6:
		/* c100 - c6ff, (except c4xx) */
		return float_bus(dcycs);
	case 4:
		return mockingboard_read(loc, dcycs);
	case 7:
		/* c700 */
		tmp = g_rom_fc_ff_ptr[0x3c500 + (loc & 0xff)];
		if((loc & 0xff) == 0xfb) {
			tmp = tmp & 0xbf;	/* clear bit 6 for ROM 03 */
		}
		return tmp;
	case 8: case 9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe:
		return float_bus(dcycs);
		// UNIMPL_READ;
	case 0xf:
		if((loc & 0xfff) == 0xfff) {
			return g_rom_fc_ff_ptr[0x3cfff];
		}
		return float_bus(dcycs);
		// UNIMPL_READ;
	}

	halt_printf("io_read: hit end, loc: %06x\n", loc);

	return 0xff;
}

void
io_write(word32 loc, int val, double *cyc_ptr)
{
	double	dcycs;
#if 0
	double	fcyc, new_fcyc;
#endif
	int	new_tmp;
	int	new_lcbank2;
	int	new_wrdefram;
	int	tmp;
	int	fixup;

	CALC_DCYCS_FROM_CYC_PTR(dcycs, cyc_ptr);

	val = val & 0xff;
	switch((loc >> 8) & 0xf) {
	case 0: /* 0xc000 - 0xc0ff */
		switch(loc & 0xff) {
		/* 0xc000 - 0xc00f */
		case 0x00: /* 0xc000 */
			if(g_cur_a2_stat & ALL_STAT_ST80) {
				g_cur_a2_stat &= (~ALL_STAT_ST80);
				fixup_st80col(dcycs);
			}
			return;
		case 0x01: /* 0xc001 */
			if((g_cur_a2_stat & ALL_STAT_ST80) == 0) {
				g_cur_a2_stat |= (ALL_STAT_ST80);
				fixup_st80col(dcycs);
			}
			return;
		case 0x02: /* 0xc002 */
			set_statereg(dcycs, g_c068_statereg & ~0x20);
			return;
		case 0x03: /* 0xc003 */
			set_statereg(dcycs, g_c068_statereg | 0x20);
			return;
		case 0x04: /* 0xc004 */
			set_statereg(dcycs, g_c068_statereg & ~0x10);
			return;
		case 0x05: /* 0xc005 */
			set_statereg(dcycs, g_c068_statereg | 0x10);
			return;
		case 0x06: /* 0xc006 = SETSLOTCXROM, use slot rom c800 */
			set_statereg(dcycs, g_c068_statereg & ~0x01);
			return;
		case 0x07: /* 0xc007 = SETINTXROM, use int rom c800 */
			set_statereg(dcycs, g_c068_statereg | 0x01);
			return;
		case 0x08: /* 0xc008 */
			set_statereg(dcycs, g_c068_statereg & ~0x80);
			return;
		case 0x09: /* 0xc009 */
			set_statereg(dcycs, g_c068_statereg | 0x80);
			return;
		case 0x0a: /* 0xc00a = SETINTC3ROM, use internal ROM slot 3*/
			tmp = 1 << 3;
			if((g_c02d_int_crom & tmp) != 0) {
				g_c02d_int_crom &= ~tmp;
				fixup_intcx();
			}
			return;
		case 0x0b: /* 0xc00b = SETSLOTC3ROM, use slot rom slot 3 */
			tmp = 1 << 3;
			if((g_c02d_int_crom & tmp) == 0) {
				g_c02d_int_crom |= tmp;
				fixup_intcx();
			}
			return;
		case 0x0c: /* 0xc00c = SETINTC3ROM, use internal rom slot 3*/
			if(g_cur_a2_stat & ALL_STAT_VID80) {
				g_cur_a2_stat &= (~ALL_STAT_VID80);
				change_display_mode(dcycs);
			}
			return;
		case 0x0d: /* 0xc00d */
			if((g_cur_a2_stat & ALL_STAT_VID80) == 0) {
				g_cur_a2_stat |= (ALL_STAT_VID80);
				change_display_mode(dcycs);
			}
			return;
		case 0x0e: /* 0xc00e */
			if(g_cur_a2_stat & ALL_STAT_ALTCHARSET) {
				g_cur_a2_stat &= (~ALL_STAT_ALTCHARSET);
				change_display_mode(dcycs);
			}
			return;
		case 0x0f: /* 0xc00f */
			if((g_cur_a2_stat & ALL_STAT_ALTCHARSET) == 0) {
				g_cur_a2_stat |= (ALL_STAT_ALTCHARSET);
				change_display_mode(dcycs);
			}
			return;
		/* 0xc010 - 0xc01f */
		case 0x10: case 0x11: case 0x12: case 0x13:
		case 0x14: case 0x15: case 0x16: case 0x17:
		case 0x18: case 0x19: case 0x1a: case 0x1b:
		case 0x1c: case 0x1d: case 0x1e: case 0x1f:
			adb_access_c010();
			return;
		/* 0xc020 - 0xc02f */
		case 0x20: /* 0xc020 */
			/* WRITE CASSETTE?? */
			return;
		case 0x21: /* 0xc021 */
			new_tmp = ((val >> 7) & 1) <<
						(31 - BIT_ALL_STAT_COLOR_C021);
			if((g_cur_a2_stat & ALL_STAT_COLOR_C021) != new_tmp) {
				g_cur_a2_stat ^= new_tmp;
				change_display_mode(dcycs);
			}
			return;
		case 0x22: /* 0xc022 */
			/* change text color */
			tmp = (g_cur_a2_stat >> BIT_ALL_STAT_BG_COLOR) & 0xff;
			if(val != tmp) {
				/* change text/bg color! */
				g_cur_a2_stat &= ~(ALL_STAT_TEXT_COLOR |
							ALL_STAT_BG_COLOR);
				g_cur_a2_stat += (val << BIT_ALL_STAT_BG_COLOR);
				change_display_mode(dcycs);
			}
			return;
		case 0x23: /* 0xc023 */
			if((val & 0x19) != 0) {
				halt_printf("c023 write of %02x!!!\n", val);
			}
			tmp = (g_c023_val & 0x70) | (val & 0x0f);
			if((tmp & 0x22) == 0x22) {
				add_irq(IRQ_PENDING_C023_SCAN);
			}
			if(!(tmp & 2)) {
				remove_irq(IRQ_PENDING_C023_SCAN);
			}
			if((tmp & 0x44) == 0x44) {
				add_irq(IRQ_PENDING_C023_1SEC);
			}
			if(!(tmp & 0x4)) {
				remove_irq(IRQ_PENDING_C023_1SEC);
			}

			if(g_irq_pending & (IRQ_PENDING_C023_SCAN |
						IRQ_PENDING_C023_1SEC)) {
				tmp |= 0x80;
			}
			g_c023_val = tmp;
			return;
		case 0x24: /* 0xc024 */
			/* Write to mouse reg: Throw it away */
			return;
		case 0x26: /* 0xc026 */
			adb_write_c026(val);
			return;
		case 0x27: /* 0xc027 */
			adb_write_c027(val);
			return;
		case 0x29: /* 0xc029 */
			g_c029_val_some = val & 0x41;
			if((val & 1) == 0) {
				halt_printf("c029: %02x\n", val);
			}
			new_tmp = val & 0xa0;
			if(new_tmp != (g_cur_a2_stat & 0xa0)) {
				g_cur_a2_stat = (g_cur_a2_stat & (~0xa0)) +
					new_tmp;
				change_display_mode(dcycs);
			}
			return;
		case 0x2a: /* 0xc02a */
#if 0
			printf("Writing c02a with %02x\n", val);
#endif
			return;
		case 0x2b: /* 0xc02b */
			g_c02b_val = val;
			if(val != 0x08 && val != 0x00) {
				printf("Writing c02b with %02x\n", val);
			}
			return;
		case 0x2d: /* 0xc02d = Slot Reg. Bit set means use slot rom */
			if((val & 0x9) != 0) {
				halt_printf("Illegal c02d write: %02x!\n", val);
			}
			fixup = (val != g_c02d_int_crom);
			g_c02d_int_crom = val;
			if(fixup) {
				vid_printf("Write c02d of %02x\n", val);
				fixup_intcx();
			}
			return;
		case 0x28: /* 0xc028 */
		case 0x2c: /* 0xc02c */
			UNIMPL_WRITE;
		case 0x25: /* 0xc025 */
			/* Space Shark writes to c025--ignore */
		case 0x2e: /* 0xc02e */
		case 0x2f: /* 0xc02f */
			/* Modulae writes to this--just ignore them */
			return;
			break;

		/* 0xc030 - 0xc03f */
		case 0x30: /* 0xc030 */
#if 0
			printf("Write speaker?\n");
#endif
			(void)doc_read_c030(dcycs);
			return;
		case 0x31: /* 0xc031 */
			tmp = val ^ g_c031_disk35;
			if(tmp & 0x40) {
				/* apple35_sel changed, maybe speed change */
				engine_recalc_events();
			}
			g_c031_disk35 = val & 0xc0;
			return;
		case 0x32: /* 0xc032 */
			tmp = g_c023_val & 0x7f;
			if(((val & 0x40) == 0) && (tmp & 0x40)) {
				/* clear 1 sec int */
				remove_irq(IRQ_PENDING_C023_1SEC);
				tmp &= 0xbf;
				g_c023_val = tmp;
			}
			if(((val & 0x20) == 0) && (tmp & 0x20)) {
				/* clear scan line int */
				remove_irq(IRQ_PENDING_C023_SCAN);
				g_c023_val = tmp & 0xdf;
				check_for_new_scan_int(dcycs);
			}
			if(g_irq_pending & (IRQ_PENDING_C023_1SEC |
						IRQ_PENDING_C023_SCAN)) {
				g_c023_val |= 0x80;
			}
			if((val & 0x9f) != 0x9f) {
				irq_printf("c032: wrote %02x!\n", val);
			}
			return;
		case 0x33: /* 0xc033 = CLOCKDATA*/
			g_c033_data = val;
			return;
		case 0x34: /* 0xc034 = CLOCKCTL */
			tmp = val ^ g_c034_val;
			clock_write_c034(val);
			if(tmp & 0xf) {
				change_border_color(dcycs, val & 0xf);
			}
			return;
		case 0x35: /* 0xc035 */
			update_shadow_reg(val);
			return;
		case 0x36: /* 0xc036 = CYAREG */
			tmp = val ^ g_c036_val_speed;
			g_c036_val_speed = (val & ~0x20);	/* clr bit 5 */
			if(tmp & 0x80) {
				/* to recalculate times since speed changing */
				engine_recalc_events();
			}
			if(tmp & 0xf) {
				/* slot_motor_detect changed */
				engine_recalc_events();
			}

			if((val & 0x60) != 0) {
				/* for ROM 03, 0x40 is the power-on status */
				/*  and can be read/write */
				if(((val & 0x60) != 0x40) ||
							(g_rom_version < 3)) {
					g_c036_val_speed &= (~0x60);
					halt_printf("c036: %2x\n", val);
				}
			}
			if(tmp & 0x10) {	/* shadow in all banks! */
				if(g_num_shadow_all_banks++ == 0) {
					printf("Shadowing all banks...This "
						"must be the NFC Megademo\n");
				}
				fixup_shadow_all_banks();
			}
			return;
		case 0x37: /* 0xc037 */
			/* just ignore, probably someone writing c036 m=0 */
			return;
		case 0x38: /* 0xc038 */
			scc_write_reg(1, val, dcycs);
			return;
		case 0x39: /* 0xc039 */
			scc_write_reg(0, val, dcycs);
			return;
		case 0x3a: /* 0xc03a */
			scc_write_data(1, val, dcycs);
			return;
		case 0x3b: /* 0xc03b */
			scc_write_data(0, val, dcycs);
			return;
		case 0x3c: /* 0xc03c */
			/* doc ctl */
			doc_write_c03c(val, dcycs);
			return;
		case 0x3d: /* 0xc03d */
			/* doc data reg */
			doc_write_c03d(val, dcycs);
			return;
		case 0x3e: /* 0xc03e */
			g_c03ef_doc_ptr = (g_c03ef_doc_ptr & 0xff00) + val;
			return;
		case 0x3f: /* 0xc03f */
			g_c03ef_doc_ptr = (g_c03ef_doc_ptr & 0xff) + (val << 8);
			return;

		/* 0xc040 - 0xc04f */
		case 0x41: /* c041 */
			g_c041_val = val & 0x1f;
			if((val & 0xe7) != 0) {
				halt_printf("write c041: %02x\n", val);
			}

			if(!(val & C041_EN_VBL_INTS)) {
				/* no more vbl interrupt */
				remove_irq(IRQ_PENDING_C046_VBL);
			}
			if(!(val & C041_EN_25SEC_INTS)) {
				remove_irq(IRQ_PENDING_C046_25SEC);
			}
			return;
		case 0x46: /* c046 */
			/* ignore writes to c046 */
			return;
		case 0x47: /* c047 */
			remove_irq(IRQ_PENDING_C046_VBL |
						IRQ_PENDING_C046_25SEC);
			g_c046_val &= 0xe7;	/* clear vblint, 1/4sec int*/
			return;
		case 0x48: /* c048 */
			/* diversitune writes this--ignore it */
			return;
		case 0x42: /* c042 */
		case 0x43: /* c043 */
			return;
		case 0x4f: /* c04f */
			g_em_emubyte_cnt = 1;
			return;
		case 0x40: /* c040 */
		case 0x44: /* c044 */
		case 0x45: /* c045 */
		case 0x49: /* c049 */
		case 0x4a: /* c04a */
		case 0x4b: /* c04b */
		case 0x4c: /* c04c */
		case 0x4d: /* c04d */
		case 0x4e: /* c04e */
			UNIMPL_WRITE;

		/* 0xc050 - 0xc05f */
		case 0x50: /* 0xc050 */
			if(g_cur_a2_stat & ALL_STAT_TEXT) {
				g_cur_a2_stat &= (~ALL_STAT_TEXT);
				change_display_mode(dcycs);
			}
			return;
		case 0x51: /* 0xc051 */
			if((g_cur_a2_stat & ALL_STAT_TEXT) == 0) {
				g_cur_a2_stat |= (ALL_STAT_TEXT);
				change_display_mode(dcycs);
			}
			return;
		case 0x52: /* 0xc052 */
			if(g_cur_a2_stat & ALL_STAT_MIX_T_GR) {
				g_cur_a2_stat &= (~ALL_STAT_MIX_T_GR);
				change_display_mode(dcycs);
			}
			return;
		case 0x53: /* 0xc053 */
			if((g_cur_a2_stat & ALL_STAT_MIX_T_GR) == 0) {
				g_cur_a2_stat |= (ALL_STAT_MIX_T_GR);
				change_display_mode(dcycs);
			}
			return;
		case 0x54: /* 0xc054 */
			set_statereg(dcycs, g_c068_statereg & (~0x40));
			return;
		case 0x55: /* 0xc055 */
			set_statereg(dcycs, g_c068_statereg | 0x40);
			return;
		case 0x56: /* 0xc056 */
			if(g_cur_a2_stat & ALL_STAT_HIRES) {
				g_cur_a2_stat &= (~ALL_STAT_HIRES);
				fixup_hires_on();
				change_display_mode(dcycs);
			}
			return;
		case 0x57: /* 0xc057 */
			if((g_cur_a2_stat & ALL_STAT_HIRES) == 0) {
				g_cur_a2_stat |= (ALL_STAT_HIRES);
				fixup_hires_on();
				change_display_mode(dcycs);
			}
			return;
		case 0x58: /* 0xc058 */
			if(g_zipgs_unlock >= 4) {
				g_zipgs_reg_c059 &= 0x4;  /* last reset cold */
			} else {
				g_c05x_annuncs &= (~1);
			}
			return;
		case 0x59: /* 0xc059 */
			if(g_zipgs_unlock >= 4) {
				g_zipgs_reg_c059 = (val & 0xf8) |
						(g_zipgs_reg_c059 & 0x7);
			} else {
				g_c05x_annuncs |= 1;
			}
			return;
		case 0x5a: /* 0xc05a */
			g_c05x_annuncs &= (~2);
			if((val & 0xf0) == 0x50) {
				g_zipgs_unlock++;
			} else if((val & 0xf0) == 0xa0) {
				g_zipgs_unlock = 0;
			} else if(g_zipgs_unlock >= 4) {
				if((g_zipgs_reg_c05b & 0x10) == 0) {
					/* to recalculate times */
					engine_recalc_events();
				}
				g_zipgs_reg_c05b |= 0x10;	// disable
			}
			return;
		case 0x5b: /* 0xc05b */
			if(g_zipgs_unlock >= 4) {
				if((g_zipgs_reg_c05b & 0x10) != 0) {
					/* to recalculate times */
					engine_recalc_events();
				}
				g_zipgs_reg_c05b &= (~0x10);	// enable
			} else {
				g_c05x_annuncs |= 2;
			}
			return;
		case 0x5c: /* 0xc05c */
			if(g_zipgs_unlock >= 4) {
				g_zipgs_reg_c05c = val;
			} else {
				g_c05x_annuncs &= (~4);
			}
			return;
		case 0x5d: /* 0xc05d */
			if(g_zipgs_unlock >= 4) {
				if(((g_zipgs_reg_c05a ^ val) >= 0x10) &&
					((g_zipgs_reg_c05b & 0x10) == 0)) {
					engine_recalc_events();
				}
				g_zipgs_reg_c05a = val | 0xf;
			} else {
				g_c05x_annuncs |= 4;
			}
			return;
		case 0x5e: /* 0xc05e */
			if(g_zipgs_unlock >= 4) {
				/* Zippy writes 0x80 and 0x00 here... */
			} else if(g_cur_a2_stat & ALL_STAT_ANNUNC3) {
				g_cur_a2_stat &= (~ALL_STAT_ANNUNC3);
				change_display_mode(dcycs);
			}
			return;
		case 0x5f: /* 0xc05f */
			if(g_zipgs_unlock >= 4) {
				halt_printf("Wrote ZipGS $c05f: %02x\n", val);
			} else if((g_cur_a2_stat & ALL_STAT_ANNUNC3) == 0) {
				g_cur_a2_stat |= (ALL_STAT_ANNUNC3);
				change_display_mode(dcycs);
			}
			return;


		/* 0xc060 - 0xc06f */
		case 0x60: /* 0xc060 */
		case 0x61: /* 0xc061 */
		case 0x62: /* 0xc062 */
		case 0x63: /* 0xc063 */
		case 0x64: /* 0xc064 */
		case 0x65: /* 0xc065 */
		case 0x66: /* 0xc066 */
		case 0x67: /* 0xc067 */
			/* all the above do nothing--return */
			return;
		case 0x68: /* 0xc068 = STATEREG */
			set_statereg(dcycs, val);
			return;
		case 0x69: /* 0xc069 */
			/* just ignore, someone writing c068 with m=0 */
			return;
		case 0x6c: /* 0xc06c */
			g_c06c_latched_cyc = (word32)((dword64)dcycs);
			return;
		case 0x6a: /* 0xc06a */
		case 0x6b: /* 0xc06b */
		case 0x6d: /* 0xc06d */
		case 0x6e: /* 0xc06e */
		case 0x6f: /* 0xc06f */
			UNIMPL_WRITE;

		/* 0xc070 - 0xc07f */
		case 0x70: /* 0xc070 = Trigger paddles */
			paddle_trigger(dcycs);
			return;
		case 0x73: /* 0xc073 = multibank ram card bank addr? */
			return;
		case 0x71: /* 0xc071 = another multibank ram card enable? */
		case 0x7e: /* 0xc07e */
		case 0x7f: /* 0xc07f */
			return;
		case 0x72: /* 0xc072 */
		case 0x74: /* 0xc074 */
		case 0x75: /* 0xc075 */
		case 0x76: /* 0xc076 */
		case 0x77: /* 0xc077 */
		case 0x78: /* 0xc078 */
		case 0x79: /* 0xc079 */
		case 0x7a: /* 0xc07a */
		case 0x7b: /* 0xc07b */
		case 0x7c: /* 0xc07c */
		case 0x7d: /* 0xc07d */
			return;

		/* 0xc080 - 0xc08f */
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b:
		case 0x8c: case 0x8d: case 0x8e: case 0x8f:
			new_lcbank2 = ((loc >> 1) & 0x4) ^ 0x4;
			new_wrdefram = (loc & 1);
			if(new_wrdefram != g_c08x_wrdefram) {
				fixup_wrdefram(new_wrdefram);
			}
			switch(loc & 0xf) {
			case 0x1: /* 0xc081 */
			case 0x2: /* 0xc082 */
			case 0x5: /* 0xc085 */
			case 0x6: /* 0xc086 */
			case 0x9: /* 0xc089 */
			case 0xa: /* 0xc08a */
			case 0xd: /* 0xc08d */
			case 0xe: /* 0xc08e */
				/* Read rom, set lcbank2 */
				set_statereg(dcycs, (g_c068_statereg & ~(0x04))|
					(new_lcbank2 | 0x08));
				break;
			case 0x0: /* 0xc080 */
			case 0x3: /* 0xc083 */
			case 0x4: /* 0xc084 */
			case 0x7: /* 0xc087 */
			case 0x8: /* 0xc088 */
			case 0xb: /* 0xc08b */
			case 0xc: /* 0xc08c */
			case 0xf: /* 0xc08f */
				/* Read ram (clear RDROM), set lcbank2 */
				set_statereg(dcycs, (g_c068_statereg & ~(0x0c))|
					(new_lcbank2));
				break;
			}
			return;

		/* 0xc090 - 0xc09f */
		case 0x90: case 0x91: case 0x92: case 0x93:
		case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b:
		case 0x9c: case 0x9d: case 0x9e: case 0x9f:
			UNIMPL_WRITE;

		/* 0xc0a0 - 0xc0af */
		case 0xa0: case 0xa1: case 0xa3:
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf:
			UNIMPL_WRITE;
		case 0xa2:	/* Burger Times writes here on error */
		case 0xa8:
			/* Kurzweil SMP writes to 0xc0a8, ignore it */
			return;

		/* 0xc0b0 - 0xc0bf */
		case 0xb0:
			/* Second sight stuff--ignore it */
			return;
		case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			return;

		/* 0xc0c0 - 0xc0cf */
		case 0xc0: case 0xc1: case 0xc2: case 0xc3:
		case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb:
		case 0xcc: case 0xcd: case 0xce: case 0xcf:
			UNIMPL_WRITE;

		/* 0xc0d0 - 0xc0df */
		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
		case 0xd4: case 0xd5: case 0xd6: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb:
		case 0xdc: case 0xdd: case 0xde: case 0xdf:
			UNIMPL_WRITE;

		/* 0xc0e0 - 0xc0ef */
		case 0xe0: case 0xe1: case 0xe2: case 0xe3:
		case 0xe4: case 0xe5: case 0xe6: case 0xe7:
		case 0xe8: case 0xe9: case 0xea: case 0xeb:
		case 0xec: case 0xed: case 0xee: case 0xef:
			write_iwm(loc, val, dcycs);
			return;

		/* 0xc0f0 - 0xc0ff */
		case 0xf0: case 0xf1: case 0xf2: case 0xf3:
		case 0xf4: case 0xf5: case 0xf6: case 0xf7:
		case 0xf8: case 0xf9: case 0xfa: case 0xfb:
		case 0xfc: case 0xfd: case 0xfe: case 0xff:
			UNIMPL_WRITE;
		default:
			printf("Write loc: %x\n",loc);
			exit(-300);
		}
		break;
	case 1: case 2: case 3: case 5: case 6: case 7:
		/* c1000 - c7ff (but not c4xx) */
		UNIMPL_WRITE;
	case 4:
		if((g_c02d_int_crom & 0x10) && !(g_c068_statereg & 1)) {
			// Slot 4 is set to Your Card and not INTCX
			mockingboard_write(loc, val, dcycs);
			return;
		}
		return;
	case 8: case 9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe:
		// UNIMPL_WRITE;
		return;
	case 0xf:
		if((loc & 0xfff) == 0xfff) {
			/* cfff */
			return;
		}
		// UNIMPL_WRITE;
		// Wings of Fury writes to $cf00-$cfff when reading a 0x300
		//  sector where it wants to load 0x200 to 0xd000-0xd1ff
		return;
	}
	printf("Huh2? Write loc: %x\n", loc);
	exit(-290);
}

// IIgs vertical line counters
// 0x7d - 0x7f: in vbl, top of screen?
// 0x80 - 0xdf: not in vbl, drawing screen
// 0xe0 - 0xff: in vbl, bottom of screen
// Note: lines are then 0-0x60 effectively, for 192 lines, from 0x80-0xdf
// vertical blanking engages on line 192, even if in super hires mode
// (Last 8 lines in SHR are drawn with vbl_active set

word32
get_lines_since_vbl(double dcycs)
{
	double	dcycs_since_last_vbl;
	double	dlines_since_vbl;
	double	dcyc_line_start;
	word32	lines_since_vbl;
	int	offset;

	dcycs_since_last_vbl = dcycs - g_last_vbl_dcycs;

	dlines_since_vbl = dcycs_since_last_vbl * (1.0 / 65.0);
	lines_since_vbl = (int)dlines_since_vbl;
	dcyc_line_start = (double)lines_since_vbl * 65.0;

	offset = ((int)(dcycs_since_last_vbl - dcyc_line_start)) & 0xff;

	lines_since_vbl = (lines_since_vbl << 8) + offset;

	if(lines_since_vbl < 0x10680) {
		return lines_since_vbl;
	} else {
		halt_printf("lines_since_vbl: %08x!\n", lines_since_vbl);
		printf("dc_s_l_v: %f, dcycs: %f, last_vbl_cycs: %f\n",
			dcycs_since_last_vbl, dcycs, g_last_vbl_dcycs);
		show_dtime_array();
		show_all_events();
		/* U_STACK_TRACE(); */
	}

	return lines_since_vbl;
}


int
in_vblank(double dcycs)
{
	int	lines_since_vbl;

	lines_since_vbl = get_lines_since_vbl(dcycs);

	if(lines_since_vbl >= 0xc000) {
		return 1;
	}

	return 0;
}

// horizontal video counter goes from 0x00,0x40 - 0x7f, then 0x80,0xc0-0xff
// over 2*65 cycles.  The last visible screen pos is 0x7f and 0xff
// KEGS starts it's "line 0" at the start of the right border for line -1
// See get_lines_since_vbl comment for the format of the vertical counter
int
read_vid_counters(int loc, double dcycs)
{
	word32	mask;
	int	lines_since_vbl;

	loc = loc & 0xf;

	lines_since_vbl = get_lines_since_vbl(dcycs);

	lines_since_vbl += 0x10000;
	if(lines_since_vbl >= 0x20000) {
		lines_since_vbl = lines_since_vbl - 0x20000 + 0xfa00;
	}

	if(lines_since_vbl > 0x1ffff) {
		halt_printf("lines_since_vbl: %04x, dcycs: %f, last_vbl: %f\n",
			lines_since_vbl, dcycs, g_last_vbl_dcycs);
	}

	if(loc == 0xe) {
		/* Vertical count */
		return (lines_since_vbl >> 9) & 0xff;
	}

	mask = (lines_since_vbl >> 1) & 0x80;

	lines_since_vbl = (lines_since_vbl & 0xff);
	if(lines_since_vbl >= 0x01) {
		lines_since_vbl = (lines_since_vbl + 0x3f) & 0x7f;
	}
	return (mask | (lines_since_vbl & 0xff));
}

