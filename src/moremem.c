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

const char rcsid_moremem_c[] = "@(#)$Header: moremem.c,v 1.195 99/01/31 22:16:45 kentd Exp $";

#include "defc.h"

extern long timezone;
extern int daylight;

extern byte *g_memory_ptr;
extern byte *g_dummy_memory1_ptr;
extern byte *g_slow_memory_ptr;
extern byte *g_rom_fc_ff_ptr;

extern word32 slow_mem_changed[];

extern int g_num_breakpoints;
extern word32 g_breakpts[];

extern int halt_sim;
extern double g_dcycles_in_16ms;
extern double g_drecip_cycles_in_16ms;
extern double g_last_vbl_dcycs;
extern int speed_changed;

extern Page_info page_info_rd_wr[];

extern int scr_mode;

extern int Verbose;
extern double g_paddle_trig_dcycs;
extern int g_rom_version;

extern Fplus *g_cur_fplus_ptr;

/* from iwm.c */
extern int head_35;
extern int g_apple35_sel;
extern int cur_drive;


int	statereg;
int	old_statereg = 0xffff;
int	halt_on_c023 = 0;
int	halt_on_c025 = 0;
int	halt_on_c02a = 0;
int	halt_on_c032 = 0;
int	halt_on_c039 = 0;
int	halt_on_c038 = 1;
int	halt_on_c041 = 0;
int	halt_on_c046 = 0;
int	halt_on_c047 = 0;
int	halt_on_shadow_reg = 0;

extern Engine_reg engine;

#define IOR(val) ( (val) ? 0x80 : 0x00 )

int	linear_vid = 1;
int	bank1latch = 0;

int	wrdefram = 0;
int	int_crom[8] = { 0, 0, 0, 0,  0, 0, 0, 0 };

extern int g_cur_a2_stat;

int	annunc_0 = 0;
int	annunc_1 = 0;
int	annunc_2 = 0;

int	shadow_reg = 0x08;

int	stop_on_c03x = 0;

extern int doc_ptr;

int	shadow_text = 1;

int	g_border_color = 0;

int	speed_fast = 1;
word32	g_slot_motor_detect = 0;
int	power_on_clear = 0;


int	c023_vgc_int = 0;
int	c023_1sec_int = 0;
int	c023_scan_int = 0;
int	c023_ext_int = 0;
int	c023_1sec_en = 0;
int	c023_scan_en = 0;
int	c023_ext_en = 0;
int	c023_scan_int_irq_pending = 0;
int	c023_1sec_int_irq_pending = 0;

int	c02b_val = 0x08;

int	c039_write_val = 0;

int	c041_en_25sec_ints = 0;
int	c041_en_vbl_ints = 0;
int	c041_en_switch_ints = 0;
int	c041_en_move_ints = 0;
int	c041_en_mouse = 0;

int	c046_mouse_down = 0;
int	c046_mouse_last = 0;
int	c046_an3_status = 0;
int	c046_25sec_int_status = 0;
int	c046_25sec_irq_pend = 0;
int	c046_vbl_int_status = 0;
int	c046_vbl_irq_pending = 0;
int	c046_switch_int_status = 0;
int	c046_move_int_status = 0;
int	c046_system_irq_status = 0;


#define UNIMPL_READ	\
	printf("UNIMP READ to addr %08x\n", loc);	\
	set_halt(1);	\
	return 0;

#define UNIMPL_WRITE	\
	printf("UNIMP WRITE to addr %08x, val: %04x\n", loc, val);	\
	set_halt(1);	\
	return;

void
fixup_brks()
{
	word32	page;
	word32	val;
	int	i, num;

	num = g_num_breakpoints;
	for(i = 0; i < num; i++) {
		page = (g_breakpts[i] >> 8) & 0xffff;
		val = GET_PAGE_INFO_RD(page);
		SET_PAGE_INFO_RD(page, val | BANK_IO_TMP | BANK_BREAK);
		val = GET_PAGE_INFO_WR(page);
		SET_PAGE_INFO_WR(page, val | BANK_IO_TMP | BANK_BREAK);
	}
}

void
fixup_bank0_0000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	word32	ramrd_add_rd;
	word32	ramwrt_add_wr;
	word32	add_rd, add_wr;
	int	shadow;
	int	j;

	add_rd = 0;
	if(mask & 0x0003) {
		/* pages 0 and 1 */
		if(ALTZP) {
			add_rd = 0x10000;
		}
		SET_PAGE_INFO_RD(0, mem0rd + add_rd);
		SET_PAGE_INFO_WR(0, mem0wr + add_rd);
		add_rd += 0x100;
		SET_PAGE_INFO_RD(1, mem0rd + add_rd);
		SET_PAGE_INFO_WR(1, mem0wr + add_rd);
	}

	ramrd_add_rd = 0;
	if(RAMRD) {
		ramrd_add_rd = 0x10000;
	}

	ramwrt_add_wr = 0;
	if(RAMWRT) {
		ramwrt_add_wr = 0x10000;
	}

	if(mask & 0x000c) {
		/* pages 2 and 3 */
		SET_PAGE_INFO_RD(2, mem0rd + ramrd_add_rd + 0x200);
		SET_PAGE_INFO_WR(2, mem0wr + ramwrt_add_wr + 0x200);
		SET_PAGE_INFO_RD(3, mem0rd + ramrd_add_rd + 0x300);
		SET_PAGE_INFO_WR(3, mem0wr + ramwrt_add_wr + 0x300);
	}

	if(mask & 0x000000f0) {
		/* pages 4-7 */
		add_rd = ramrd_add_rd + 0x400;
		add_wr = ramwrt_add_wr + 0x400;
		shadow = BANK_SHADOW;
		if(g_cur_a2_stat & ALL_STAT_ST80) {
			if(PAGE2) {

/* What if superhires is on?? */

				add_rd = 0x10400;
				add_wr = 0x10400;
				shadow = BANK_SHADOW2;
			} else {
				add_rd = 0x0400;
				add_wr = 0x0400;
			}
		}
		if((shadow_reg & 0x01) == 0) {
			/* shadow */
			add_wr += shadow;
		}
		
		for(j = 4; j < 8; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add_rd);
			SET_PAGE_INFO_WR(j, mem0wr + add_wr);
			add_rd += 0x100;
			add_wr += 0x100;
		}
	}
	if(mask & 0x00000f00) {
		/* pages 8 - b */
		add_rd = ramrd_add_rd + 0x800;
		add_wr = ramwrt_add_wr + 0x800;
		shadow = BANK_SHADOW;
		if((shadow_reg & 0x20) == 0 && g_rom_version >= 3) {
			/* shadow */
			add_wr += shadow;
		}

		for(j = 8; j < 0xc; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add_rd);
			SET_PAGE_INFO_WR(j, mem0wr + add_wr);
			add_rd += 0x100;
			add_wr += 0x100;
		}
	}

	if(mask & 0xfffff000) {
		/* pages c-1f */
		add_rd = ramrd_add_rd + 0xc00;
		add_wr = ramwrt_add_wr + 0xc00;
		for(j = 0xc; j < 0x20; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add_rd);
			SET_PAGE_INFO_WR(j, mem0wr + add_wr);
			add_rd += 0x100;
			add_wr += 0x100;
		}
	}
}


void
fixup_bank1_0000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	word32	add;
	int	j;

	add = 0;
	if(mask & 0x0000000f) {
		/* pages 0 - 3 */
		for(j = start_page; j < start_page + 4; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add);
			SET_PAGE_INFO_WR(j, mem0wr + add);
			add += 0x100;
		}
	}

	if(mask & 0xfffff000) {
		/* pages c-1f */
		add = 0xc00;
		for(j = start_page + 0xc; j < start_page + 0x20; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add);
			SET_PAGE_INFO_WR(j, mem0wr + add);
			add += 0x100;
		}
	}

	if(mask & 0x000000f0) {
		/* pages 4-7 */
		add = 0x400;
		if((shadow_reg & 0x01) == 0) {
			/* shadow */
			add += BANK_SHADOW2;
		}
		
		for(j = start_page + 4; j < start_page + 8; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add);
			SET_PAGE_INFO_WR(j, mem0wr + add);
			add += 0x100;
		}
	}

	if(mask & 0x00000f00) {
		/* pages 8-b */
		add = 0x800;
		if((shadow_reg & 0x20) == 0 && g_rom_version >= 3) {
			/* shadow */
			add += BANK_SHADOW2;
		}
		
		for(j = start_page + 8; j < start_page + 0xc; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add);
			SET_PAGE_INFO_WR(j, mem0wr + add);
			add += 0x100;
		}
	}

}

void
fixup_bank0_2000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	if((g_cur_a2_stat & ALL_STAT_ST80) && (g_cur_a2_stat & ALL_STAT_HIRES)){
		if(PAGE2) {

/* What if superhires is on?? */

			mem0rd += 0x10000;
			mem0wr += 0x10000;
			if((shadow_reg & 0x12) == 0 || (shadow_reg & 0x8) == 0){
				mem0wr += BANK_SHADOW2;
			}
		} else if((shadow_reg & 0x02) == 0) {
			mem0wr += BANK_SHADOW;
		}
		
	} else {
		if(RAMRD) {
			mem0rd += 0x10000;
		}
		if(RAMWRT) {
			mem0wr += 0x10000;
			if((shadow_reg & 0x12) == 0 || (shadow_reg & 0x8) == 0){
				mem0wr += BANK_SHADOW2;
			}
		} else if((shadow_reg & 0x02) == 0) {
			mem0wr += BANK_SHADOW;
		}
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}

void
fixup_bank1_2000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	if((shadow_reg & 0x12) == 0 || (shadow_reg & 0x8) == 0) {
		mem0wr += BANK_SHADOW2;
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}

void
fixup_bank0_1_4000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{

	if(start_page < 0x100) {
		if(RAMRD) {
			mem0rd += 0x10000;
		}

		if(RAMWRT) {
			mem0wr += 0x10000;
			if((shadow_reg & 0x14) == 0 || (shadow_reg & 0x8) == 0){
				mem0wr |= BANK_SHADOW2;
			}
		} else if((shadow_reg & 0x04) == 0) {
			mem0wr |= BANK_SHADOW;
		}
	} else {
		if((shadow_reg & 0x14) == 0 || (shadow_reg & 0x8) == 0) {
			mem0wr |= BANK_SHADOW2;
		}
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}


void
fixup_bank0_1_6000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	if(start_page < 0x100) {
		if(RAMRD) {
			mem0rd += 0x10000;
		}

		if(RAMWRT) {
			mem0wr += 0x10000;
			if((shadow_reg & 0x8) == 0) {
				mem0wr |= BANK_SHADOW2;
			}
		}
	} else {
		if((shadow_reg & 0x8) == 0) {
			mem0wr |= BANK_SHADOW2;
		}
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}

void
fixup_bank0_1_8000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	if(start_page < 0x100) {
		if(RAMRD) {
			mem0rd += 0x10000;
		}

		if(RAMWRT) {
			mem0wr += 0x10000;
			if((shadow_reg & 0x8) == 0) {
				mem0wr += BANK_SHADOW2;
			}
		}
	} else {
		if((shadow_reg & 0x8) == 0) {
			mem0wr |= BANK_SHADOW2;
		}
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}

void
fixup_bank0_1_a000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	if(start_page < 0x100) {
		if(RAMRD) {
			mem0rd += 0x10000;
		}

		if(RAMWRT) {
			mem0wr += 0x10000;
		}
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}


void
fixup_any_bank_c000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	word32	rom10000;
	word32	add;
	word32	add_rd, add_wr;
	word32	rom_inc;
	int	indx;
	int	j;

	rom10000 = (word32)&(g_rom_fc_ff_ptr[0x30000]);
	add = 0;

	if((shadow_reg & 0x40) && start_page < 0x200) {
		/* LC, I/O shadow off in banks 0 & 1 */
		/* ignore ALTZP, it does not affect c000-d000 */

		/* 1c000: LCBANK2 */
		add = ((start_page + 0x10) & 0xff) << 8;
		for(j = start_page; j < start_page + 0x10; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add);
			SET_PAGE_INFO_WR(j, mem0wr + add);
			add += 0x100;
		}

		/* and 1d000-1dfff */
		/* d000: LCBANK1 */
		add = (start_page & 0xff) << 8;
		/* In no I/O shadow, never ROM in banks 0 or 1 */
		if(start_page < 0x100 && ALTZP) {
			mem0rd += 0x10000;
			mem0wr += 0x10000;
		}

		for(j = start_page + 0x10; j < start_page + 0x20; j++) {
			SET_PAGE_INFO_RD(j, mem0rd + add);
			SET_PAGE_INFO_WR(j, mem0wr + add);
			add += 0x100;
		}

		return;

	}

	/* shadow_reg & 0x40 done...I/O area & normal 1d000-1dfff */
	/* I/O area! */
	SET_PAGE_INFO_RD(start_page, SET_BANK_IO);
	SET_PAGE_INFO_WR(start_page, SET_BANK_IO);
	rom_inc = rom10000 + 0xc100;
	for(j = start_page + 1; j < start_page + 0x10; j++) {
		indx = j & 0xf;
		if(indx < 8) {
			if((int_crom[indx] == 0) || INTCX) {
				SET_PAGE_INFO_RD(j, rom_inc);
			} else {
				SET_PAGE_INFO_RD(j, SET_BANK_IO);
			}
		} else {
			/* c800 - cfff */
			if(int_crom[3] == 0 || INTCX) {
				SET_PAGE_INFO_RD(j, rom_inc);
			} else {
				/* c800 space not necessarily mapped */
				/*   just map in ROM */
				SET_PAGE_INFO_RD(j, rom_inc);
#if 0
				printf("c8000-cfff space not map!\n");
				set_halt(1);
#endif
			}
		}
		SET_PAGE_INFO_WR(j, SET_BANK_IO);
		rom_inc += 0x100;
	}

	SET_PAGE_INFO_RD(start_page + 7, SET_BANK_IO);		/* smartport */

	/* and do 0xd000 - 0xdfff now */

	add = ((start_page + 0x10) & 0xff) << 8;
	add_wr = 0;
	add_rd = 0;

	if(!wrdefram && (start_page < 0x200)) {
		mem0wr |= (BANK_IO_TMP | BANK_IO2_TMP);
	}

	if(start_page < 0x100) {
		if(ALTZP) {
			mem0rd += 0x10000;
			mem0wr += 0x10000;
		}
	}

	if( ! LCBANK2) {
		/* lcbank1, use area 0xc000-0xd000 */
		add_wr = -0x1000;
		add_rd = -0x1000;
	}

	if(RDROM && (start_page < 0x200)) {
		/* do rom */
		mem0rd = rom10000;
		add_rd = 0;
	}

	for(j = start_page + 0x10; j < start_page + 0x20; j++) {
		SET_PAGE_INFO_RD(j, mem0rd + add + add_rd);
		SET_PAGE_INFO_WR(j, mem0wr + add + add_wr);
		add += 0x100;
	}
}


void
fixup_any_bank_e000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	word32	rom10000;

	rom10000 = (word32)&(g_rom_fc_ff_ptr[0x30000]);

	if((shadow_reg & 0x40) && start_page < 0x200) {
		/* LC shadowing off! */
		/* Must be RAM in d000-ffff */
		if(ALTZP) {
			mem0rd += 0x10000;
			mem0wr += 0x10000;
		}
	} else {
		/* decide on ROM or RAM */
		if(!wrdefram && start_page < 0x200) {
			mem0wr |= (BANK_IO_TMP | BANK_IO2_TMP);
		}

		if((start_page < 0x100) && ALTZP) {
			mem0rd += 0x10000;
			mem0wr += 0x10000;
		}

		if(RDROM && (start_page < 0x200)) {
			/* do rom */
			mem0rd = rom10000;
		}
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}

void
fixup_banke0_e1_0000(word32 mask, int start_page, word32 mem0rd, word32 mem0wr)
{
	word32	add;
	int	j;

	add = 0;

	for(j = start_page; j < start_page + 0x20; j++) {
		SET_PAGE_INFO_RD(j, mem0rd + add);
		SET_PAGE_INFO_WR(j, mem0wr + add);
		add += 0x100;
	}

	/* pages 4-b */
	if(start_page & 0x100) {
		add = 0x400 + BANK_SHADOW2;
	} else {
		add = 0x400 + BANK_SHADOW;
	}
	for(j = start_page + 4; j < start_page + 0xc; j++) {
		SET_PAGE_INFO_RD(j, mem0rd + add);
		SET_PAGE_INFO_WR(j, mem0wr + add);
		add += 0x100;
	}
}

void
fixup_banke0_e1_2_4000(word32 mask, int start_page, word32 mem0rd,
			word32 mem0wr)
{
	if(start_page & 0x100) {
		mem0wr += BANK_SHADOW2;
	} else {
		mem0wr += BANK_SHADOW;
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}

void
fixup_banke0_e1_6_8000(word32 mask, int start_page, word32 mem0rd,
			word32 mem0wr)
{
	if(start_page & 0x100) {
		mem0wr += BANK_SHADOW2;
	}

	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}

void
fixup_banke0_e1_a000(word32 mask, int start_page, word32 mem0rd,
			word32 mem0wr)
{
	fixup_any_bank_any_page(mask, start_page, mem0rd, mem0wr);
}

void
fixup_any_bank_any_page(word32 mask, int start_page, word32 mem0rd,
	word32 mem0wr)
{
	word32	add;
	int	j;

	add = (start_page & 0xff) << 8;

	for(j = start_page; j < start_page + 0x20; j++) {
		SET_PAGE_INFO_RD(j, mem0rd + add);
		SET_PAGE_INFO_WR(j, mem0wr + add);
		add += 0x100;
	}
}

typedef void (*Bank_func)(word32 mask, int start_page, word32 mem0rd,
					word32 mem0wr);

const Bank_func fixup_bank0_ptrs[] = {
	fixup_bank0_0000,
	fixup_bank0_2000,
	fixup_bank0_1_4000,
	fixup_bank0_1_6000,
	fixup_bank0_1_8000,		/* 08000 */
	fixup_bank0_1_a000,		/* 0a000 */
	fixup_any_bank_c000,		/* 0c000 */
	fixup_any_bank_e000		/* 0e000 */
};

const Bank_func fixup_bank1_ptrs[] = {
	fixup_bank1_0000,
	fixup_bank1_2000,
	fixup_bank0_1_4000,
	fixup_bank0_1_6000,
	fixup_bank0_1_8000,		/* 18000 */
	fixup_bank0_1_a000,		/* 1a000 */
	fixup_any_bank_c000,		/* 1c000 */
	fixup_any_bank_e000		/* 1e000 */
};

const Bank_func fixup_banke0_ptrs[] = {
	fixup_banke0_e1_0000,
	fixup_banke0_e1_2_4000,
	fixup_banke0_e1_2_4000,
	fixup_banke0_e1_6_8000,
	fixup_banke0_e1_6_8000,		/* 08000 */
	fixup_banke0_e1_a000,		/* 0a000 */
	fixup_any_bank_c000,		/* 0c000 */
	fixup_any_bank_e000		/* 0e000 */
};

const Bank_func fixup_banke1_ptrs[] = {
	fixup_banke0_e1_0000,
	fixup_banke0_e1_2_4000,
	fixup_banke0_e1_2_4000,
	fixup_banke0_e1_6_8000,
	fixup_banke0_e1_6_8000,		/* 08000 */
	fixup_banke0_e1_a000,		/* 0a000 */
	fixup_any_bank_c000,		/* 0c000 */
	fixup_any_bank_e000		/* 0e000 */
};

word32 bank0_adjust_mask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
word32 bank1_adjust_mask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
word32 banke0_adjust_mask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
word32 banke1_adjust_mask[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

void
fixup_bank0()
{
	word32	mask;
	word32	mem0;
	int	i;

	mem0 = (word32)&(g_memory_ptr[0]);
	for(i = 0; i < 8; i++) {
		mask = bank0_adjust_mask[i];
		if(mask == 0) {
			continue;
		}
		bank0_adjust_mask[i] = 0;
		(*fixup_bank0_ptrs[i])(mask, i*32, mem0, mem0);
	}
}

void
fixup_bank1()
{
	word32	mem1;
	word32	mask;
	int	i;

	mem1 = (word32)&(g_memory_ptr[0x10000]);
	for(i = 0; i < 8; i++) {
		mask = bank1_adjust_mask[i];
		if(mask == 0) {
			continue;
		}
		bank1_adjust_mask[i] = 0;
		(*fixup_bank1_ptrs[i])(mask, 0x100 + i*32, mem1, mem1);
	}
}

void
fixup_banke0()
{
	word32	mask;
	word32	mem0;
	int	i;

	mem0 = (word32)&(g_slow_memory_ptr[0]);
	for(i = 0; i < 8; i++) {
		mask = banke0_adjust_mask[i];
		if(mask == 0) {
			continue;
		}
		banke0_adjust_mask[i] = 0;
		(*fixup_banke0_ptrs[i])(mask, 0xe000 + i*32, mem0, mem0);
	}
}

void
fixup_banke1()
{
	word32	mask;
	word32	mem1;
	int	i;

	mem1 = (word32)&(g_slow_memory_ptr[0x10000]);
	for(i = 0; i < 8; i++) {
		mask = banke1_adjust_mask[i];
		if(mask == 0) {
			continue;
		}
		banke1_adjust_mask[i] = 0;
		(*fixup_banke1_ptrs[i])(mask, 0xe100 + i*32, mem1, mem1);
	}
}

void
fixup_intcx(int call_fixups)
{
	bank0_adjust_mask[6] |= 0xffff;
	bank1_adjust_mask[6] |= 0xffff;
	banke0_adjust_mask[6] |= 0xffff;
	banke1_adjust_mask[6] |= 0xffff;

	if(call_fixups) {
		fixup_bank0();
		fixup_bank1();
		fixup_banke0();
		fixup_banke1();
		fixup_brks();
	}
}

void
fixup_st80col(double dcycs, int call_fixups)
{
	/* pages 4 - 7 */
	bank0_adjust_mask[0] |= 0xf0;

	/* 2000-4000 also */
	if((g_cur_a2_stat & ALL_STAT_HIRES) && PAGE2) {
		bank0_adjust_mask[1] = -1;
	}

	if(PAGE2) {
		change_display_mode(dcycs);
	}

	if(call_fixups) {
		fixup_bank0();
		fixup_brks();
	}
}

void
fixup_hires_on(int call_fixups)
{
	if((g_cur_a2_stat & ALL_STAT_ST80) && PAGE2) {
		/* 2000-4000 also */
		bank0_adjust_mask[1] = -1;
		if(call_fixups) {
			fixup_bank0();
			fixup_brks();
		}
	}
}

void
fixup_page2(double dcycs, int call_fixups)
{
	if((g_cur_a2_stat & ALL_STAT_ST80)) {
		/* pages 4 - 7 */
		bank0_adjust_mask[0] |= 0xf0;

		/* 2000-4000 also */
		if((g_cur_a2_stat & ALL_STAT_HIRES)) {
			bank0_adjust_mask[1] = -1;
		}
		if(call_fixups) {
			fixup_bank0();
			fixup_brks();
		}
	} else {
		change_display_mode(dcycs);
	}
}

void
fixup_wrdefram(int new_wrdefram, int call_fixups)
{
	wrdefram = new_wrdefram;

	/* called if wrdefram changes */
	bank0_adjust_mask[6] = 0xffff0000;
	bank0_adjust_mask[7] = 0xffffffff;
	bank1_adjust_mask[6] = 0xffff0000;
	bank1_adjust_mask[7] = 0xffffffff;

	if(call_fixups) {
		fixup_bank0();
		fixup_bank1();
		fixup_brks();
	}
}

void
set_statereg(double dcycs, int val)
{
	int	xor;
	int	must_fixup_e0e1;
	int	must_fixup_1;

	xor = val ^ statereg;
	old_statereg = statereg;
	statereg = val;
	if(xor == 0) {
		return;
	}

	must_fixup_e0e1 = 0;
	must_fixup_1 = 0;
	if(xor & 0x80) {
		/* altzp */

		bank0_adjust_mask[0] |= 0x3;

		/* c000 since if state_reg & 0x40, it depends on altzp */
		/* and d000-ffff is necessary */
		bank0_adjust_mask[6] = 0xffffffff;
		bank0_adjust_mask[7] = 0xffffffff;
	}
	if(xor & 0x40) {
		/* page2 */
		fixup_page2(dcycs, 0);
	}

	if(xor & 0x30) {
		/* RAMRD/RAMWRT */
		bank0_adjust_mask[0] |= 0xfffffffc;
		bank0_adjust_mask[1] = -1;
		bank0_adjust_mask[2] = -1;
		bank0_adjust_mask[3] = -1;
		bank0_adjust_mask[4] = -1;
		bank0_adjust_mask[5] = -1;
	}

	if(xor & 0x0c) {
		/* RDROM/LCBANK2 */
		bank0_adjust_mask[6] |= 0xffff0000;
		bank1_adjust_mask[6] |= 0xffff0000;
		banke0_adjust_mask[6] |= 0xffff0000;
		banke1_adjust_mask[6] |= 0xffff0000;
		bank0_adjust_mask[7] = -1;
		bank1_adjust_mask[7] = -1;
		banke0_adjust_mask[7] = -1;
		banke1_adjust_mask[7] = -1;
		must_fixup_e0e1 = 1;
		must_fixup_1 = 1;
	}

	if(xor & 0x02) {
		printf("just set rombank = %d\n", ROMB);
		set_halt(1);
	}

	if(xor & 0x01) {
		fixup_intcx(0);
		must_fixup_e0e1 = 1;
		must_fixup_1 = 1;
	}

	fixup_bank0();
	if(must_fixup_1) {
		fixup_bank1();
	}
	if(must_fixup_e0e1) {
		fixup_banke0();
		fixup_banke1();
	}
	fixup_brks();
}


void
setup_bank0()
{
	int i;

	shadow_printf("setup bank0!\n");

	for(i = 0; i < 8; i++) {
		bank0_adjust_mask[i] = -1;
	}

	fixup_bank0();
	fixup_brks();
}

void
setup_bank1()
{
	int i;

	shadow_printf("setup bank1!\n");

	for(i = 0; i < 8; i++) {
		bank1_adjust_mask[i] = -1;
	}

	fixup_bank1();
	fixup_brks();
}

void
setup_banke0()
{
	int i;

	shadow_printf("setup banke0!\n");

	for(i = 0; i < 8; i++) {
		banke0_adjust_mask[i] = -1;
	}

	fixup_banke0();
	fixup_brks();
}

void
setup_banke1()
{
	int i;

	shadow_printf("setup banke1!\n");

	for(i = 0; i < 8; i++) {
		banke1_adjust_mask[i] = -1;
	}

	fixup_banke1();
	fixup_brks();
}

void
setup_pageinfo()
{
	word32	new_addr;
	int	i;

	setup_bank0();
	setup_bank1();

	/* bank 2 through last bank */
	for(i = 0x0200; i < (MEM_SIZE/256); i++) {
		SET_PAGE_INFO_RD(i, (word32)&g_memory_ptr[i*256]);
		SET_PAGE_INFO_WR(i, (word32)&g_memory_ptr[i*256]);
	}
	for(i = (MEM_SIZE/256); i < 0xfc00; i++) {
		SET_PAGE_INFO_RD(i, BANK_BAD_MEM);
		SET_PAGE_INFO_WR(i, BANK_BAD_MEM);
	}

	setup_banke0();
	setup_banke1();

	for(i = 0xfc00; i <= 0xffff; i++) {
		new_addr = (i - 0xfc00)*256;
		SET_PAGE_INFO_RD(i, (word32)(&g_rom_fc_ff_ptr[new_addr]));
		SET_PAGE_INFO_WR(i, (word32)(&g_rom_fc_ff_ptr[new_addr]) |
						(BANK_IO_TMP | BANK_IO2_TMP));
	}
	fixup_brks();
}

void
fixup_all_banks()
{
	setup_pageinfo();
}

void
update_shadow_reg(int val)
{
	int	xor;

	if(shadow_reg == val) {
		return;
	}

#if 0
	/* eddy@cs.ucdavis.edu has a demo which write bit 7, but just */
	/*  ignore it */
	if((val & 0x80) != 0) {
		printf("shadow_reg: %02x is illegal!\n", val);
		set_halt(1);
	}
#endif

	xor = shadow_reg ^ val;

	shadow_reg = val;
	/* Note, even if just superhires changing, must update bank 0 */
	/*  to handle case where AUXCARD on, so that 0x2000-0xa000 = bank 1 */

	shadow_printf("shadow_reg: %02x\n", val);

	/* HACK:  CAN DO THIS BETTER! */
	setup_bank0();
	setup_bank1();

	set_halt(halt_on_shadow_reg);

	fixup_brks();
}

void
show_bankptrs_bank0rdwr()
{
	show_bankptrs(0);
	show_bankptrs(1);
	show_bankptrs(0xe0);
	show_bankptrs(0xe1);
}

void
show_bankptrs(int bnk)
{
	int i;
	word32 rd, wr;
	byte *ptr_rd, *ptr_wr;

	printf("g_memory_ptr: %08x, dummy_mem: %08x, slow_mem_ptr: %08x\n",
		(word32)g_memory_ptr, (word32)g_dummy_memory1_ptr,
		(word32)g_slow_memory_ptr);
	printf("g_rom_fc_ff_ptr: %08x\n", (word32)g_rom_fc_ff_ptr);

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

	if(ptr >= g_memory_ptr && ptr < &g_memory_ptr[MEM_SIZE]) {
		printf("%08x--memory[%06x]", (word32)ptr,
					(word32)(ptr - g_memory_ptr));
	} else if(ptr >= g_rom_fc_ff_ptr && ptr < &g_rom_fc_ff_ptr[256*1024]) {
		printf("%08x--rom_fc_ff[%06x]", (word32)ptr,
					(word32)(ptr - g_rom_fc_ff_ptr));
	} else if(ptr >= g_slow_memory_ptr && ptr<&g_slow_memory_ptr[128*1024]){
		printf("%08x--slow_memory[%06x]", (word32)ptr,
					(word32)(ptr - g_slow_memory_ptr));
	} else if(ptr >=g_dummy_memory1_ptr && ptr < &g_dummy_memory1_ptr[256]){
		printf("%08x--dummy_memory[%06x]", (word32)ptr,
					(word32)(ptr - g_dummy_memory1_ptr));
	} else {
		printf("%08x--unknown", (word32)ptr);
	}
}


#if 0
#define CALC_DCYCS_FROM_CYC_PTR(dcycs, cyc_ptr, fcyc, new_fcyc)	\
	fcyc = *cyc_ptr;					\
	new_fcyc = (int)(fcyc + g_cur_fplus_ptr->plus_x_minus_1); \
	*cyc_ptr = new_fcyc;					\
	dcycs = g_last_vbl_dcycs + new_fcyc;
#endif

#define CALC_DCYCS_FROM_CYC_PTR(dcycs, cyc_ptr, fcyc, new_fcyc)	\
	dcycs = g_last_vbl_dcycs + *cyc_ptr;


int dummy = 0;

int
io_read(word32 loc, Cyc *cyc_ptr)
{
	double	dcycs;
#if 0
	float	fcyc, new_fcyc;
#endif
	int new_lcbank2;
	int new_wrdefram;
	int tmp;
	int slot;
	int i;

	CALC_DCYCS_FROM_CYC_PTR(dcycs, cyc_ptr, fcyc, new_fcyc);

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
		case 0x15: /* c015 = INTCX */
			return IOR(INTCX);
		case 0x16: /* c016: ALTZP */
			return IOR(ALTZP);
		case 0x17: /* c017: rdc3rom */
			return IOR(int_crom[3]);
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
			return IOR(PAGE2);
		case 0x1d: /* c01d: rdhires */
			return IOR(g_cur_a2_stat & ALL_STAT_HIRES);
		case 0x1e: /* c01e: altcharset on? */
			return IOR(g_cur_a2_stat & ALL_STAT_ALTCHARSET);
		case 0x1f: /* c01f: rd80vid */
			return IOR(g_cur_a2_stat & ALL_STAT_VID80);

		/* 0xc020 - 0xc02f */
		case 0x20: /* 0xc020 */
			/* Click cassette port */
			return 0x00;
		case 0x21: /* 0xc021 */
			UNIMPL_READ;
		case 0x22: /* 0xc022 */
			return (g_cur_a2_stat >> BIT_ALL_STAT_BG_COLOR) & 0xff;
		case 0x23: /* 0xc023 */
			tmp = ( (c023_vgc_int << 7) + (c023_1sec_int << 6) +
				(c023_scan_int << 5) + (c023_ext_int << 4) +
				(c023_1sec_en << 2) + (c023_scan_en << 1) +
				(c023_ext_en) );
			return tmp;
		case 0x24: /* 0xc024 */
			return mouse_read_c024();
		case 0x25: /* 0xc025 */
			return adb_read_c025();
		case 0x26: /* 0xc026 */
			return adb_read_c026();
		case 0x27: /* 0xc027 */
			return adb_read_c027();
		case 0x28: /* 0xc028 */
			UNIMPL_READ;
		case 0x29: /* 0xc029 */
			return((g_cur_a2_stat & 0xa0) | (linear_vid<<6) |
				(bank1latch));
		case 0x2a: /* 0xc02a */
#if 0
			printf("Reading c02a...returning 0\n");
#endif
			set_halt(halt_on_c02a);
			return 0;
		case 0x2b: /* 0xc02b */
			return c02b_val;
		case 0x2c: /* 0xc02c */
			/* printf("reading c02c, returning 0\n"); */
			return 0;
		case 0x2d: /* 0xc02d */
			tmp = 0;
			for(i = 0; i < 8; i++) {
				tmp = tmp | (int_crom[i] << i);
			}
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
			return (head_35 << 7) | (g_apple35_sel << 6);
		case 0x32: /* 0xc032 */
			/* scan int */
			return 0;
		case 0x33: /* 0xc033 = CLOCKDATA*/
			return clock_read_c033();
		case 0x34: /* 0xc034 = CLOCKCTL */
			return clock_read_c034();
		case 0x35: /* 0xc035 */
			return shadow_reg;
		case 0x36: /* 0xc036 = CYAREG */
			tmp = (speed_fast << 7) + (power_on_clear << 6) +
				g_slot_motor_detect;
			return tmp;
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
			return (doc_ptr & 0xff);
		case 0x3f: /* 0xc03f */
			return (doc_ptr >> 8);

		/* 0xc040 - 0xc04f */
		case 0x40: /* 0xc040 */
			/* cassette */
			return 0;
		case 0x41: /* 0xc041 */
			set_halt(halt_on_c041);
			tmp = ((c041_en_25sec_ints << 4) +
				(c041_en_vbl_ints << 3) +
				(c041_en_switch_ints << 2) +
				(c041_en_move_ints << 1) + (c041_en_mouse) );
			return tmp;
		case 0x45: /* 0xc045 */
			printf("Mega II mouse read: c045\n");
			set_halt(1);
			return 0;
		case 0x46: /* 0xc046 */
			tmp = c046_mouse_last;
			c046_mouse_last = c046_mouse_down;
			set_halt(halt_on_c046);
			tmp = ((c046_mouse_down << 7) | (tmp << 6) |
				(c046_an3_status << 5) |
				(c046_25sec_int_status << 4) |
				(c046_vbl_int_status << 3) |
				(c046_switch_int_status << 2) |
				(c046_move_int_status << 1) |
				(c046_system_irq_status) );
			return tmp;
		case 0x47: /* 0xc047 */
			if(c046_vbl_irq_pending) {
				remove_irq();
				c046_vbl_irq_pending = 0;
			}
			if(c046_25sec_irq_pend) {
				remove_irq();
				c046_25sec_irq_pend = 0;
			}
			c046_vbl_int_status = 0;
			c046_25sec_int_status = 0;
			return 0;
		case 0x42: /* 0xc042 */
		case 0x43: /* 0xc043 */
		case 0x44: /* 0xc044 */
		case 0x48: /* 0xc048 */
		case 0x49: /* 0xc049 */
		case 0x4a: /* 0xc04a */
		case 0x4b: /* 0xc04b */
		case 0x4c: /* 0xc04c */
		case 0x4d: /* 0xc04d */
		case 0x4e: /* 0xc04e */
		case 0x4f: /* 0xc04f */
			UNIMPL_READ;

		/* 0xc050 - 0xc05f */
		case 0x50: /* 0xc050 */
			if(g_cur_a2_stat & ALL_STAT_TEXT) {
				g_cur_a2_stat &= (~ALL_STAT_TEXT);
				change_display_mode(dcycs);
			}
			return 0;
		case 0x51: /* 0xc051 */
			if((g_cur_a2_stat & ALL_STAT_TEXT) == 0) {
				g_cur_a2_stat |= (ALL_STAT_TEXT);
				change_display_mode(dcycs);
			}
			return 0;
		case 0x52: /* 0xc052 */
			if(g_cur_a2_stat & ALL_STAT_MIX_T_GR) {
				g_cur_a2_stat &= (~ALL_STAT_MIX_T_GR);
				change_display_mode(dcycs);
			}
			return 0;
		case 0x53: /* 0xc053 */
			if((g_cur_a2_stat & ALL_STAT_MIX_T_GR) == 0) {
				g_cur_a2_stat |= (ALL_STAT_MIX_T_GR);
				change_display_mode(dcycs);
			}
			return 0;
		case 0x54: /* 0xc054 */
			set_statereg(dcycs, statereg & (~0x40));
			return 0;
		case 0x55: /* 0xc055 */
			set_statereg(dcycs, statereg | 0x40);
			return 0;
		case 0x56: /* 0xc056 */
			if(g_cur_a2_stat & ALL_STAT_HIRES) {
				g_cur_a2_stat &= (~ALL_STAT_HIRES);
				fixup_hires_on(1);
				change_display_mode(dcycs);
			}
			return 0;
		case 0x57: /* 0xc057 */
			if((g_cur_a2_stat & ALL_STAT_HIRES) == 0) {
				g_cur_a2_stat |= (ALL_STAT_HIRES);
				fixup_hires_on(1);
				change_display_mode(dcycs);
			}
			return 0;
		case 0x58: /* 0xc058 */
			annunc_0 = 0;
			return 0;
		case 0x59: /* 0xc059 */
			annunc_0 = 1;
			return 0;
		case 0x5a: /* 0xc05a */
			annunc_1 = 0;
			return 0;
		case 0x5b: /* 0xc05b */
			annunc_1 = 1;
			return 0;
		case 0x5c: /* 0xc05c */
			annunc_2 = 0;
			return 0;
		case 0x5d: /* 0xc05d */
			annunc_2 = 1;
			return 0;
		case 0x5e: /* 0xc05e */
			if(g_cur_a2_stat & ALL_STAT_ANNUNC3) {
				g_cur_a2_stat &= (~ALL_STAT_ANNUNC3);
				change_display_mode(dcycs);
			}
			return 0;
		case 0x5f: /* 0xc05f */
			if((g_cur_a2_stat & ALL_STAT_ANNUNC3) == 0) {
				g_cur_a2_stat |= (ALL_STAT_ANNUNC3);
				change_display_mode(dcycs);
			}
			return 0;


		/* 0xc060 - 0xc06f */
		case 0x60: /* 0xc060 */
			return IOR(0);
		case 0x61: /* 0xc061 */
			return IOR(adb_is_cmd_key_down());
		case 0x62: /* 0xc062 */
			return IOR(adb_is_option_key_down());
		case 0x63: /* 0xc063 */
			return IOR(0);
		case 0x64: /* 0xc064 */
			return read_paddles(0, dcycs);
		case 0x65: /* 0xc065 */
			return read_paddles(1, dcycs);
		case 0x66: /* 0xc066 */
			return read_paddles(2, dcycs);
		case 0x67: /* 0xc067 */
			return read_paddles(3, dcycs);
		case 0x68: /* 0xc068 = STATEREG */
			return statereg;
		case 0x69: /* 0xc069 */
			/* Reserved reg, return 0 */
			return 0;
		case 0x6a: /* 0xc06a */
		case 0x6b: /* 0xc06b */
		case 0x6c: /* 0xc06c */
		case 0x6d: /* 0xc06d */
		case 0x6e: /* 0xc06e */
		case 0x6f: /* 0xc06f */
			UNIMPL_READ;

		/* 0xc070 - 0xc07f */
		case 0x70: /* c070 */
			g_paddle_trig_dcycs = dcycs;
			return 0;
		case 0x71:	/* 0xc071 */
			return 0xe2;
		case 0x72:
			return 0x40;
		case 0x73:
			return 0x50;
		case 0x74:
			return 0xb8;
		case 0x75:
			return 0x5c;
		case 0x76:
			return 0x10;
		case 0x77:
			return 0x00;
		case 0x78:
			return 0xe1;
		case 0x79:
			return 0x38;
		case 0x7a:
			return 0x90;
		case 0x7b:
			return 0x18;
		case 0x7c:
			return 0x5c;
		case 0x7d:
			return 0xb4;
		case 0x7e:
			return 0xb7;
		case 0x7f:
			return 0xff;

		/* 0xc080 - 0xc08f */
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b:
		case 0x8c: case 0x8d: case 0x8e: case 0x8f:
			new_lcbank2 = ((loc & 0x8) >> 1) ^ 0x4;
			new_wrdefram = (loc & 1);
			if(new_wrdefram != wrdefram) {
				fixup_wrdefram(new_wrdefram, 1);
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
				set_statereg(dcycs, (statereg & ~(0x04)) |
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
				set_statereg(dcycs, (statereg & ~(0x0c)) |
					(new_lcbank2));
				break;
			}
			return 0xa0;
		/* 0xc090 - 0xc09f */
		case 0x90: case 0x91: case 0x92: case 0x93:
		case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9a: case 0x9b:
		case 0x9c: case 0x9d: case 0x9e: case 0x9f:
			UNIMPL_READ;

		/* 0xc0a0 - 0xc0af */
		case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf:
			UNIMPL_READ;

		/* 0xc0b0 - 0xc0bf */
		case 0xb0:
			/* c0b0: female voice tool033 look at this */
			return 0;
		case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			UNIMPL_READ;
		/* c0b8: Second Sight card stuff: return 0 */
		case 0xb8:
			return 0;
			break;

		/* 0xc0c0 - 0xc0cf */
		case 0xc0: case 0xc1: case 0xc2: case 0xc3:
		case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb:
		case 0xcc: case 0xcd: case 0xce: case 0xcf:
			UNIMPL_READ;
		/* 0xc0d0 - 0xc0df */
		case 0xd0: case 0xd1: case 0xd2: case 0xd3:
		case 0xd4: case 0xd5: case 0xd6: case 0xd7:
		case 0xd8: case 0xd9: case 0xda: case 0xdb:
		case 0xdc: case 0xdd: case 0xde: case 0xdf:
			UNIMPL_READ;
		/* 0xc0e0 - 0xc0ef */
		case 0xe0: case 0xe1: case 0xe2: case 0xe3:
		case 0xe4: case 0xe5: case 0xe6: case 0xe7:
		case 0xe8: case 0xe9: case 0xea: case 0xeb:
		          case 0xed: case 0xee: case 0xef:
			return read_iwm(loc, dcycs);
		case 0xec:
			return iwm_read_c0ec(dcycs);
		/* 0xc0f0 - 0xc0ff */
		case 0xf0: case 0xf1: case 0xf2: case 0xf3:
		case 0xf4: case 0xf5: case 0xf6: case 0xf7:
		case 0xf8: case 0xf9: case 0xfa: case 0xfb:
		case 0xfc: case 0xfd: case 0xfe: case 0xff:
			UNIMPL_READ;

		default:
			printf("loc: %04x bad\n", loc);
			UNIMPL_READ;
		}
	case 1: case 2: case 3: case 4: case 5: case 6:
		/* c100 - c6ff */
		slot = ((loc >> 8) & 7);
		if(INTCX || (int_crom[slot] == 0)) {
			return(g_rom_fc_ff_ptr[0x3c000 + (loc & 0xfff)]);
		}
		return (dummy++) & 0xff;
		UNIMPL_READ;
	case 7:
		/* c700 */
		if(INTCX || (int_crom[7] == 0)) {
			return(g_rom_fc_ff_ptr[0x3c000 + (loc & 0xfff)]);
		}
		return g_rom_fc_ff_ptr[0x3c500 + (loc & 0xff)];
	case 8: case 9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe:
		if(INTCX || (int_crom[3] == 0)) {
			return(g_rom_fc_ff_ptr[0x3c000 + (loc & 0xfff)]);
		}
		UNIMPL_READ;
	case 0xf:
		if(INTCX || (int_crom[3] == 0)) {
			return(g_rom_fc_ff_ptr[0x3c000 + (loc & 0xfff)]);
		}
		UNIMPL_READ;
	}

	printf("io_read: hit end, loc: %06x\n", loc);
	set_halt(1);

	return 0xff;
}

void
io_write(word32 loc, int val, Cyc *cyc_ptr)
{
	double	dcycs;
#if 0
	float	fcyc, new_fcyc;
#endif
	int	new_tmp;
	int	new_lcbank2;
	int	new_wrdefram;
	int	i;
	int	tmp, tmp2;
	int	fixup;

	CALC_DCYCS_FROM_CYC_PTR(dcycs, cyc_ptr, fcyc, new_fcyc);

	val = val & 0xff;
	switch((loc >> 8) & 0xf) {
	case 0: /* 0xc000 - 0xc0ff */
		switch(loc & 0xff) {
		/* 0xc000 - 0xc00f */
		case 0x00: /* 0xc000 */
			if(g_cur_a2_stat & ALL_STAT_ST80) {
				g_cur_a2_stat &= (~ALL_STAT_ST80);
				fixup_st80col(dcycs, 1);
			}
			return;
		case 0x01: /* 0xc001 */
			if((g_cur_a2_stat & ALL_STAT_ST80) == 0) {
				g_cur_a2_stat |= (ALL_STAT_ST80);
				fixup_st80col(dcycs, 1);
			}
			return;
		case 0x02: /* 0xc002 */
			set_statereg(dcycs, statereg & ~0x20);
			return;
		case 0x03: /* 0xc003 */
			set_statereg(dcycs, statereg | 0x20);
			return;
		case 0x04: /* 0xc004 */
			set_statereg(dcycs, statereg & ~0x10);
			return;
		case 0x05: /* 0xc005 */
			set_statereg(dcycs, statereg | 0x10);
			return;
		case 0x06: /* 0xc006 */
			set_statereg(dcycs, statereg & ~0x01);
			return;
		case 0x07: /* 0xc007 */
			set_statereg(dcycs, statereg | 0x01);
			return;
		case 0x08: /* 0xc008 */
			set_statereg(dcycs, statereg & ~0x80);
			return;
		case 0x09: /* 0xc009 */
			set_statereg(dcycs, statereg | 0x80);
			return;
		case 0x0a: /* 0xc00a */
			if(int_crom[3] != 0) {
				int_crom[3] = 0;
				fixup_intcx(1);
			}
			return;
		case 0x0b: /* 0xc00b */
			if(int_crom[3] == 0) {
				int_crom[3] = 1;
				fixup_intcx(1);
			}
			return;
		case 0x0c: /* 0xc00c */
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
				printf("c023 write of %02x!!!\n",val);
				set_halt(1);
			}
			c023_1sec_en = ((val & 0x4) != 0);
			c023_scan_en = ((val & 2) != 0);
			if(c023_scan_en && c023_scan_int &&
					!c023_scan_int_irq_pending) {
				c023_scan_int_irq_pending = 1;
				add_irq();
			}
			if(!c023_scan_en && c023_scan_int_irq_pending) {
				c023_scan_int_irq_pending = 0;
				remove_irq();
			}
			if(c023_1sec_en && c023_1sec_int &&
					!c023_1sec_int_irq_pending) {
				c023_1sec_int_irq_pending = 1;
				add_irq();
			}
			if(!c023_1sec_en && c023_1sec_int_irq_pending) {
				c023_1sec_int_irq_pending = 0;
				remove_irq();
			}

			if(c023_1sec_int_irq_pending ||
					c023_scan_int_irq_pending) {
				c023_vgc_int = 1;
			} else {
				c023_vgc_int = 0;
			}
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
			bank1latch = val & 1;
			linear_vid = (val >> 6) & 1;
			new_tmp = val & 0xa0;
			if(bank1latch == 0) {
				printf("c029: %02x\n", val);
				set_halt(1);
			}
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
			set_halt(halt_on_c02a);
			return;
		case 0x2b: /* 0xc02b */
			c02b_val = val;
			if(val != 0x08 && val != 0x00) {
				printf("Writing c02b with %02x\n", val);
				set_halt(1);
			}
			return;
		case 0x2d: /* 0xc02d */
			if((val & 0x9) != 0) {
				printf("Illegal c02d write: %02x!\n", val);
				set_halt(1);
			}
			fixup = 0;
			for(i = 0; i < 8; i++) {
				tmp = ((val & (1 << i)) != 0);
				if(int_crom[i] != tmp) {
					fixup = 1;
					int_crom[i] = tmp;
				}
			}
			if(fixup) {
				vid_printf("Write c02d of %02x\n", val);
				fixup_intcx(1);
			}
			return;
		case 0x25: /* 0xc025 */
		case 0x28: /* 0xc028 */
		case 0x2c: /* 0xc02c */
			UNIMPL_WRITE;
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
			tmp = ((val & 0x80) != 0);
			tmp2 = ((val & 0x40) != 0);
			head_35 = tmp;
			iwm_set_apple35_sel(tmp2);
			iwm_printf("write c031: %02x, h: %d, 35: %d\n",
				val, head_35, g_apple35_sel);
			return;
		case 0x32: /* 0xc032 */
			if((val & 0x40) == 0) {
				/* clear 1 sec int */
				if(c023_1sec_int) {
					irq_printf("Clear 1sec int\n");
					if(c023_1sec_int_irq_pending) {
						remove_irq();
					}
					c023_1sec_int = 0;
					c023_1sec_int_irq_pending = 0;
				}
			}
			if((val & 0x20) == 0) {
				/* clear scan line int */
				if(c023_scan_int) {
					irq_printf("Clear scn int1\n");
					if(c023_scan_int_irq_pending) {
						remove_irq();
					}
					c023_scan_int_irq_pending = 0;
					c023_scan_int = 0;
					check_for_new_scan_int(dcycs);
				}
			}
			if(c023_1sec_int_irq_pending ||
					c023_scan_int_irq_pending) {
				c023_vgc_int = 1;
			} else {
				c023_vgc_int = 0;
			}
			if((val & 0x9f) != 0x9f) {
				irq_printf("c032: wrote %02x!\n", val);
			}
			return;
		case 0x33: /* 0xc033 = CLOCKDATA*/
			clock_write_c033(val);
			return;
		case 0x34: /* 0xc034 = CLOCKCTL */
			clock_write_c034(val);
			if((val & 0xf) != g_border_color) {
				g_border_color = val & 0xf;
				change_border_color(dcycs, val & 0xf);
			}
			return;
		case 0x35: /* 0xc035 */
			update_shadow_reg(val);
			return;
		case 0x36: /* 0xc036 = CYAREG */
			tmp = (val>>7) & 1;
			if(speed_fast != tmp) {
				speed_changed++;

				/* to recalculate times */
				set_halt(HALT_EVENT);
			}
			speed_fast = tmp;
			if((val & 0xf) != g_slot_motor_detect) {
				set_halt(HALT_EVENT);
			}
			g_slot_motor_detect = (val & 0xf);

			power_on_clear = (val >> 6) & 1;
			if((val & 0x70) != 0) {
				printf("c036: %2x\n", val);
				set_halt(1);
			}
			return;
		case 0x37: /* 0xc037 */
			if(val != 0) {
				UNIMPL_WRITE;
			}
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
			doc_write_c03e(val);
			return;
		case 0x3f: /* 0xc03f */
			doc_write_c03f(val);
			return;

		/* 0xc040 - 0xc04f */
		case 0x41: /* c041 */
			c041_en_25sec_ints = ((val & 0x10) != 0);
			c041_en_vbl_ints = ((val & 0x8) != 0);
			c041_en_switch_ints = ((val & 0x4) != 0);
			c041_en_move_ints = ((val & 0x2) != 0);
			c041_en_mouse = ((val & 0x1) != 0);
			if((val & 0xe7) != 0) {
				printf("write c041: %02x\n", val);
				set_halt(1);
			}

			if(!c041_en_vbl_ints && c046_vbl_irq_pending) {
				/* there was an interrupt, but no more*/
				remove_irq();
				c046_vbl_irq_pending = 0;
			}
			if(!c041_en_25sec_ints && c046_25sec_irq_pend) {
				/* there was an interrupt, but no more*/
				remove_irq();
				c046_25sec_irq_pend = 0;
			}
			set_halt(halt_on_c041);
			return;
		case 0x46: /* c046 */
			/* ignore writes to c046 */
			return;
		case 0x47: /* c047 */
			if(c046_vbl_irq_pending) {
				remove_irq();
				c046_vbl_irq_pending = 0;
			}
			if(c046_25sec_irq_pend) {
				remove_irq();
				c046_25sec_irq_pend = 0;
			}
			c046_vbl_int_status = 0;
			c046_25sec_int_status = 0;
			return;
		case 0x40: /* c040 */
		case 0x42: /* c042 */
		case 0x43: /* c043 */
		case 0x44: /* c044 */
		case 0x45: /* c045 */
		case 0x48: /* c048 */
		case 0x49: /* c049 */
		case 0x4a: /* c04a */
		case 0x4b: /* c04b */
		case 0x4c: /* c04c */
		case 0x4d: /* c04d */
		case 0x4e: /* c04e */
		case 0x4f: /* c04f */
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
			set_statereg(dcycs, statereg & (~0x40));
			return;
		case 0x55: /* 0xc055 */
			set_statereg(dcycs, statereg | 0x40);
			return;
		case 0x56: /* 0xc056 */
			if(g_cur_a2_stat & ALL_STAT_HIRES) {
				g_cur_a2_stat &= (~ALL_STAT_HIRES);
				fixup_hires_on(1);
				change_display_mode(dcycs);
			}
			return;
		case 0x57: /* 0xc057 */
			if((g_cur_a2_stat & ALL_STAT_HIRES) == 0) {
				g_cur_a2_stat |= (ALL_STAT_HIRES);
				fixup_hires_on(1);
				change_display_mode(dcycs);
			}
			return;
		case 0x58: /* 0xc058 */
			annunc_0 = 0;
			return;
		case 0x59: /* 0xc059 */
			annunc_0 = 1;
			return;
		case 0x5a: /* 0xc05a */
			annunc_1 = 0;
			return;
		case 0x5b: /* 0xc05b */
			annunc_1 = 1;
			return;
		case 0x5c: /* 0xc05c */
			annunc_2 = 0;
			return;
		case 0x5d: /* 0xc05d */
			annunc_2 = 1;
			return;
		case 0x5e: /* 0xc05e */
			if(g_cur_a2_stat & ALL_STAT_ANNUNC3) {
				g_cur_a2_stat &= (~ALL_STAT_ANNUNC3);
				change_display_mode(dcycs);
			}
			return;
		case 0x5f: /* 0xc05f */
			if((g_cur_a2_stat & ALL_STAT_ANNUNC3) == 0) {
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
			if(val != 0) {
				printf("Writing c069 with %02x\n",val);
				set_halt(1);
			}
			return;
		case 0x6a: /* 0xc06a */
		case 0x6b: /* 0xc06b */
		case 0x6c: /* 0xc06c */
		case 0x6d: /* 0xc06d */
		case 0x6e: /* 0xc06e */
		case 0x6f: /* 0xc06f */
			UNIMPL_WRITE;

		/* 0xc070 - 0xc07f */
		case 0x70: /* 0xc070 = Trigger paddles */
			g_paddle_trig_dcycs = dcycs;
			return;
		case 0x73: /* 0xc073 = slinky ram card bank addr? */
			return;
		case 0x71: /* 0xc071 = another slinky card enable? */
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
			UNIMPL_WRITE;

		/* 0xc080 - 0xc08f */
		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8a: case 0x8b:
		case 0x8c: case 0x8d: case 0x8e: case 0x8f:
			new_lcbank2 = ((loc & 0x8) >> 1) ^ 0x4;
			new_wrdefram = (loc & 1);
			if(new_wrdefram != wrdefram) {
				fixup_wrdefram(new_wrdefram, 1);
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
				set_statereg(dcycs, (statereg & ~(0x04)) |
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
				set_statereg(dcycs, (statereg & ~(0x0c)) |
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
		case 0xa0: case 0xa1: case 0xa2: case 0xa3:
		case 0xa4: case 0xa5: case 0xa6: case 0xa7:
		case 0xa8: case 0xa9: case 0xaa: case 0xab:
		case 0xac: case 0xad: case 0xae: case 0xaf:
			UNIMPL_WRITE;

		/* 0xc0b0 - 0xc0bf */
		case 0xb0:
			/* Second sight stuff--ignore it */
			return;
		case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			UNIMPL_WRITE;

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
			printf("WRite loc: %x\n",loc);
			exit(-300);
		}
		break;
	case 1: case 2: case 3: case 4: case 5: case 6: case 7:
		/* c1000 - c7ff */
			UNIMPL_WRITE;
	case 8: case 9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe:
			UNIMPL_WRITE;
	case 0xf:
			UNIMPL_WRITE;
	}
	printf("Huh2? Write loc: %x\n", loc);
	exit(-290);
}



#if 0
int
get_slow_mem(word32 loc, int duff_cycles)
{
	int val;

	loc = loc & 0x1ffff;
	
	if((loc &0xf000) == 0xc000) {
		return(io_read(loc &0xfff, duff_cycles));
	}
	if((loc & 0xf000) >= 0xd000) {
		if((loc & 0xf000) == 0xd000) {
			if(!LCBANK2) {
				/* Not LCBANK2 == be 0xc000 - 0xd000 */
				loc = loc - 0x1000;
			}
		}
	}

	val = g_slow_memory_ptr[loc];

	printf("get_slow_mem: %06x = %02x\n", loc, val);
	set_halt(1);

	return val;
}

int
set_slow_mem(word32 loc, int val, int duff_cycles)
{
	int	or_pos;
	word32	or_val;

	loc = loc & 0x1ffff;
	if((loc & 0xf000) == 0xc000) {
		return(io_write(loc & 0xfff, val, duff_cycles));
	}

	if((loc & 0xf000) == 0xd000) {
		if(!LCBANK2) {
			/* Not LCBANK2 == be 0xc000 - 0xd000 */
			loc = loc - 0x1000;
		}
	}

	if(g_slow_memory_ptr[loc] != val) {
		or_pos = (loc >> SHIFT_PER_CHANGE) & 0x1f;
		or_val = DEP1(1, or_pos, 0);
		if((loc >> CHANGE_SHIFT) >= SLOW_MEM_CH_SIZE || loc < 0) {
			printf("loc: %08x!!\n", loc);
			exit(11);
		}
		slow_mem_changed[(loc & 0xffff) >> CHANGE_SHIFT] |= or_val;
	}

/* doesn't shadow text/hires graphics properly! */
	g_slow_memory_ptr[loc] = val;

	return val;
}
#endif

const double dlines_in_16ms = (262.0);
const double g_drecip_lines_in_16ms = (1.0/262.0);

/* IIgs vertical line counters */
/* 0x7d - 0x7f: in vbl, top of screen? */
/* 0x80 - 0xdf: not in vbl, drawing screen */
/* 0xe0 - 0xff: in vbl, bottom of screen */

/* Note: lines are then 0-0x60 effectively, for 192 lines */
/* vertical blanking engages on line 192, even if in super hires mode */
/* (Last 8 lines in SHR are drawn with vbl_active set */

word32
get_lines_since_vbl(double dcycs)
{
	double	dcycs_since_last_vbl;
	double	dlines_since_vbl;
	double	dcyc_line_start;
	word32	lines_since_vbl;
	int	offset;

	dcycs_since_last_vbl = dcycs - g_last_vbl_dcycs;

	dlines_since_vbl = (dlines_in_16ms *
		(dcycs_since_last_vbl * g_drecip_cycles_in_16ms));
	lines_since_vbl = (int)dlines_since_vbl;
	dcyc_line_start = ((double)lines_since_vbl * g_drecip_lines_in_16ms *
			g_dcycles_in_16ms);

	offset = ((int)(dcycs_since_last_vbl - dcyc_line_start)) & 0xff;

	lines_since_vbl = (lines_since_vbl << 8) + offset;

	if(lines_since_vbl < 0x10680) {
		return lines_since_vbl;
	} else {
		set_halt(1);
		printf("lines_since_vbl: %08x!\n", lines_since_vbl);
		printf("dc_s_l_v: %f, g_drecip_cycles_in_16: %f, lines_16:%f\n",
			dcycs_since_last_vbl, g_drecip_cycles_in_16ms,
			dlines_in_16ms);
		printf("cycs_in16ms: %f\n", g_dcycles_in_16ms);
		printf("dcycs: %f, last_vbl_cycs: %f\n",
			dcycs, g_last_vbl_dcycs);
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

int
read_vid_counters(int loc, double dcycs)
{
	word32	mask;
	int	lines_since_vbl;

	loc = loc & 0xf;

	lines_since_vbl = get_lines_since_vbl(dcycs);

	lines_since_vbl += 0x10000;
	if(lines_since_vbl >= 0x20000) {
		lines_since_vbl -= (0x20000 + 0xfa00);
	}

	if(lines_since_vbl > 0x1ffff) {
		printf("lines_since_vbl: %04x, dcycs: %f, last_vbl: %f\n",
			lines_since_vbl, dcycs, g_last_vbl_dcycs);
		set_halt(1);
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

