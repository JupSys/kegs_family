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
const char rcsid_scc_h[] = "@(#)$Header: scc.h,v 1.3 98/05/04 22:58:25 kentd Exp $";
#endif

#include <sys/socket.h>
#include <netinet/in.h>

/* my scc port 0 == channel A, port 1 = channel B */

#define	SCC_INBUF_SIZE		4096
#define	SCC_OUTBUF_SIZE		4096

STRUCT(Scc) {
	int	port;
	int	socket_state;
	int	accfd;
	int	rdwrfd;
	struct	sockaddr_in client_addr;
	int	client_len;

	int	mode;
	int	reg_ptr;
	int	reg[16];

	int	in_rdptr;
	int	in_wrptr;
	byte	in_buf[SCC_INBUF_SIZE];

	int	out_rdptr;
	int	out_wrptr;
	byte	out_buf[SCC_OUTBUF_SIZE];

	int	int_pending_rx;
	int	int_pending_tx;
};

