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

const char rcsid_adb_c[] = "@(#)$Header: adb.c,v 1.38 99/10/11 01:30:41 kentd Exp $";

/* adb_mode bit 3 and bit 2 (faster repeats for arrows and space/del) not done*/

#include "adb.h"

extern int Verbose;
extern word32 g_vbl_count;
extern int g_rom_version;

enum {
	ADB_IDLE = 0,
	ADB_IN_CMD,
	ADB_SENDING_DATA,
};

#define ADB_C027_MOUSE_DATA	0x80
#define ADB_C027_MOUSE_INT	0x40
#define ADB_C027_DATA_VALID	0x20
#define ADB_C027_DATA_INT	0x10
#define ADB_C027_KBD_VALID	0x08
#define ADB_C027_KBD_INT	0x04
#define ADB_C027_MOUSE_COORD	0x02
#define ADB_C027_CMD_FULL	0x01

#define ADB_C027_NEG_MASK	( ~ (				\
	 	ADB_C027_MOUSE_DATA | ADB_C027_DATA_VALID |	\
		ADB_C027_KBD_VALID | ADB_C027_MOUSE_COORD |	\
		ADB_C027_CMD_FULL))


int halt_on_all_c027 = 0;

word32	g_adb_repeat_delay = 45;
word32	g_adb_repeat_rate = 3;
word32	g_adb_repeat_info = 0x23;
word32	g_adb_char_set = 0x0;
word32	g_adb_layout_lang = 0x0;

word32	g_adb_interrupt_byte = 0;
int	g_adb_state = ADB_IDLE;

word32	g_adb_cmd = -1;
int	g_adb_cmd_len = 0;
int	g_adb_cmd_so_far = 0;
word32	g_adb_cmd_data[16];

#define MAX_ADB_DATA_PEND	16

word32	g_adb_data[MAX_ADB_DATA_PEND];
int	g_adb_data_pending = 0;

word32	g_c027_val = 0;
word32	g_c025_val = 0;

byte	adb_memory[256];

word32 g_adb_mode = 0;		/* mode set via set_modes, clear_modes */

int a2_mouse_button = 0;

int g_warp_pointer = 0;
int g_mouse_cur_x = 0;
int g_mouse_cur_y = 0;
int g_mouse_moved = 0;
int g_mouse_button = 0;

int g_mouse_delta_x = 0;
int g_mouse_delta_y = 0;
int g_mouse_overflow_x = 0;
int g_mouse_overflow_y = 0;
int g_mouse_overflow_button = 0;
int g_mouse_overflow_valid = 0;

int	g_adb_mouse_valid_data = 0;
int	g_adb_mouse_coord = 0;

int	g_adb_data_int_sent = 0;
int	g_adb_mouse_int_sent = 0;
int	g_adb_kbd_srq_sent = 0;


#define MAX_KBD_BUF		8

int	g_key_down = 0;
int	g_hard_key_down = 0;
int	g_a2code_down = 0;
int	g_kbd_read_no_update = 0;
int	g_kbd_chars_buffered = 0;
int	g_kbd_buf[MAX_KBD_BUF];
word32	g_adb_repeat_vbl = 0;

int	g_kbd_dev_addr = 2;		/* ADB physical kbd addr */
int	g_mouse_dev_addr = 3;		/* ADB physical mouse addr */

int	g_kbd_ctl_addr = 2;		/* ADB microcontroller's kbd addr */
int	g_mouse_ctl_addr = 3;		/* ADB ucontroller's mouse addr*/
			/* above are ucontroller's VIEW of where mouse/kbd */
			/*  are...if they are moved, mouse/keyboard funcs */
			/*  should stop (c025, c000, c024, etc). */

int	a2_key_to_ascii[128][4];
word32	g_virtual_key_up[4];

#define SHIFT_DOWN	( (g_c025_val & 0x01) )
#define CTRL_DOWN	( (g_c025_val & 0x02) )
#define CAPS_LOCK_DOWN	( (g_c025_val & 0x04) )
#define OPTION_DOWN	( (g_c025_val & 0x40) )
#define CMD_DOWN	( (g_c025_val & 0x80) )


#define MAX_ADB_KBD_REG3	16

int g_kbd_reg0_pos = 0;
int g_kbd_reg0_data[MAX_ADB_KBD_REG3];
int g_kbd_reg3_16bit = 0x602;			/* also set in adb_reset()! */


int	g_adb_init = 0;

void
adb_init()
{
	int	i;
	int	asc1, asc2, asc3, asc4;
	int	keycode;

	if(g_adb_init) {
		halt_printf("g_adb_init = %d!\n", g_adb_init);
	}
	g_adb_init = 1;

	for(i = 0; i < 0x80; i++) {
		a2_key_to_ascii[i][0] = -1;
		a2_key_to_ascii[i][1] = -1;
		a2_key_to_ascii[i][2] = -1;
		a2_key_to_ascii[i][3] = -1;
	}

	i = 0;
	while(a2_key_to_ascii_raw[i][0] >= 0) {
		keycode = a2_key_to_ascii_raw[i][0];
		asc1 = a2_key_to_ascii_raw[i][1];
		asc2 = a2_key_to_ascii_raw[i][2];
		asc3 = a2_key_to_ascii_raw[i][3];

		if(keycode < 0 || keycode >= 0x80) {
			printf("ADB keycode out of range: %d: %d\n",
				i, keycode);
			my_exit(1);
		}
		a2_key_to_ascii[keycode][0] = asc1;
		a2_key_to_ascii[keycode][1] = asc2;
		asc4 = asc2;
		if(asc3 == 0) {
			asc3 = asc1 & 0x1f;
			asc4 = asc3;
		} else if(asc3 > 0) {
			if(asc3 >= 0x80) {
				/* special case for null */
				asc3 = 0;
			}
			asc4 = asc3;
		} else {
			/* asc3 < 0 */
			asc3 = asc1;
		}
		a2_key_to_ascii[keycode][2] = asc3;
		a2_key_to_ascii[keycode][3] = asc4;

		i++;
	}

	g_c025_val = 0;

	for(i = 0; i < 4; i++) {
		g_virtual_key_up[i] = -1;
	}

	adb_reset();
}


void
adb_reset()
{

	g_c027_val = 0;

	g_key_down = 0;

	g_kbd_dev_addr = 2;
	g_mouse_dev_addr = 3;

	g_kbd_ctl_addr = 2;
	g_mouse_ctl_addr = 3;

	g_adb_data_int_sent = 0;
	g_adb_mouse_int_sent = 0;
	g_adb_kbd_srq_sent = 0;

	g_adb_data_pending = 0;
	g_adb_interrupt_byte = 0;
	g_adb_state = ADB_IDLE;
	g_adb_mouse_coord = 0;
	g_adb_mouse_valid_data = 0;

	g_kbd_reg0_pos = 0;
	g_kbd_reg3_16bit = 0x602;
}

#define LEN_ADB_LOG	16
STRUCT(Adb_log) {
	word32	addr;
	int	val;
	int	state;
};

Adb_log g_adb_log[LEN_ADB_LOG];
int	g_adb_log_pos = 0;

void
adb_log(word32 addr, int val)
{
	int	pos;

	pos = g_adb_log_pos;
	g_adb_log[pos].addr = addr;
	g_adb_log[pos].val = val;
	g_adb_log[pos].state = g_adb_state;
	pos++;
	if(pos >= LEN_ADB_LOG) {
		pos = 0;
	}
	g_adb_log_pos = pos;
}

void
show_adb_log(void)
{
	int	pos;
	int	i;

	pos = g_adb_log_pos;
	printf("ADB log pos: %d\n", pos);
	for(i = 0; i < LEN_ADB_LOG; i++) {
		pos--;
		if(pos < 0) {
			pos = LEN_ADB_LOG - 1;
		}
		printf("%d:%d:  addr:%04x = %02x, st:%d\n", i, pos,
			g_adb_log[pos].addr, g_adb_log[pos].val,
			g_adb_log[pos].state);
	}
	printf("kbd: dev: %x, ctl: %x; mouse: dev: %x, ctl: %x\n",
		g_kbd_dev_addr, g_kbd_ctl_addr,
		g_mouse_dev_addr, g_mouse_ctl_addr);
	printf("adb_data_int_sent: %d, adb_kbd_srq_sent: %d, mouse_int: %d\n",
		g_adb_data_int_sent, g_adb_kbd_srq_sent, g_adb_mouse_int_sent);
	printf("g_adb_state: %d, g_adb_interrupt_byte: %02x\n",
		g_adb_state, g_adb_interrupt_byte);
}

void
adb_error(void)
{
	halt_printf("Adb Error\n");

	show_adb_log();
}



void
adb_add_kbd_srq()
{
	if(g_kbd_reg3_16bit & 0x200) {
		/* generate SRQ */
		g_adb_interrupt_byte |= 0x08;;
		if(!g_adb_kbd_srq_sent) {
			g_adb_kbd_srq_sent = 1;
			add_irq();
		}
	} else {
		printf("Got keycode but no kbd SRQ!\n");
	}
}

void
adb_clear_kbd_srq()
{
	if(g_adb_kbd_srq_sent) {
		remove_irq();
		g_adb_kbd_srq_sent = 0;
	}

	/* kbd SRQ's are the only ones to handle now, so just clean it out */
	g_adb_interrupt_byte &= (~(0x08));
}

void
adb_add_data_int()
{
	if(g_c027_val & ADB_C027_DATA_INT) {
		if(!g_adb_data_int_sent) {
			g_adb_data_int_sent = 1;
			add_irq();
		}
	}
}

void
adb_add_mouse_int()
{
	if(g_c027_val & ADB_C027_MOUSE_INT) {
		if(!g_adb_mouse_int_sent) {
			/* printf("Mouse int sent\n"); */
			g_adb_mouse_int_sent = 1;
			add_irq();
		}
	}
}

void
adb_clear_data_int()
{
	if(g_adb_data_int_sent) {
		remove_irq();
		g_adb_data_int_sent = 0;
	}
}

void
adb_clear_mouse_int()
{
	if(g_adb_mouse_int_sent) {
		remove_irq();
		g_adb_mouse_int_sent = 0;
		/* printf("Mouse int clear, button: %d\n", a2_mouse_button); */
	}
}


void
adb_send_bytes(int num_bytes, word32 val0, word32 val1, word32 val2)
{
	word32	val;
	int	shift_amount;
	int	i;

	if((num_bytes >= 12) || (num_bytes >= MAX_ADB_DATA_PEND))  {
		halt_printf("adb_send_bytes: %d is too many!\n", num_bytes);
	}

	g_adb_state = ADB_SENDING_DATA;
	g_adb_data_pending = num_bytes;
	adb_add_data_int();

	for(i = 0; i < num_bytes; i++) {
		if(i < 4) {
			val = val0;
		} else if(i < 8) {
			val = val1;
		} else {
			val = val2;
		}

		shift_amount = 8*(3 - i);
		g_adb_data[i] = (val >> shift_amount) & 0xff;
		adb_printf("adb_send_bytes[%d] = %02x\n", i, g_adb_data[i]);
	}
}


void
adb_send_1byte(word32 val)
{

	if(g_adb_data_pending != 0) {
		halt_printf("g_adb_data_pending: %d\n", g_adb_data_pending);
	}

	adb_send_bytes(1, val << 24, 0, 0);
}



void
adb_response_packet(int num_bytes, word32 val)
{

	if(g_adb_data_pending != 0) {
		halt_printf("adb_response_packet, but pending: %d\n",
			g_adb_data_pending);
	}

	g_adb_state = ADB_IDLE;
	g_adb_data_pending = num_bytes;
	g_adb_data[0] = val & 0xff;
	g_adb_data[1] = (val >> 8) & 0xff;
	g_adb_data[2] = (val >> 16) & 0xff;
	g_adb_data[3] = (val >> 24) & 0xff;
	if(num_bytes) {
		g_adb_interrupt_byte |= 0x80 + num_bytes - 1;
	} else {
		g_adb_interrupt_byte |= 0x80;
	}

	adb_printf("adb_response packet: %d: %08x\n",
		num_bytes, val);

	adb_add_data_int();
}


void
adb_kbd_reg0_data(int a2code, int is_up)
{
	if(g_kbd_reg0_pos >= MAX_ADB_KBD_REG3) {
		/* too many keys, toss */
		halt_printf("Had to toss key: %02x, %d\n", a2code, is_up);
		return;
	}

	g_kbd_reg0_data[g_kbd_reg0_pos] = a2code + (is_up << 7);

	adb_printf("g_kbd_reg0_data[%d] = %02x\n", g_kbd_reg0_pos,
		g_kbd_reg0_data[g_kbd_reg0_pos]);

	g_kbd_reg0_pos++;

	adb_add_kbd_srq();
}

void
adb_kbd_talk_reg0()
{
	word32	val0, val1;
	word32	reg;
	int	num_bytes;
	int	num;
	int	i;

	num = 0;
	val0 = g_kbd_reg0_data[0];
	val1 = g_kbd_reg0_data[1];

	num_bytes = 0;
	if(g_kbd_reg0_pos > 0) {
		num_bytes = 2;
		num = 1;
		if((val0 & 0x7f) == 0x7f) {
			/* reset */
			val1 = val0;
		} else if(g_kbd_reg0_pos > 1) {
			num = 2;
			if((val1 & 0x7f) == 0x7f) {
				/* If first byte some other key, don't */
				/*  put RESET next! */
				num = 1;
				val1 = 0xff;
			}
		} else {
			val1 = 0xff;
		}
	}

	if(num) {
		for(i = num; i < g_kbd_reg0_pos; i++) {
			g_kbd_reg0_data[i-1] = g_kbd_reg0_data[i];
		}
		g_kbd_reg0_pos -= num;
	}

	reg = (val0 << 8) + val1;

	adb_printf("adb_kbd_talk0: %04x\n", reg);

	adb_response_packet(num_bytes, reg);
	if(g_kbd_reg0_pos == 0) {
		adb_clear_kbd_srq();
	}
}

void
adb_set_config(word32 val0, word32 val1, word32 val2)
{
	word32	new_mouse;
	word32	new_kbd;
	int	tmp1;

	new_mouse = val0 >> 4;
	new_kbd = val0  & 0xf;
	if(new_mouse != g_mouse_ctl_addr) {
		printf("ADB config: mouse from %x to %x!\n",
			g_mouse_ctl_addr, new_mouse);
		adb_error();
		g_mouse_ctl_addr = new_mouse;
	}
	if(new_kbd != g_kbd_ctl_addr) {
		printf("ADB config: kbd from %x to %x!\n",
			g_kbd_ctl_addr, new_kbd);
		adb_error();
		g_kbd_ctl_addr = new_kbd;
	}


	tmp1 = val2 >> 4;
	if(tmp1 == 4) {
		g_adb_repeat_delay = 0;
	} else if(tmp1 < 4) {
		g_adb_repeat_delay = (tmp1 + 1) * 15;
	} else {
		halt_printf("Bad ADB repeat delay: %02x\n", tmp1);
	}

	tmp1 = val2 & 0xf;
	switch(tmp1) {
	case 0:
		g_adb_repeat_rate = 1;
		break;
	case 1:
		g_adb_repeat_rate = 2;
		break;
	case 2:
		g_adb_repeat_rate = 3;
		break;
	case 3:
		g_adb_repeat_rate = 3;
		break;
	case 4:
		g_adb_repeat_rate = 4;
		break;
	case 5:
		g_adb_repeat_rate = 5;
		break;
	case 6:
		g_adb_repeat_rate = 7;
		break;
	case 7:
		g_adb_repeat_rate = 15;
		break;
	case 8:
		/* I don't know what this should be, ROM 03 uses it */
		g_adb_repeat_rate = 30;
		break;
	case 9:
		/* I don't know what this should be, ROM 03 uses it */
		g_adb_repeat_rate = 60;
	default:
		halt_printf("Bad repeat rate: %02x\n", tmp1);
	}

}

void
adb_set_new_mode(word32 val)
{
	if(val & 0x03) {
		printf("Disabling keyboard/mouse:%02x!\n", val);
	}

	if(val & 0xe2) {
		halt_printf("ADB set mode: %02x!\n", val);
		adb_error();
	}

	g_adb_mode = val;
}


int
adb_read_c026()
{
	word32	ret;
	int	i;

	ret = 0;
	switch(g_adb_state) {
	case ADB_IDLE:
		ret = g_adb_interrupt_byte;
		g_adb_interrupt_byte = 0;
		if(g_adb_kbd_srq_sent) {
			g_adb_interrupt_byte |= 0x08;
		}
		if(g_adb_data_pending == 0) {
			if(ret & 0x80) {
				halt_printf("read_c026: ret:%02x, pend:%d\n",
					ret, g_adb_data_pending);
			}
			adb_clear_data_int();
		}
		if(g_adb_data_pending) {
			if(g_adb_state != ADB_IN_CMD) {
				g_adb_state = ADB_SENDING_DATA;
			}
		}
		break;
	case ADB_IN_CMD:
		ret = 0;
		break;
	case ADB_SENDING_DATA:
		ret = g_adb_data[0];
		for(i = 1; i < g_adb_data_pending; i++) {
			g_adb_data[i-1] = g_adb_data[i];
		}
		g_adb_data_pending--;
		if(g_adb_data_pending <= 0) {
			g_adb_data_pending = 0;
			g_adb_state = ADB_IDLE;
			adb_clear_data_int();
		}
		break;
	default:
		halt_printf("Bad ADB state: %d!\n", g_adb_state);
		adb_clear_data_int();
		break;
	}

	adb_printf("Reading c026.  Returning %02x, st: %02x, pend: %d\n",
		ret, g_adb_state, g_adb_data_pending);

	adb_log(0xc026, ret);
	return (ret & 0xff);
}


void
adb_write_c026(int val)
{
	word32	dev;
	word32	tmp;

	adb_printf("Writing c026 with %02x\n", val);
	adb_log(0x1c026, val);


	switch(g_adb_state) {
	case ADB_IDLE:
		g_adb_cmd = val;
		g_adb_cmd_so_far = 0;
		g_adb_cmd_len = 0;

		dev = val & 0xf;
		switch(val) {
		case 0x01:	/* Abort */
			adb_printf("Performing adb abort\n");
			/* adb_abort() */
			break;
		case 0x03:	/* Flush keyboard buffer */
			adb_printf("Flushing adb keyboard buffer\n");
			/* Do nothing */
			break;
		case 0x04:	/* Set modes */
			adb_printf("ADB set modes\n");
			g_adb_state = ADB_IN_CMD;
			g_adb_cmd_len = 1;
			break;
		case 0x05:	/* Clear modes */
			adb_printf("ADB clear modes\n");
			g_adb_state = ADB_IN_CMD;
			g_adb_cmd_len = 1;
			break;
		case 0x06:	/* Set config */
			adb_printf("ADB set config\n");
			g_adb_state = ADB_IN_CMD;
			g_adb_cmd_len = 3;
			break;
		case 0x07:	/* Sync */
			adb_printf("Performing sync cmd!\n");
			g_adb_state = ADB_IN_CMD;
			if(g_rom_version == 1) {
				g_adb_cmd_len = 4;
			} else {
				g_adb_cmd_len = 8;
			}
			break;
		case 0x08:	/* Write mem */
			adb_printf("Starting write_mem cmd\n");
			g_adb_state = ADB_IN_CMD;
			g_adb_cmd_len = 2;
			break;
		case 0x09:	/* Read mem */
			adb_printf("Performing read_mem cmd!\n");
			g_adb_state = ADB_IN_CMD;
			g_adb_cmd_len = 2;
			break;
		case 0x0a:	/* Read modes byte */
			printf("Performing read_modes cmd!\n");
			/* set_halt(1); */
			adb_send_1byte(g_adb_mode);
			break;
		case 0x0b:	/* Read config bytes */
			printf("Performing read_configs cmd!\n");
			tmp = (g_mouse_ctl_addr << 20) +
				(g_kbd_ctl_addr << 16) +
				(g_adb_char_set << 12) +
				(g_adb_layout_lang << 8) +
				(g_adb_repeat_info << 0);
			tmp = (0x82 << 24) + tmp;
			adb_send_bytes(4, tmp, 0, 0);
			break;
		case 0x0d:	/* Get Version */
			adb_printf("Performing get_version cmd!\n");
			val = 0;
			if(g_rom_version == 1) {
				/* ROM 01 = revision 5 */
				val = 5;
			} else {
				/* ROM 03 checks for rev >= 6 */
				val = 6;
			}
			adb_send_1byte(val);
			break;
		case 0x0e:	/* Read avail char sets */
			adb_printf("Performing read avail char sets cmd!\n");
			adb_send_bytes(2,	/* just 2 bytes */
				0x08000000,	/* number of ch sets=0x8 */
				0, 0);
			/* set_halt(1); */
			break;
		case 0x0f:	/* Read avail kbd layouts */
			adb_printf("Performing read avail kbd layouts cmd!\n");
			adb_send_bytes(0x2,	/* number of kbd layouts=0xa */
				0x0a000000, 0, 0);
			/* set_halt(1); */
			break;
		case 0x10:	/* Reset */
			printf("ADB reset, cmd 0x10\n");
			do_reset();
			break;
		case 0x11:	/* Send ADB keycodes */
			adb_printf("Sending ADB keycodes\n");
			g_adb_state = ADB_IN_CMD;
			g_adb_cmd_len = 1;
			break;
		case 0x12:	/* ADB cmd 12: ROM 03 only! */
			if(g_rom_version >= 3) {
				g_adb_state = ADB_IN_CMD;
				g_adb_cmd_len = 2;
			} else {
				printf("ADB cmd 12, but not ROM 3!\n");
				adb_error();
			}
			break;
		case 0x13:	/* ADB cmd 13: ROM 03 only! */
			if(g_rom_version >= 3) {
				g_adb_state = ADB_IN_CMD;
				g_adb_cmd_len = 2;
			} else {
				printf("ADB cmd 13, but not ROM 3!\n");
				adb_error();
			}
			break;
		case 0x73:	/* Disable SRQ device 3: mouse */
			adb_printf("Disabling Mouse SRQ's (device 3)\n");
			/* HACK HACK...should deal with SRQs on mouse */
			break;
		case 0xb0: case 0xb1: case 0xb2: case 0xb3:
		case 0xb4: case 0xb5: case 0xb6: case 0xb7:
		case 0xb8: case 0xb9: case 0xba: case 0xbb:
		case 0xbc: case 0xbd: case 0xbe: case 0xbf:
			/* Listen dev x reg 3 */
			adb_printf("Sending data to dev %x reg 3\n", dev);
			g_adb_state = ADB_IN_CMD;
			g_adb_cmd_len = 2;
			break;
		case 0xc0: case 0xc1: case 0xc2: case 0xc3:
		case 0xc4: case 0xc5: case 0xc6: case 0xc7:
		case 0xc8: case 0xc9: case 0xca: case 0xcb:
		case 0xcc: case 0xcd: case 0xce: case 0xcf:
			/* Talk dev x reg 0 */
			adb_printf("Performing talk dev %x reg 0\n", dev);
			if(dev == g_kbd_dev_addr) {
				adb_kbd_talk_reg0();
			} else {
				printf("Unknown talk dev %x reg 0!\n", dev);
				/* send no data, on SRQ, system polls devs */
				/*  so we don't want to send anything */
				adb_error();
			}
			break;
		case 0xf0: case 0xf1: case 0xf2: case 0xf3:
		case 0xf4: case 0xf5: case 0xf6: case 0xf7:
		case 0xf8: case 0xf9: case 0xfa: case 0xfb:
		case 0xfc: case 0xfd: case 0xfe: case 0xff:
			/* Talk dev x reg 3 */
			adb_printf("Performing talk dev %x reg 3\n", dev);
			if(dev == g_kbd_dev_addr) {
				adb_response_packet(2, g_kbd_reg3_16bit);
			} else {
				printf("Performing talk dev %x reg 3!!\n", dev);
				adb_error();
			}
			break;
		default:
			printf("ADB ucontroller cmd %02x unknown!\n", val);
			adb_error();
			halt_on_all_c027 = 1;
			break;
		}
		break;
	case ADB_IN_CMD:
		adb_printf("Setting byte %d of cmd %02x to %02x\n",
			g_adb_cmd_so_far, g_adb_cmd, val);

		g_adb_cmd_data[g_adb_cmd_so_far] = val;
		g_adb_cmd_so_far++;
		if(g_adb_cmd_so_far >= g_adb_cmd_len) {
			adb_printf("Finished cmd %02x\n", g_adb_cmd);
			do_adb_cmd();
		}

		break;
	default:
		printf("adb_state: %02x is unknown!  Setting it to ADB_IDLE\n",
			g_adb_state);
		g_adb_state = ADB_IDLE;
		adb_error();
		halt_on_all_c027 = 1;
		break;
	}
	return;
}

void
do_adb_cmd()
{
	word32	dev;
	int	new_kbd;
	int	addr;
	int	val;

	dev = g_adb_cmd & 0xf;

	g_adb_state = ADB_IDLE;

	switch(g_adb_cmd) {
	case 0x04:	/* Set modes */
		adb_printf("Performing ADB set mode: OR'ing in %02x\n",
			g_adb_cmd_data[0]);

		val = g_adb_cmd_data[0] | g_adb_mode;
		adb_set_new_mode(val);

		break;
	case 0x05:	/* clear modes */
		adb_printf("Performing ADB clear mode: AND'ing in ~%02x\n",
			g_adb_cmd_data[0]);

		val = g_adb_cmd_data[0];
		val = g_adb_mode & (~val);
		adb_set_new_mode(val);
		break;
	case 0x06:	/* Set config */
		adb_printf("Set ADB config to %02x %02x %02x\n",
			g_adb_cmd_data[0], g_adb_cmd_data[1],g_adb_cmd_data[2]);

		adb_set_config(g_adb_cmd_data[0], g_adb_cmd_data[1],
			g_adb_cmd_data[2]);

		break;
	case 0x07:	/* SYNC */
		adb_printf("Performing ADB SYNC\n");
		adb_printf("data: %02x %02x %02x %02x\n",
			g_adb_cmd_data[0], g_adb_cmd_data[1], g_adb_cmd_data[2],
			g_adb_cmd_data[3]);

		adb_set_new_mode(g_adb_cmd_data[0]);
		adb_set_config(g_adb_cmd_data[1], g_adb_cmd_data[2],
				g_adb_cmd_data[3]);

		if(g_rom_version >= 3) {
			adb_printf("  and cmd12:%02x %02x cmd13:%02x %02x\n",
				g_adb_cmd_data[4], g_adb_cmd_data[5],
				g_adb_cmd_data[6], g_adb_cmd_data[7]);
		}
		break;
	case 0x08:	/* Write mem */
		addr = g_adb_cmd_data[0];
		val = g_adb_cmd_data[1];
		write_adb_ram(addr, val);
		break;
	case 0x09:	/* Read mem */
		addr = (g_adb_cmd_data[1] << 8) + g_adb_cmd_data[0];
		adb_printf("Performing mem read to addr %04x\n", addr);
		adb_send_1byte(read_adb_ram(addr));
		break;
	case 0x11:	/* Send ADB keycodes */
		val = g_adb_cmd_data[0];
		adb_printf("Performing send ADB keycodes: %02x\n", val);
		adb_virtual_key_update(val & 0x7f, val >> 7);
		break;
	case 0x12:	/* ADB cmd12 */
		adb_printf("Performing ADB cmd 12\n");
		adb_printf("data: %02x %02x\n", g_adb_cmd_data[0],
							g_adb_cmd_data[1]);
		break;
	case 0x13:	/* ADB cmd13 */
		adb_printf("Performing ADB cmd 13\n");
		adb_printf("data: %02x %02x\n", g_adb_cmd_data[0],
							g_adb_cmd_data[1]);
		break;
	case 0xb0: case 0xb1: case 0xb2: case 0xb3:
	case 0xb4: case 0xb5: case 0xb6: case 0xb7:
	case 0xb8: case 0xb9: case 0xba: case 0xbb:
	case 0xbc: case 0xbd: case 0xbe: case 0xbf:
		/* Listen dev x reg 3 */
		if(dev == g_kbd_dev_addr) {
			if(g_adb_cmd_data[1] == 0xfe) {
				/* change keyboard addr? */
				new_kbd = g_adb_cmd_data[0] & 0xf;
				if(new_kbd != dev) {
					printf("Moving kbd to dev %x!\n",
								new_kbd);
					adb_error();
				}
				g_kbd_dev_addr = new_kbd;
			} else if(g_adb_cmd_data[1] != 1) {
				/* see what new device handler id is */
				printf("KBD listen to dev %x reg 3: 1:%02x\n",
					dev, g_adb_cmd_data[1]);
				adb_error();
			}
			if(g_adb_cmd_data[0] != g_kbd_dev_addr) {
				/* see if app is trying to change addr */
				printf("KBD listen to dev %x reg 3: 0:%02x!\n",
					dev, g_adb_cmd_data[0]);
				adb_error();
			}
			g_kbd_reg3_16bit = ((g_adb_cmd_data[0] & 0xf) << 12) +
				(g_kbd_reg3_16bit & 0x0fff);
		} else if(dev == g_mouse_dev_addr) {
			if(g_adb_cmd_data[0] != dev) {
				/* see if app is trying to change mouse addr */
				printf("MOUS listen to dev %x reg3: 0:%02x!\n",
					dev, g_adb_cmd_data[0]);
				adb_error();
			}
			if(g_adb_cmd_data[1] != 1 && g_adb_cmd_data[1] != 2) {
				/* see what new device handler id is */
				printf("MOUS listen to dev %x reg 3: 1:%02x\n",
					dev, g_adb_cmd_data[1]);
				adb_error();
			}
		} else {
			printf("Listen cmd to dev %x reg3????\n", dev);
			printf("data0: %02x, data1: %02x ????\n",
				g_adb_cmd_data[0], g_adb_cmd_data[1]);
			adb_error();
		}
		break;
	default:
		printf("Doing adb_cmd %02x: UNKNOWN!\n", g_adb_cmd);
		break;
	}
}


int
adb_read_c027()
{
	word32	ret;

	if(halt_on_all_c027) {
		halt_printf("halting on all c027 reads!\n");
	}

	if(g_c027_val & (~ADB_C027_NEG_MASK)) {
		halt_printf("read_c027: g_c027_val: %02x\n", g_c027_val);
	}

	ret = (g_c027_val & ADB_C027_NEG_MASK);

	if(g_adb_mouse_valid_data) {
		ret |= ADB_C027_MOUSE_DATA;
	}

	if(g_adb_interrupt_byte != 0) {
		ret |= ADB_C027_DATA_VALID;
	} else if(g_adb_data_pending > 0) {
		if((g_adb_state != ADB_IN_CMD)) {
			ret |= ADB_C027_DATA_VALID;
		}
	}

	if(g_adb_mouse_coord) {
		ret |= ADB_C027_MOUSE_COORD;
	}

	adb_printf("Read c027: %02x, int_byte: %02x, d_pend: %d\n",
		ret, g_adb_interrupt_byte, g_adb_data_pending);
#if 0
	adb_log(0xc027, ret);
#endif
	return ret;
}

void
adb_write_c027(int val)
{
	word32	old_val;
	word32	new_int;
	word32	old_int;

	adb_printf("Writing c027 with %02x\n", val);
	adb_log(0x1c027, val);


	old_val = g_c027_val;

	g_c027_val = (val & ADB_C027_NEG_MASK);
	new_int = g_c027_val & ADB_C027_MOUSE_INT;
	old_int = old_val & ADB_C027_MOUSE_INT;
	if(!new_int && old_int) {
		adb_clear_mouse_int();
	}

	new_int = g_c027_val & ADB_C027_DATA_INT;
	old_int = old_val & ADB_C027_DATA_INT;
	if(!new_int && old_int) {
		/* ints were on, now off */
		adb_clear_data_int();
	}

	if(g_c027_val & ADB_C027_KBD_INT) {
		halt_printf("Can't support kbd interrupts!\n");
	}

	return;
}

int
read_adb_ram(word32 addr)
{
	int val;

	adb_printf("Reading adb ram addr: %02x\n", addr);

	if(addr >= 0x100) {
		if(addr >= 0x1000 && addr < 0x2000) {
			/* ROM self-test checksum */
			if(addr == 0x1400) {
				val = 0x72;
			} else if(addr == 0x1401) {
				val = 0xf7;
			} else {
				val = 0;
			}
		} else {
			printf("adb ram addr out of range: %04x!\n", addr);
			val = 0;
		}
	} else {
		val = adb_memory[addr];
		if((addr == 0xb) && (g_rom_version == 1)) {
			// read special key state byte for Out of This World
			val = (g_c025_val >> 1) & 0x43;
			val |= (g_c025_val << 2) & 0x4;
			val |= (g_c025_val >> 2) & 0x10;
		}
		if((addr == 0xc) && (g_rom_version >= 3)) {
			// read special key state byte for Out of This World
			val = g_c025_val & 0xc7;
			printf("val is %02x\n", val);
		}
	}

	adb_printf("adb_ram returning %02x\n", val);
	return val;
}

void
write_adb_ram(word32 addr, int val)
{

	adb_printf("Writing adb_ram addr: %02x: %02x\n", addr, val);

	if(addr >= 0x100) {
		printf("write adb_ram addr: %02x: %02x!\n", addr, val);
		adb_error();
	} else {
		adb_memory[addr] = val;
	}
}

int
update_mouse(int x, int y, int button_state, int button_valid)
{

	if(x < 0 && y < 0) {
		return 0;
	}

	if((x == X_A2_WINDOW_WIDTH/2) && (y == X_A2_WINDOW_HEIGHT/2) &&
							g_warp_pointer) {
		/* skip it */
		g_mouse_cur_x = x;
		g_mouse_cur_y = y;
		g_mouse_moved = 0;
	} else {
		g_mouse_moved = 1;
	}

	g_mouse_delta_x += x - g_mouse_cur_x;
	g_mouse_delta_y += y - g_mouse_cur_y;
	g_mouse_cur_x = x;
	g_mouse_cur_y = y;

	if(button_valid) {
		if(!g_mouse_overflow_valid && (button_state != g_mouse_button)){
			/* copy delta to overflow, set overflow */
			g_mouse_overflow_x = g_mouse_delta_x;
			g_mouse_overflow_y = g_mouse_delta_y;
			g_mouse_overflow_button = g_mouse_button;
			g_mouse_overflow_valid = 1;
			g_mouse_delta_x = 0;
			g_mouse_delta_y = 0;
		}

		g_mouse_button = button_state;
	}

	if(g_mouse_moved || button_valid) {
		if( (g_mouse_ctl_addr == g_mouse_dev_addr) &&
						((g_adb_mode & 0x2) == 0)) {
			g_adb_mouse_valid_data = 1;
			adb_add_mouse_int();
		}
	}

	return g_mouse_moved;
}


int
mouse_read_c024()
{
	int	delta;
	word32	ret;
	int	mouse_button;

	if(((g_adb_mode & 0x2) != 0) || (g_mouse_dev_addr != g_mouse_ctl_addr)){
		/* mouse is off, return 0, or mouse is not autopoll */
		g_adb_mouse_valid_data = 0;
		adb_clear_mouse_int();
		return 0;
	}

	if(g_adb_mouse_coord) {
		/* y coord */
		if(g_mouse_overflow_valid) {
			delta = g_mouse_overflow_y;
			if(delta > 0x3f) {
				delta = 0x3f;
			} else if(delta < (-0x3f)) {
				delta = -0x3f;
			}
			g_mouse_overflow_y -= delta;
			mouse_button = g_mouse_overflow_button;
			if(g_mouse_overflow_y == 0 && g_mouse_overflow_x == 0) {
				g_mouse_overflow_valid = 0;
			}
		} else {
			delta = g_mouse_delta_y;
			if(delta > 0x3f) {
				delta = 0x3f;
			} else if(delta < (-0x3f)) {
				delta = -0x3f;
			}
			g_mouse_delta_y -= delta;
			mouse_button = g_mouse_button;
		}
		ret = ((!mouse_button) << 7) + (delta & 0x7f);

		irq_printf("Read c024, mouse y: %02x\n", ret);
		a2_mouse_button = mouse_button;
	} else {
		/* x coord */
		if(g_mouse_overflow_valid) {
			delta = g_mouse_overflow_x;
			if(delta > 0x3f) {
				delta = 0x3f;
			} else if(delta < (-0x3f)) {
				delta = -0x3f;
			}
			g_mouse_overflow_x -= delta;
			if(g_mouse_overflow_y == 0 && g_mouse_overflow_x == 0 &&
				  (a2_mouse_button == g_mouse_overflow_button)){
				g_mouse_overflow_valid = 0;
			}
		} else {
			delta = g_mouse_delta_x;
			if(delta > 0x3f) {
				delta = 0x3f;
			} else if(delta < (-0x3f)) {
				delta = -0x3f;
			}
			g_mouse_delta_x -= delta;
		}

		ret = 0x80 | (delta & 0x7f);

		irq_printf("Read c024, mouse x: %02x\n", ret);
	}

	if(!g_mouse_overflow_valid && !g_mouse_delta_x && !g_mouse_delta_y &&
			(g_mouse_button == a2_mouse_button)) {
		g_adb_mouse_valid_data = 0;
		adb_clear_mouse_int();
	}

	g_adb_mouse_coord ^= 1;
	return ret;
}

void
adb_key_event(int a2code, int is_up)
{
	word32	special;
	int	key;
	int	hard_key;
	int	pos;
	int	ascii;

	if(is_up) {
		adb_printf("adb_key_event, key:%02x, is up, g_key_down: %02x\n",
			a2code, g_key_down);
	}

	if(a2code < 0 || a2code > 0x7f) {
		halt_printf("add_key_event: a2code: %04x!\n", a2code);
		return;
	}

	if(!is_up && a2code == 0x35) {
		/* ESC pressed, see if ctrl & cmd key down */
		if(CTRL_DOWN && CMD_DOWN) {
			/* Desk mgr int */
			printf("Desk mgr int!\n");

			g_adb_interrupt_byte |= 0x20;
			adb_add_data_int();
		}
	}

	/* convert key to ascii, if possible */
	hard_key = 0;
	if(a2_key_to_ascii[a2code][0] & 0xef00) {
		/* special key */
	} else {
		/* we have ascii */
		hard_key = 1;
	}

	pos = 0;
	ascii = a2_key_to_ascii[a2code][0];
	if(CAPS_LOCK_DOWN && (ascii >= 'a' && ascii <= 'z')) {
		pos = 1;
	} else if(SHIFT_DOWN) {
		pos = 1;
	}

	if(CTRL_DOWN) {
		pos += 2;
	}

	ascii = a2_key_to_ascii[a2code][pos];
	key = (ascii & 0x7f) + 0x80;

	special = (ascii >> 8) & 0xff;
	if(ascii < 0) {
		printf("ascii1: %d, a2code: %02x, pos: %d\n", ascii,a2code,pos);
		ascii = 0;
		special = 0;
	}


	if(!is_up) {
		if(hard_key) {
			g_kbd_buf[g_kbd_chars_buffered] = key;
			g_kbd_chars_buffered++;
			if(g_kbd_chars_buffered >= MAX_KBD_BUF) {
				g_kbd_chars_buffered = MAX_KBD_BUF - 1;
			}
			g_key_down = 1;
			g_a2code_down = a2code;

			/* first key down, set up autorepeat */
			g_adb_repeat_vbl = g_vbl_count + g_adb_repeat_delay;
			if(g_adb_repeat_delay == 0) {
				g_key_down = 0;
			}
			g_hard_key_down = 1;
		}

		g_c025_val = g_c025_val | special;
		adb_printf("new c025_or: %02x\n", g_c025_val);
	} else {
		if(hard_key && (a2code == g_a2code_down)) {
			g_hard_key_down = 0;
			/* Turn off repeat */
			g_key_down = 0;
		}

		g_c025_val = g_c025_val & (~ special);
		adb_printf("new c025_and: %02x\n", g_c025_val);
	}

	if(g_key_down) {
		g_c025_val = g_c025_val & (~0x20);
	} else {
		/* If no hard key down, set update mod latch */
		g_c025_val = g_c025_val | 0x20;
	}

}

word32
adb_read_c000()
{
	if( ((g_kbd_buf[0] & 0x80) == 0) && (g_key_down == 0)) {
		/* nothing happening, just get out */
		return g_kbd_buf[0];
	}
	if(g_kbd_buf[0] & 0x80) {
		/* got one */
		if((g_kbd_read_no_update++ > 5) && (g_kbd_chars_buffered > 1)) {
			/* read 5 times, keys pending, let's move it along */
			printf("Read %02x 3 times, tossing\n", g_kbd_buf[0]);
			adb_access_c010();
		}
	} else if(g_key_down && g_vbl_count >= g_adb_repeat_vbl) {
		/* repeat the g_key_down */
		g_c025_val |= 0x8;
		adb_key_event(g_a2code_down, 0);
		g_adb_repeat_vbl = g_vbl_count + g_adb_repeat_rate;
	}

	return g_kbd_buf[0];
}

word32
adb_access_c010()
{
	int	tmp;
	int	i;

	g_kbd_read_no_update = 0;

	tmp = g_kbd_buf[0] & 0x7f;
	g_kbd_buf[0] = tmp;

	tmp = tmp | (g_hard_key_down << 7);
	if(g_kbd_chars_buffered) {
		for(i = 1; i < g_kbd_chars_buffered; i++) {
			g_kbd_buf[i - 1] = g_kbd_buf[i];
		}
		g_kbd_chars_buffered--;
	}

	g_c025_val = g_c025_val & (~ (0x08));

	return tmp;
}

word32
adb_read_c025()
{
	return	g_c025_val;
}

int
adb_is_cmd_key_down()
{
	return	CMD_DOWN;
}

int
adb_is_option_key_down()
{
	return	OPTION_DOWN;
}

void
adb_physical_key_update(int a2code, int is_up)
{
	int	bitpos;
	word32	mask;
	int	autopoll;
	int	i;

	/* this routine called by xdriver to pass raw codes--handle */
	/*  ucontroller and ADB bus protocol issues here */
	/* if autopoll on, pass it on through to c025,c000 regs */
	/*  else only put it in kbd reg 3, and pull SRQ if needed */

	adb_printf("adb_phys_key_update: %02x, %d\n", a2code, is_up);

	autopoll = 1;
	if(g_adb_mode & 1) {
		/* autopoll is explicitly off */
		autopoll = 0;
	}
	if(g_kbd_dev_addr != g_kbd_ctl_addr) {
		/* autopoll is off because ucontroller doesn't know kbd moved */
		autopoll = 0;
	}

	adb_printf("Handle a2code: %02x, is_up: %d\n", a2code, is_up);

	if(a2code < 0 || a2code > 0x7f) {
		halt_printf("a2code: %04x!\n", a2code);
		return;
	}

	/* Only process reset requests here */
	if(is_up == 0 && a2code == 0x7f && CTRL_DOWN) {
		/* Reset pressed! */
		printf("Reset pressed since CTRL_DOWN: %d\n", CTRL_DOWN);
		do_reset();
		return;
	}

	i = (a2code >> 5) & 3;
	bitpos = a2code & 0x1f;
	mask = (1 << bitpos);

	if(is_up) {
		if(!autopoll) {
			/* no auto keys, generate SRQ! */
			adb_kbd_reg0_data(a2code, is_up);
		} else {
			adb_virtual_key_update(a2code, is_up);
		}
	} else {
		if(!autopoll) {
			/* no auto keys, generate SRQ! */
			adb_kbd_reg0_data(a2code, is_up);
		} else {
			/* was up, now down */
			adb_virtual_key_update(a2code, is_up);
		}
	}
}

void
adb_virtual_key_update(int a2code, int is_up)
{
	int	i;
	int	bitpos;
	word32	mask;

	adb_printf("Virtual handle a2code: %02x, is_up: %d\n", a2code, is_up);

	if(a2code < 0 || a2code > 0x7f) {
		halt_printf("a2code: %04x!\n", a2code);
		return;
	}

	i = (a2code >> 5) & 3;
	bitpos = a2code & 0x1f;
	mask = (1 << bitpos);

	if(is_up) {
		if(g_virtual_key_up[i] & mask) {
			/* already up, do nothing */
		} else {
			g_virtual_key_up[i] |= mask;
			adb_key_event(a2code, is_up);
		}
	} else {
		if(g_virtual_key_up[i] & mask) {
			g_virtual_key_up[i] &= (~mask);
			adb_key_event(a2code, is_up);
		}
	}
}

void
adb_kbd_repeat_off()
{
	g_key_down = 0;
}
