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

const char rcsid_scc_c[] = "@(#)$Header: scc.c,v 1.5 97/04/30 18:12:21 kentd Exp $";

#include "defc.h"

extern int Verbose;

/* my scc port 0 == channel B, port 1 = channel A */

int scc_mode[2] = { 0, 0};
int scc_regnum[2] = { 0, 0};
int scc_reg[2][16];

byte scc_receive_buf[2][3];
int scc_receive_numvalid[2] = { 0, 0 };

void
show_scc_state()
{
	printf("SCC state:\n");

}

word32
scc_read_reg(int port)
{
	int	regnum;
	word32	ret;

	scc_mode[port] = 0;
	regnum = scc_regnum[port];

	switch(regnum) {
	case 0:
		ret = 0x6c;
		if(scc_receive_numvalid[port]) {
			ret |= 0x01;
		}
		break;
	case 1:
		ret = 0x01;	/* all sent */
		break;
	case 10:
	case 12:
	case 13:
	case 14:
	case 15:
	case 2:
		ret = scc_reg[port][regnum];
		break;
	case 3:
		scc_regnum[port] = 0;
		ret = 0;
		return ret;
		break;
	default:
		printf("Tried reading c03%x with regnum: %d!\n", 8+port,
			regnum);
		ret = 0;
		set_halt(1);
	}

	scc_regnum[port] = 0;
	scc_printf("Read c03%x, rr%d, ret: %02x\n", 8+port, regnum, ret);

	return ret;

}

void
scc_write_reg(int port, word32 val)
{
	int	regnum;
	int	mode;

	regnum = scc_regnum[port];
	mode = scc_mode[port];

	if(mode == 0) {
		if((val & 0xf0) == 0) {
			/* Set scc_regnum */
			scc_regnum[port] = val & 0xf;
			regnum = 0;
			scc_mode[port] = 1;
		}
	} else {
		scc_regnum[port] = 0;
		scc_mode[port] = 0;
	}

	/* Set reg reg */
	switch(regnum) {
	case 0:
		/* wr0 */
		if((val & 0xf0) != 0) {
			printf("Write c03%x to wr0 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		return;
	case 1:
		/* wr1 */
		if(val != 0) {
			printf("Write c03%x to wr1 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 2:
		/* wr2 */
		/* All values do nothing, let 'em all through! */
		scc_reg[port][regnum] = val;
		return;
	case 3:
		/* wr3 */
		if((val & 0xfe) != 0xc0) {
			printf("Write c03%x to wr3 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 4:
		/* wr4 */
		if((val & 0x30) != 0x00) {
			printf("Write c03%x to wr4 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 5:
		/* wr5 */
		if((val & 0xf7) != 0x62) {
			printf("Write c03%x to wr5 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 6:
		/* wr6 */
		if(val != 0) {
			printf("Write c03%x to wr6 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 7:
		/* wr7 */
		if(val != 0) {
			printf("Write c03%x to wr7 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 9:
		/* wr9 */
		if((val & 0x35) != 0x00) {
			printf("Write c03%x to wr9 of %02x!\n", 8+port, val);
			printf("val & 0x35: %02x\n", (val & 0x35));
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 10:
		/* wr10 */
		if((val & 0xff) != 0x00) {
			printf("Write c03%x to wr10 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 11:
		/* wr11 */
		if((val & 0x7c) != 0x50) {
			printf("Write c03%x to wr11 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 12:
		/* wr12 */
		if((val & 0xff) != 0x0a) {
			scc_printf("Write c03%x to wr12: %02x!\n", 8+port, val);
		}
		scc_reg[port][regnum] = val;
		return;
	case 13:
		/* wr13 */
		if((val & 0xff) != 0x0) {
			scc_printf("Write c03%x to wr13: %02x!\n", 8+port, val);
		}
		scc_reg[port][regnum] = val;
		return;
	case 14:
		/* wr14 */
		if((val & 0xec) != 0x0) {
			printf("Write c03%x to wr14 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_reg[port][regnum] = val;
		return;
	case 15:
		/* wr15 */
		/* ignore all accesses since IIgs self test messes with it */
		if((val & 0xff) != 0x0) {
			scc_printf("Write c03%x to wr15 of %02x!\n", 8+port,
				val);
		}
		scc_reg[port][regnum] = val;
		return;
	default:
		printf("Write c03%x to wr%d of %02x!\n", 8+port, regnum, val);
		set_halt(1);
		return;
	}
}

word32
scc_read_data(int port)
{
	word32	ret;

	ret = 0;
	if(scc_receive_numvalid[port]) {
		ret = scc_receive_buf[port][0];
		scc_receive_buf[port][0] = scc_receive_buf[port][1];
		scc_receive_buf[port][1] = scc_receive_buf[port][2];
		scc_receive_buf[port][2] = 0;
		scc_receive_numvalid[port]--;
	}

	scc_printf("SCC read %04x: ret %02x\n", 0xc03a+port, ret);

	return ret;
}

void
scc_write_data(int port, word32 val)
{
	int	pos;

	scc_printf("SCC write %04x: %02x\n", 0xc03a+port, val);

	if(scc_reg[port][14] & 0x10) {
		/* local loopback! */
		pos = scc_receive_numvalid[port];
		if(pos >= 3) {
			printf("Loopback overflow port %d!\n", port);
			set_halt(1);
		} else {
			scc_receive_buf[port][pos] = val;
			scc_receive_numvalid[port] = pos + 1;
		}
	} else {
		printf("Wrote to SCC data, port %d, val: %02x\n", port, val);
		set_halt(1);
	}
}
