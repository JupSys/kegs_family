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

const char rcsid_scc_c[] = "@(#)$Header: scc.c,v 1.14 98/05/05 01:35:36 kentd Exp $";

#include "defc.h"

extern int Verbose;
extern double g_cur_dcycs;

/* my scc port 0 == channel A = slot 1 */
/*        port 1 == channel B = slot 2 */

#include "scc.h"

Scc	scc_stat[2];

void
scc_init()
{
	struct sockaddr_in sa_in;
	int	i;
	int	ret;
	int	sockfd;

	int	on;

	for(i = 0; i < 2; i++) {
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if(sockfd < 0) {
			printf("socket ret: %d, errno: %d\n", sockfd, errno);
			exit(3);
		}
		/* printf("socket ret: %d\n", sockfd); */

		on = 1;
		ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&on,
					sizeof(on));
		if(ret < 0) {
			printf("setsockopt REUSEADDR ret: %d, errno:%d\n",
				ret, errno);
			exit(3);
		}

		memset(&sa_in, 0, sizeof(sa_in));
		sa_in.sin_family = AF_INET;
		sa_in.sin_port = htons(6501 + i);
		sa_in.sin_addr.s_addr = htonl(INADDR_ANY);

		ret = bind(sockfd, (struct sockaddr *)&sa_in, sizeof(sa_in));

		if(ret < 0) {
			printf("bind ret: %d, errno: %d\n", ret, errno);
			exit(3);
		}

		ret = listen(sockfd, 1);

		on = 1;
		ret = ioctl(sockfd, FIOSNBIO, (char *)&on);
		if(ret == -1) {
			printf("ioctl ret: %d, errno: %d\n", ret, errno);
			exit(3);
		}

		scc_stat[i].accfd = sockfd;
		scc_stat[i].rdwrfd = -1;
		scc_stat[i].socket_state = 0;
	}

	scc_reset();
}

void
scc_reset()
{
	int	i;

	for(i = 0; i < 2; i++) {
		scc_stat[i].port = i;
		scc_stat[i].mode = 0;
		scc_stat[i].reg_ptr = 0;
		scc_stat[i].in_rdptr = 0;
		scc_stat[i].in_wrptr = 0;
		scc_stat[i].out_rdptr = 0;
		scc_stat[i].out_wrptr = 0;
		scc_stat[i].int_pending_rx = 0;
		scc_stat[i].int_pending_tx = 0;
	}
}

void
scc_update()
{
	/* called each VBL update */
	scc_try_to_empty_writebuf(0);
	scc_try_to_empty_writebuf(1);
	scc_try_fill_readbuf(0);
	scc_try_fill_readbuf(1);
}

void
show_scc_state()
{
	int	i, j;

	for(i = 0; i < 2; i++) {
		printf("SCC port: %d\n", i);
		for(j = 0; j < 16; j += 4) {
			printf("Reg %2d-%2d: %02x %02x %02x %02x\n", j, j+3,
				scc_stat[i].reg[j], scc_stat[i].reg[j+1],
				scc_stat[i].reg[j+2], scc_stat[i].reg[j+3]);
		}
		printf("int_pendings: rx:%d tx:%d\n",
			scc_stat[i].int_pending_rx, scc_stat[i].int_pending_tx);
	}

}

#define LEN_SCC_LOG	50
STRUCT(Scc_log) {
	int	regnum;
	word32	val;
	double	dcycs;
};

Scc_log	g_scc_log[LEN_SCC_LOG];
int	g_scc_log_pos = 0;

#define SCC_REGNUM(wr,port,reg) ((wr << 8) + (port << 4) + reg)

void
scc_log(int regnum, word32 val, double dcycs)
{
	int	pos;

	pos = g_scc_log_pos;
	g_scc_log[pos].regnum = regnum;
	g_scc_log[pos].val = val;
	g_scc_log[pos].dcycs = dcycs;
	pos++;
	if(pos >= LEN_SCC_LOG) {
		pos = 0;
	}
	g_scc_log_pos = pos;
}

void
show_scc_log(void)
{
	double	dcycs;
	int	regnum;
	int	pos;
	int	i;

	pos = g_scc_log_pos;
	dcycs = g_cur_dcycs;
	printf("SCC log pos: %d, cur dcycs:%lf\n", pos, dcycs);
	for(i = 0; i < LEN_SCC_LOG; i++) {
		pos--;
		if(pos < 0) {
			pos = LEN_SCC_LOG - 1;
		}
		regnum = g_scc_log[pos].regnum;
		printf("%d:%d: port:%d wr:%d reg: %d val:%02x at t:%lf\n",
			i, pos, (regnum >> 4) & 0xf, (regnum >> 8),
			(regnum & 0xf),
			g_scc_log[pos].val,
			g_scc_log[pos].dcycs - dcycs);
	}
}

word32
scc_read_reg(int port, double dcycs)
{
	Scc	*scc_ptr;
	word32	ret;
	int	regnum;
	int	wr9;
	int	i;

	scc_ptr = &(scc_stat[port]);
	scc_ptr->mode = 0;
	regnum = scc_ptr->reg_ptr;

	/* port 0 is channel A, port 1 is channel B */
	switch(regnum) {
	case 0:
		ret = 0x6c;	// 0x44 = no dcd, no cts, 0x6c = dcd ok, cts ok
		if(scc_ptr->in_rdptr != scc_ptr->in_wrptr) {
			ret |= 0x01;
		}
		break;
	case 1:
		ret = 0x01;	/* all sent */
		break;
	case 2:
		if(port == 0) {
			ret = scc_ptr->reg[regnum];
		} else {

			printf("Read of RR2B...stopping\n");
			set_halt(1);
			ret = 0;
#if 0
			ret = scc_stat[0].reg[regnum];
			wr9 = scc_stat[0].reg[9];
			for(i = 0; i < 8; i++) {
				if(ZZZ){};
			}
			if(wr9 & 0x10) {
				/* wr9 status high */
				
			}
#endif
		}
		break;
	case 3:
		if(port == 0) {
			ret = (scc_stat[1].int_pending_tx << 1) |
				(scc_stat[1].int_pending_rx << 2) |
				(scc_stat[0].int_pending_tx << 4) |
				(scc_stat[0].int_pending_rx << 5);
		} else {
			ret = 0;
		}
		break;
	case 9:
		ret = scc_stat[0].reg[9];
		break;
	case 10:
	case 12:
	case 13:
	case 14:
	case 15:
		ret = scc_ptr->reg[regnum];
		break;
	default:
		printf("Tried reading c03%x with regnum: %d!\n", 8+port,
			regnum);
		ret = 0;
		set_halt(1);
	}

	scc_ptr->reg_ptr = 0;
	scc_printf("Read c03%x, rr%d, ret: %02x\n", 8+port, regnum, ret);
	if(regnum != 0 && regnum != 3) {
		scc_log(SCC_REGNUM(0,port,regnum), ret, dcycs);
	}

	return ret;

}

void
scc_write_reg(int port, word32 val, double dcycs)
{
	Scc	*scc_ptr;
	word32	old_val;
	int	regnum;
	int	mode;
	int	tmp1;

	scc_ptr = &(scc_stat[port]);
	regnum = scc_ptr->reg_ptr;
	mode = scc_ptr->mode;

	if(mode == 0) {
		if((val & 0xf0) == 0) {
			/* Set reg_ptr */
			scc_ptr->reg_ptr = val & 0xf;
			regnum = 0;
			scc_ptr->mode = 1;
		}
	} else {
		scc_ptr->reg_ptr = 0;
		scc_ptr->mode = 0;
	}

	if(regnum != 0) {
		scc_log(SCC_REGNUM(1,port,regnum), val, dcycs);
	}

	/* Set reg reg */
	switch(regnum) {
	case 0:
		/* wr0 */
		tmp1 = (val >> 3) & 0x7;
		switch(tmp1) {
		case 0x0:
		case 0x1:
			break;
		case 0x2:	/* reset ext/status ints */
			/* GNO uses this, just ignore for now */
			break;
		case 0x5:	/* reset tx int pending */
			scc_clr_tx_int(port);
			break;
		case 0x7:	/* reset highest pri int pending */
			if(scc_ptr->int_pending_rx) {
				scc_clr_rx_int(port);
			} else if(scc_ptr->int_pending_tx) {
				scc_clr_tx_int(port);
			}
			break;
		case 0x4:	/* enable int on next rx char */
		default:
			printf("Write c03%x to wr0 of %02x, bad cmd code:%x!\n",
				8+port, val, tmp1);
			set_halt(1);
		}
		if((val & 0xc0) != 0) {
			printf("Write c03%x to wr0 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		return;
	case 1:
		/* wr1 */
		/* proterm sets this == 0x10, which is int on all rx */
		if((val != 0) && (val != 0x10) && (val != 0x12)) {
			printf("Write c03%x to wr1 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 2:
		/* wr2 */
		/* All values do nothing, let 'em all through! */
		scc_ptr->reg[regnum] = val;
		return;
	case 3:
		/* wr3 */
		if((val & 0xde) != 0xc0) {
			printf("Write c03%x to wr3 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 4:
		/* wr4 */
		if((val & 0x30) != 0x00) {
			printf("Write c03%x to wr4 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 5:
		/* wr5 */
		if((val & 0xf5) != 0x60) {
			printf("Write c03%x to wr5 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 6:
		/* wr6 */
		if(val != 0) {
			printf("Write c03%x to wr6 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 7:
		/* wr7 */
		if(val != 0) {
			printf("Write c03%x to wr7 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 9:
		/* wr9 */
		if((val & 0xc0)) {
			printf("Scc wr9 = %02x, reset (not done)\n", val);
		}
		if((val & 0x35) != 0x00) {
			printf("Write c03%x to wr9 of %02x!\n", 8+port, val);
			printf("val & 0x35: %02x\n", (val & 0x35));
			set_halt(1);
		}
		old_val = scc_stat[0].reg[9];
		if((val ^ old_val) & 0x08) {
			/* Master int en bit changed */
			if((val & 0x08) == 0) {
				/* ints are now off */
				scc_clr_rx_int(0);
				scc_clr_rx_int(1);
				scc_clr_tx_int(0);
				scc_clr_tx_int(1);
			}
		}
		scc_stat[0].reg[regnum] = val;
		return;
	case 10:
		/* wr10 */
		if((val & 0xff) != 0x00) {
			printf("Write c03%x to wr10 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 11:
		/* wr11 */
		if((val & 0x78) != 0x50) {
			printf("Write c03%x to wr11 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 12:
		/* wr12 */
		if((val & 0xff) != 0x0a) {
			scc_printf("Write c03%x to wr12: %02x!\n", 8+port, val);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 13:
		/* wr13 */
		if((val & 0xff) != 0x0) {
			scc_printf("Write c03%x to wr13: %02x!\n", 8+port, val);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 14:
		/* wr14 */
		switch((val >> 5) & 0x7) {
		case 0x0:
			break;
		case 0x4:	/* DPLL source is BR gen */
			break;
		default:
			printf("Write c03%x to wr14 of %02x, bad dpll code!\n",
				8+port, val);
			set_halt(1);
		}
		if((val & 0x1c) != 0x0) {
			printf("Write c03%x to wr14 of %02x!\n", 8+port, val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	case 15:
		/* wr15 */
		/* ignore all accesses since IIgs self test messes with it */
		if((val & 0xff) != 0x0) {
			scc_printf("Write c03%x to wr15 of %02x!\n", 8+port,
				val);
		}
		if((scc_stat[0].reg[9] & 0x8) && (val != 0)) {
			printf("Write wr15:%02x and master int en = 1!\n",val);
			set_halt(1);
		}
		scc_ptr->reg[regnum] = val;
		return;
	default:
		printf("Write c03%x to wr%d of %02x!\n", 8+port, regnum, val);
		set_halt(1);
		return;
	}
}

void
scc_set_rx_int(int port)
{
	Scc	*scc_ptr;
	int	mie;
	int	rx_int_mode;

	scc_ptr = &(scc_stat[port]);

	rx_int_mode = (scc_ptr->reg[1] >> 3) & 0x3;
	mie = scc_stat[0].reg[9] & 0x8;			/* Master int en */

	if(mie && ((rx_int_mode == 1) || (rx_int_mode == 2))) {
		if(!scc_ptr->int_pending_rx) {
			/* printf("rx int on port %d\n", port); */
			/* set_halt(1); */
			scc_ptr->int_pending_rx = 1;
			add_irq();
		}
	}
}

void
scc_clr_rx_int(int port)
{
	Scc	*scc_ptr;

	scc_ptr = &(scc_stat[port]);

	if(scc_ptr->int_pending_rx) {
		/* scc_printf("clr rx int on port %d\n", port); */
		/* set_halt(1) */
		scc_ptr->int_pending_rx = 0;
		remove_irq();
	}
}

void
scc_set_tx_int(int port)
{
	Scc	*scc_ptr;
	int	mie;
	int	tx_int_mode;

	scc_ptr = &(scc_stat[port]);

	tx_int_mode = (scc_ptr->reg[1] & 0x2);
	mie = scc_stat[0].reg[9] & 0x8;			/* Master int en */

	if(mie && tx_int_mode) {
		if(!scc_ptr->int_pending_tx) {
			/* printf("tx int on port %d\n", port); */
			/* set_halt(1); */
			scc_ptr->int_pending_tx = 1;
			add_irq();
		}
	}
}

void
scc_clr_tx_int(int port)
{
	Scc	*scc_ptr;

	scc_ptr = &(scc_stat[port]);

	if(scc_ptr->int_pending_tx) {
		/* printf("clr tx int on port %d\n", port); */
		/* set_halt(1); */
		scc_ptr->int_pending_tx = 0;
		remove_irq();
	}
}

void
scc_add_to_readbuf(int port, word32 val)
{
	Scc	*scc_ptr;
	int	in_wrptr;
	int	in_wrptr_next;
	int	in_rdptr;

	scc_ptr = &(scc_stat[port]);

	in_wrptr = scc_ptr->in_wrptr;
	in_rdptr = scc_ptr->in_rdptr;
	in_wrptr_next = (in_wrptr + 1) & (SCC_INBUF_SIZE - 1);
	if(in_wrptr_next != in_rdptr) {
		scc_ptr->in_buf[in_wrptr] = val;
		scc_ptr->in_wrptr = in_wrptr_next;
		scc_printf("scc in port[%d] add char 0x%02x, %d,%d != %d\n",
				scc_ptr->port, val,
				in_wrptr, in_wrptr_next, in_rdptr);
	} else {
		printf("scc inbuf overflow port %d\n", scc_ptr->port);
		set_halt(1);
	}

	scc_set_rx_int(port);
}

void
scc_add_to_writebuf(int port, word32 val)
{
	Scc	*scc_ptr;
	int	out_wrptr;
	int	out_wrptr_next;
	int	out_rdptr;

	scc_ptr = &(scc_stat[port]);

	out_wrptr = scc_ptr->out_wrptr;
	out_rdptr = scc_ptr->out_rdptr;
	out_wrptr_next = (out_wrptr + 1) & (SCC_OUTBUF_SIZE - 1);
	if(out_wrptr_next != out_rdptr) {
		scc_ptr->out_buf[out_wrptr] = val;
		scc_ptr->out_wrptr = out_wrptr_next;
		scc_printf("scc wrbuf port %d had char 0x%02x added\n",
			scc_ptr->port, val);
	} else {
		printf("scc outbuf overflow port %d\n", scc_ptr->port);
		set_halt(1);
	}
}

void
scc_accept_socket(int port)
{
	Scc	*scc_ptr;
	int	rdwrfd;

	scc_ptr = &(scc_stat[port]);

	if(scc_ptr->socket_state == 0) {
		scc_ptr->client_len = sizeof(scc_ptr->client_addr);
		rdwrfd = accept(scc_ptr->accfd, &(scc_ptr->client_addr),
			&(scc_ptr->client_len));
		if(rdwrfd < 0) {
			return;
		}
		scc_ptr->rdwrfd = rdwrfd;
	}
}

void
scc_try_fill_readbuf(int port)
{
	byte	tmp_buf[256];
	Scc	*scc_ptr;
	int	rdwrfd;
	int	i;
	int	ret;

	scc_ptr = &(scc_stat[port]);

	// Accept socket if not already open
	scc_accept_socket(port);

	rdwrfd = scc_ptr->rdwrfd;
	if(rdwrfd < 0) {
		return;
	}

	// Try reading some bytes
	ret = read(rdwrfd, tmp_buf, 256);
	if(ret > 0) {
		for(i = 0; i < ret; i++) {
			if(tmp_buf[i] == 0) {
				// Skip null chars
				continue;
			}
			scc_add_to_readbuf(port, tmp_buf[i]);
		}
	}
	
}

void
scc_try_to_empty_writebuf(int port)
{
	Scc	*scc_ptr;
	int	rdptr;
	int	wrptr;
	int	rdwrfd;
	int	i;
	int	done;
	int	ret;
	int	len;

	scc_ptr = &(scc_stat[port]);

	scc_accept_socket(port);

	rdwrfd = scc_ptr->rdwrfd;
	if(rdwrfd < 0) {
		return;
	}

	// Try writing some bytes
	done = 0;
	while(!done) {
		rdptr = scc_ptr->out_rdptr;
		wrptr = scc_ptr->out_wrptr;
		if(rdptr == wrptr) {
			done = 1;
			break;
		}
		len = wrptr - rdptr;
		if(len < 0) {
			len = SCC_OUTBUF_SIZE - rdptr;
		}
		if(len > 32) {
			len = 32;
		}
		if(len <= 0) {
			done = 1;
			break;
		}
		ret = write(rdwrfd, &(scc_ptr->out_buf[rdptr]), len);
		if(ret <= 0) {
			done = 1;
			break;
		} else {
			rdptr = rdptr + ret;
			if(rdptr >= SCC_OUTBUF_SIZE) {
				rdptr = rdptr - SCC_OUTBUF_SIZE;
			}
			scc_ptr->out_rdptr = rdptr;
			scc_set_tx_int(port);
		}
	}
}

word32
scc_read_data(int port, double dcycs)
{
	Scc	*scc_ptr;
	word32	ret;
	int	in_rdptr;
	int	in_wrptr;
	int	rx_int_mode;

	scc_ptr = &(scc_stat[port]);

	scc_try_fill_readbuf(port);

	in_rdptr = scc_ptr->in_rdptr;
	in_wrptr = scc_ptr->in_wrptr;

	ret = 0;
	if(in_rdptr != in_wrptr) {
		ret = scc_ptr->in_buf[in_rdptr];
		in_rdptr = (in_rdptr + 1) & (SCC_INBUF_SIZE - 1);
		scc_ptr->in_rdptr = in_rdptr;
		rx_int_mode = (scc_ptr->reg[1] >> 3) & 0x3;
		if(rx_int_mode == 1) {
			/* int on first char--clear on reading */
			scc_clr_rx_int(port);
		}
		if(rx_int_mode == 2) {
			/* if no more chars, clear int */
			if(in_rdptr == in_wrptr) {
				scc_clr_rx_int(port);
			}
		}
	}

	scc_printf("SCC read %04x: ret %02x, %d != %d\n", 0xc03b-port, ret,
			in_rdptr, scc_ptr->in_wrptr);

	scc_log(SCC_REGNUM(0,port,8), ret, dcycs);

	return ret;
}


void
scc_write_data(int port, word32 val, double dcycs)
{
	Scc	*scc_ptr;
	int	pos;
	int	out_wrptr;

	scc_printf("SCC write %04x: %02x\n", 0xc03b-port, val);
	scc_log(SCC_REGNUM(1,port,8), val, dcycs);

	val = val & 0x7f;
	scc_ptr = &(scc_stat[port]);
	if(scc_ptr->reg[14] & 0x10) {
		/* local loopback! */
		scc_add_to_readbuf(port, val);
	} else {
		scc_add_to_writebuf(port, val);
		scc_try_to_empty_writebuf(port);
		//printf("Wrote to SCC data, port %d, val: %02x\n", port, val);
		//set_halt(1);
	}

}

