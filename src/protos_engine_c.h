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
const char rcsid_protos_engine_c_h[] = "@(#)$Header: protos_engine_c.h,v 1.5 2000/09/24 00:56:10 kentd Exp $";
#endif

/* engine_c.c */
void check_breakpoints(word32 addr);
word32 get_memory8_io_stub(word32 addr, byte *stat, double *fcycs_ptr, double fplus_x_m1);
word32 get_memory16_pieces_stub(word32 addr, byte *stat, double *fcycs_ptr, Fplus *fplus_ptr);
word32 get_memory24_pieces_stub(word32 addr, byte *stat, double *fcycs_ptr, Fplus *fplus_ptr);
void set_memory8_io_stub(word32 addr, word32 val, byte *stat, double *fcycs_ptr, double fplus_x_m1);
void set_memory16_pieces_stub(word32 addr, word32 val, double *fcycs_ptr, Fplus *fplus_ptr);
void set_memory24_pieces_stub(word32 addr, word32 val, double *fcycs_ptr, Fplus *fplus_ptr);
word32 get_memory_c(word32 addr, int cycs);
word32 get_memory16_c(word32 addr, int cycs);
word32 get_memory24_c(word32 addr, int cycs);
void set_memory_c(word32 addr, word32 val, int cycs);
void set_memory16_c(word32 addr, word32 val, int cycs);
void set_memory24_c(word32 addr, word32 val, int cycs);
word32 do_adc_sbc8(word32 in1, word32 in2, word32 psr, int sub);
word32 do_adc_sbc16(word32 in1, word32 in2, word32 psr, int sub);
void fixed_memory_ptrs_init(void);
word32 get_itimer(void);
void set_halt_act(int val);
void clr_halt_act(void);
word32 get_remaining_operands(word32 addr, word32 opcode, word32 psr, Fplus *fplus_ptr);
int enter_engine(Engine_reg *engine_ptr);

