const char rcsid_debugger_c[] = "@(#)$KmKId: debugger.c,v 1.22 2020-12-11 21:06:48+00 kentd Exp $";

/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002-2020 by Kent Dickey		*/
/*									*/
/*	This code is covered by the GNU GPL v3				*/
/*	See the file COPYING.txt or https://www.gnu.org/licenses/	*/
/*	This program is provided with no warranty			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/


#include <stdio.h>
#include <stdarg.h>
#include "defc.h"

#include "disas.h"

#define LINE_SIZE		160		/* Input buffer size */
#define PRINTF_BUF_SIZE		239
#define DEBUG_ENTRY_MAX_CHARS	80

typedef void (Dbg_fn)(const char *str);

STRUCT(Debug_entry) {
	byte str_buf[DEBUG_ENTRY_MAX_CHARS];
};

char g_debug_printf_buf[PRINTF_BUF_SIZE];
char g_debug_stage_buf[PRINTF_BUF_SIZE];
int	g_debug_stage_pos = 0;

Debug_entry *g_debug_lines_ptr = 0;
int	g_debug_lines_total = 0;
int	g_debug_lines_pos = 0;
int	g_debug_lines_alloc = 0;
int	g_debug_lines_max = 1024*1024;
int	g_debug_lines_view = -1;
int	g_debug_lines_viewable_lines = 20;
int	g_debugwin_changed = 1;

extern byte *g_memory_ptr;
extern byte *g_slow_memory_ptr;
extern int g_halt_sim;
extern int g_c068_statereg;
extern word32 stop_run_at;
extern int Verbose;
extern int Halt_on;
extern int g_a2_key_to_ascii[][4];
extern Kimage g_debugwin_kimage;

extern int g_config_control_panel;

int	g_num_breakpoints = 0;
Break_point g_break_pts[MAX_BREAK_POINTS];

extern int g_irq_pending;

extern Engine_reg engine;

int g_stepping = 0;

word32	g_list_kpc;
int	g_hex_line_len = 0x10;
word32	g_a1 = 0;
word32	g_a2 = 0;
word32	g_a3 = 0;
word32	g_a4 = 0;
word32	g_a1bank = 0;
word32	g_a2bank = 0;
word32	g_a3bank = 0;
word32	g_a4bank = 0;

#define	MAX_CMD_BUFFER		229

int	g_quit_sim_now = 0;
char	g_cmd_buffer[MAX_CMD_BUFFER + 2] = { 0 };
int	g_cmd_buffer_len = 2;

#define MAX_DISAS_BUF		150
char g_disas_buffer[MAX_DISAS_BUF];

STRUCT(Dbg_longcmd) {
	const char *str;
	Dbg_fn	*fnptr;
};

Dbg_longcmd g_debug_longcmds[] = {
	{ "bp",		debug_bp },
	{ 0, 0 }
};

void
debugger_init()
{
	debugger_help();
	g_list_kpc = engine.kpc;
}

int g_dbg_new_halt = 0;

int
debugger_run_16ms()
{
	// Called when g_halt_sim is set
	if(g_dbg_new_halt) {
		g_list_kpc = engine.kpc;
		show_regs();
	}
	g_dbg_new_halt = 0;
	// printf("debugger_run_16ms: g_halt_sim:%d\n", g_halt_sim);
	return 0;
}

void
debugger_update_list_kpc()
{
	g_dbg_new_halt = 1;
}

void
debugger_key_event(int a2code, int is_up, int shift_down, int ctrl_down,
							int lock_down)
{
	int	key, pos, changed;

	pos = 1;
	if(shift_down) {
		pos = 2;
	} else if(lock_down) {
		key = g_a2_key_to_ascii[a2code][1];
		if((key >= 'a') && (key <= 'z')) {
			pos = 2;		// CAPS LOCK on
		}
	}
	if(ctrl_down) {
		pos = 3;
	}
	key = g_a2_key_to_ascii[a2code][pos];
	if((key < 0) || is_up) {
		return;
	}
	if(key >= 0x80) {
		// printf("key: %04x\n", key);
		if(key == 0x8007) {			// F7 - close debugger
			video_set_active(&g_debugwin_kimage,
						!g_debugwin_kimage.active);
			printf("Toggled debugger window to:%d\n",
						g_debugwin_kimage.active);
		}
		if((key & 0xff) == 0x74) {		// Page up keycode
			debugger_page_updown(1);
		}
		if((key & 0xff) == 0x79) {		// Page down keycode
			debugger_page_updown(-1);
		}
		return;
	}
	pos = g_cmd_buffer_len;
	changed = 0;
	if((key >= 0x20) && (key < 0x7f)) {
		// printable character, add it
		if(pos < MAX_CMD_BUFFER) {
			// printf("cmd[%d]=%c\n", pos, key);
			g_cmd_buffer[pos++] = key;
			changed = 1;
		}
	} else if((key == 0x08) || (key == 0x7f)) {
		// Left arrow or backspace
		if(pos > 2) {
			pos--;
			changed = 1;
		}
	} else if((key == 0x0d) || (key == 0x0a)) {
		//dbg_printf("Did return, pos:%d, str:%s\n", pos, g_cmd_buffer);
		do_debug_cmd(&g_cmd_buffer[2]);
		pos = 2;
		changed = 1;
	} else {
		// printf("ctrl key:%04x\n", key);
	}
	g_cmd_buffer[pos] = 0;
	g_cmd_buffer_len = pos;
	g_debug_lines_view = -1;
	g_debugwin_changed |= changed;
	// printf("g_cmd_buffer: %s\n", g_cmd_buffer);
}

void
debugger_page_updown(int isup)
{
	int	view, max;

	view = g_debug_lines_view;
	if(view < 0) {
		view = 0;
	}
	view = view + (isup*g_debug_lines_viewable_lines);
	if(view < 0) {
		view = -1;
	}
	max = g_debug_lines_pos;
	if(g_debug_lines_alloc >= g_debug_lines_max) {
		max = g_debug_lines_alloc - 4;
	}
	view = MY_MIN(view, max - g_debug_lines_viewable_lines);

	// printf("new view:%d, was:%d\n", view, g_debug_lines_view);
	if(view != g_debug_lines_view) {
		g_debug_lines_view = view;
		g_debugwin_changed++;
	}
}

void
debugger_redraw_screen(Kimage *kimage_ptr)
{
	int	line, vid_line, back, border_top, save_pos, num, lines_done;
	int	save_view;
	int	i;

	if((g_debugwin_changed == 0) || (kimage_ptr->active == 0)) {
		return;				// Nothing to do
	}

	save_pos = g_debug_lines_pos;
	save_view = g_debug_lines_view;
	// printf("DEBUGGER drawing SCREEN!\n");
	g_cmd_buffer[0] = '>';
	g_cmd_buffer[1] = ' ';
	g_cmd_buffer[g_cmd_buffer_len] = 0xa0;		// Cursor: inverse space
	g_cmd_buffer[g_cmd_buffer_len+1] = 0;
	dbg_printf("%s\n", &g_cmd_buffer[0]);
	g_cmd_buffer[g_cmd_buffer_len] = 0;
	dbg_printf("g_halt_sim:%02x\n", g_halt_sim);
	border_top = 8;

	vid_line = (((kimage_ptr->a2_height - 2*border_top) / 16) * 8) - 1;
	num = g_debug_lines_pos - save_pos;
	if(num < 0) {
		num = num + g_debug_lines_alloc;
	}
	if(num > 4) {
		// printf("num is > 4!\n");
		num = 4;
	}
	for(i = 0; i < num; i++) {
		line = debug_get_view_line(i);
		debug_draw_debug_line(kimage_ptr, line, vid_line);
		vid_line -= 8;
	}
	g_debug_lines_pos = save_pos;
	g_debug_lines_view = save_view;
	back = save_view;
	if(back < 0) {			// -1 means always show most recent
		back = 0;
	}
	lines_done = 0;
	while(vid_line >= border_top) {
		line = debug_get_view_line(back);
		debug_draw_debug_line(kimage_ptr, line, vid_line);
		back++;
		vid_line -= 8;
		lines_done++;
#if 0
		printf(" did a line, line is now: %d after str:%s\n", line,
									str);
#endif
	}
	g_debug_lines_viewable_lines = lines_done;
	g_debugwin_changed = 0;
	kimage_ptr->x_refresh_needed = 1;
	// printf("x_refresh_needed = 1, viewable_lines:%d\n", lines_done);
}

void
debug_draw_debug_line(Kimage *kimage_ptr, int line, int vid_line)
{
	int	i;

	// printf("draw debug line:%d at vid_line:%d\n", line, vid_line);
	for(i = 7; i >= 0; i--) {
		redraw_changed_string(&(g_debug_lines_ptr[line].str_buf[0]),
			vid_line, -1L, kimage_ptr->wptr + 8, 0, 0x00ffffff,
			kimage_ptr->a2_width_full, 1);
		vid_line--;
	}
}

void
debugger_help()
{
	dbg_printf("KEGS Debugger help (courtesy Fredric Devernay\n");
	dbg_printf("General command syntax: [bank]/[address][command]\n");
	dbg_printf("e.g. 'e1/0010B' to set a breakpoint at the interrupt jump "
								"pt\n");
	dbg_printf("Enter all addresses using lower-case\n");
	dbg_printf("As with the IIgs monitor, you can omit the bank number "
								"after\n");
	dbg_printf("having set it: 'e1/0010B' followed by '14B' will set\n");
	dbg_printf("breakpoints at e1/0010 and e1/0014\n");
	dbg_printf("\n");
	dbg_printf("g                       Go\n");
	dbg_printf("[bank]/[addr]g          Go from [bank]/[address]\n");
	dbg_printf("s                       Step one instruction\n");
	dbg_printf("[bank]/[addr]s          Step one instr at [bank]/[addr]\n");
	dbg_printf("[bank]/[addr]B          Set breakpoint at [bank]/[addr]\n");
	dbg_printf("B                       Show all breakpoints\n");
	dbg_printf("[bank]/[addr]D          Delete breakpoint at [bank]/"
								"[addr]\n");
	dbg_printf("[bank]/[addr1].[addr2]  View memory\n");
	dbg_printf("[bank]/[addr]L          Disassemble memory\n");

	dbg_printf("P                       Dump the trace to 'pc_log_out'\n");
	dbg_printf("Z                       Dump SCC state\n");
	dbg_printf("I                       Dump IWM state\n");
	dbg_printf("[drive].[track]I        Dump IWM state\n");
	dbg_printf("E                       Dump Ensoniq state\n");
	dbg_printf("[osc]E                  Dump oscillator [osc] state\n");
	dbg_printf("R                       Dump dtime array and events\n");
	dbg_printf("T                       Show toolbox log\n");
	dbg_printf("[bank]/[addr]T          Dump tools using ptr [bank]/"
								"[addr]\n");
	dbg_printf("                            as 'tool_set_info'\n");
	dbg_printf("[mode]V                 XOR verbose with 1=DISK, 2=IRQ,\n");
	dbg_printf("                         4=CLK,8=SHADOW,10=IWM,20=DOC,\n");
	dbg_printf("                         40=ABD,80=SCC, 100=TEST, 200="
								"VIDEO\n");
	dbg_printf("[mode]H                 XOR halt_on with 1=SCAN_INT,\n");
	dbg_printf("                         2=IRQ, 4=SHADOW_REG, 8="
							"C70D_WRITES\n");
	dbg_printf("r                       Reset\n");
	dbg_printf("[0/1]=m                 Changes m bit for l listings\n");
	dbg_printf("[0/1]=x                 Changes x bit for l listings\n");
	dbg_printf("S                       show_bankptr_bank0 & smartport "
								"errs\n");
	dbg_printf("P                       show_pmhz\n");
	dbg_printf("A                       show_a2_line_stuff show_adb_log\n");
	dbg_printf("Ctrl-e                  Dump registers\n");
	dbg_printf("[bank]/[addr1].[addr2]us[file]  Save mem area to [file]\n");
	dbg_printf("[bank]/[addr1].[addr2]ul[file]  Load mem area from "
								"[file]\n");
	dbg_printf("v                       Show video information\n");
	dbg_printf("q                       Exit Debugger (and KEGS)\n");
}

void
do_debug_cmd(const char *in_str)
{
	Dbg_fn	*fnptr;
	const char *line_ptr;
	const char *str;
	int	slot_drive, track, osc, ret_val, mode, old_mode, got_num, len;
	int	pos, c;
	int	i;

	mode = 0;
	old_mode = 0;

	dbg_printf("*%s\n", in_str);
	line_ptr = in_str;

	// See if the command is from the longcmd list
	while(*line_ptr == ' ') {
		line_ptr++;		// eat spaces
	}
	pos = 0;
	for(i = 0; i < 1000; i++) {
		// Provide a limit to avoid hang if table not terminated right
		str = g_debug_longcmds[pos].str;
		fnptr = g_debug_longcmds[pos].fnptr;
		if(!str || !fnptr) {		// End of table
			break;			// No match found
		}
		len = (int)strlen(str);
		if(strncmp(line_ptr, str, len) == 0) {
			// Ensure next char is either a space, or 0
			// Let's us avoid commands which are prefixes, or
			//  which are old Apple II monitor hex+commands
			c = line_ptr[len];
			if((c == 0) || (c == ' ')) {
				(*fnptr)(in_str + len);
				return;
			}
		}
		pos++;
	}

	// If we get here, parse an Apple II monitor like command:
	//  {address}{cmd} repeat.
	while(1) {
		ret_val = 0;
		g_a2 = 0;
		got_num = 0;
		while(1) {
			if((mode == 0) && (got_num != 0)) {
				g_a3 = g_a2;
				g_a3bank = g_a2bank;
				g_a1 = g_a2;
				g_a1bank = g_a2bank;
			}
			ret_val = *line_ptr++ & 0x7f;
			if((ret_val >= '0') && (ret_val <= '9')) {
				g_a2 = (g_a2 << 4) + ret_val - '0';
				got_num = 1;
				continue;
			}
			if((ret_val >= 'a') && (ret_val <= 'f')) {
				g_a2 = (g_a2 << 4) + ret_val - 'a' + 10;
				got_num = 1;
				continue;
			}
			if(ret_val == '/') {
				g_a2bank = g_a2;
				g_a2 = 0;
				continue;
			}
			break;
		}
		old_mode = mode;
		mode = 0;
		switch(ret_val) {
		case 'h':
			debugger_help();
			break;
		case 'R':
			show_dtime_array();
			show_all_events();
			break;
		case 'I':
			slot_drive = -1;
			track = -1;
			if(got_num) {
				if(old_mode == '.') {
					slot_drive = g_a1;
				}
				track = g_a2;
			}
			iwm_show_track(slot_drive, track);
			iwm_show_stats();
			break;
		case 'E':
			osc = -1;
			if(got_num) {
				osc = g_a2;
			}
			doc_show_ensoniq_state(osc);
			break;
		case 'T':
			if(got_num) {
				show_toolset_tables(g_a2bank, g_a2);
			} else {
				show_toolbox_log();
			}
			break;
		case 'v':
			if(got_num) {
				dis_do_compare();
			} else {
				video_show_debug_info();
			}
			break;
		case 'V':
			dbg_printf("g_irq_pending: %05x\n", g_irq_pending);
			dbg_printf("Setting Verbose ^= %04x\n", g_a1);
			Verbose ^= g_a1;
			dbg_printf("Verbose is now: %04x\n", Verbose);
			break;
		case 'H':
			dbg_printf("Setting Halt_on ^= %04x\n", g_a1);
			Halt_on ^= g_a1;
			dbg_printf("Halt_on is now: %04x\n", Halt_on);
			break;
		case 'r':
			do_reset();
			g_list_kpc = engine.kpc;
			break;
		case 'm':
			if(old_mode == '=') {
				if(!g_a1) {
					engine.psr &= ~0x20;
				} else {
					engine.psr |= 0x20;
				}
				if(engine.psr & 0x100) {
					engine.psr |= 0x30;
				}
			} else {
				dis_do_memmove();
			}
			break;
		case 'p':
			dis_do_pattern_search();
			break;
		case 'x':
			if(old_mode == '=') {
				if(!g_a1) {
					engine.psr &= ~0x10;
				} else {
					engine.psr |= 0x10;
				}
				if(engine.psr & 0x100) {
					engine.psr |= 0x30;
				}
			}
			break;
		case 'z':
			if(old_mode == '=') {
				stop_run_at = g_a1;
				dbg_printf("Calling add_event for t:%08x\n",
									g_a1);
				add_event_stop((double)g_a1);
				dbg_printf("set stop_run_at = %x\n", g_a1);
			}
			break;
		case 'l': case 'L':
			if(got_num) {
				g_list_kpc = (g_a2bank << 16) + (g_a2 & 0xffff);
			}
			do_debug_list();
			break;
		case 'Z':
			show_scc_log();
			show_scc_state();
			break;
		case 'S':
			show_bankptrs_bank0rdwr();
			smartport_error();
			break;
		case 'P':
			dbg_printf("Show pc_log!\n");
			show_pc_log();
			break;
		case 'M':
			show_pmhz();
			mockingboard_show(got_num, g_a1);
			break;
		case 'A':
			show_a2_line_stuff();
			show_adb_log();
			break;
		case 's':
			g_stepping = 1;
			if(got_num) {
				engine.kpc = (g_a2bank << 16) + (g_a2 & 0xffff);
			}
			mode = 's';
			g_list_kpc = engine.kpc;
			break;
		case 'B':
			if(got_num) {
				dbg_printf("got_num:%d, a2bank:%x, g_a2:%x\n",
						got_num, g_a2bank, g_a2);
				set_bp((g_a2bank << 16) + g_a2,
						(g_a2bank << 16) + g_a2);
			} else {
				show_bp();
			}
			break;
		case 'D':
			if(got_num) {
				dbg_printf("got_num: %d, a2bank: %x, a2: %x\n",
						got_num, g_a2bank, g_a2);
				delete_bp((g_a2bank << 16) + g_a2);
			}
			break;
		case 'g':
		case 'G':
			dbg_printf("Going..\n");
			g_stepping = 0;
			if(got_num) {
				engine.kpc = (g_a2bank << 16) + (g_a2 & 0xffff);
			}
			do_go();
			g_list_kpc = engine.kpc;
			break;
		case 'u':
			dbg_printf("Unix commands\n");
			line_ptr = do_debug_unix(line_ptr, old_mode);
			break;
		case ':': case '.':
		case '+': case '-':
		case '=': case ',':
			mode = ret_val;
			dbg_printf("Setting mode = %x\n", mode);
			break;
		case ' ': case '\t':
			if(!got_num) {
				mode = old_mode;
				break;
			}
			mode = do_blank(mode, old_mode);
			break;
		case '<':
			g_a4 = g_a2;
			g_a4bank = g_a2bank;
			break;
		case 0x05: /* ctrl-e */
		case 'Q':
		case 'q':
			show_regs();
			break;
		case 0:			// The final null char
			if(old_mode == 's') {
				mode = do_blank(mode, old_mode);
				return;
			}
			if(line_ptr == &in_str[1]) {
				g_a2 = g_a1 | (g_hex_line_len - 1);
				show_hex_mem(g_a1bank, g_a1, g_a2bank, g_a2,-1);
				g_a1 = g_a2 + 1;
			} else {
				if((got_num == 1) || (mode == 's')) {
					mode = do_blank(mode, old_mode);
				}
			}
			return;			// Get out, all done
			break;
		default:
			dbg_printf("\nUnrecognized command: %s\n", in_str);
			return;
		}
	}
}

word32
dis_get_memory_ptr(word32 addr)
{
	word32	tmp1, tmp2, tmp3;

	tmp1 = get_memory_c(addr, 0);
	tmp2 = get_memory_c(addr + 1, 0);
	tmp3 = get_memory_c(addr + 2, 0);

	return (tmp3 << 16) + (tmp2 << 8) + tmp1;
}

void
show_one_toolset(FILE *toolfile, int toolnum, word32 addr)
{
	word32	rout_addr;
	int	num_routs;
	int	i;

	num_routs = dis_get_memory_ptr(addr);
	fprintf(toolfile, "Tool 0x%02x, table: 0x%06x, num_routs:%03x\n",
		toolnum, addr, num_routs);

	for(i = 1; i < num_routs; i++) {
		rout_addr = dis_get_memory_ptr(addr + 4*i);
		fprintf(toolfile, "%06x = %02x%02x\n", rout_addr, i, toolnum);
	}
}

void
show_toolset_tables(word32 a2bank, word32 addr)
{
	FILE	*toolfile;
	word32	tool_addr;
	int	num_tools;
	int	i;

	addr = (a2bank << 16) + (addr & 0xffff);

	toolfile = fopen("tool_set_info", "w");
	if(toolfile == 0) {
		fprintf(stderr, "fopen of tool_set_info failed: %d\n", errno);
		exit(2);
	}

	num_tools = dis_get_memory_ptr(addr);
	fprintf(toolfile, "There are 0x%02x tools using ptr at %06x\n",
			num_tools, addr);

	for(i = 1; i < num_tools; i++) {
		tool_addr = dis_get_memory_ptr(addr + 4*i);
		show_one_toolset(toolfile, i, tool_addr);
	}

	fclose(toolfile);
}

word32
debug_getnum(const char **str_ptr)
{
	const char *str;
	word32	val;
	int	c, got_num;

	str = *str_ptr;
	while(*str == ' ') {
		str++;
	}
	got_num = 0;
	val = 0;
	while(1) {
		c = tolower(*str);
		//printf("got c:%02x %c val was %08x got_num:%d\n", c, c, val,
		//						got_num);
		if((c >= '0') && (c <= '9')) {
			val = (val << 4) + (c - '0');
			got_num = 1;
		} else if((c >= 'a') && (c <= 'f')) {
			val = (val << 4) + 10 + (c - 'a');
			got_num = 1;
		} else {
			break;
		}
		str++;
	}
	*str_ptr = str;
	if(got_num) {
		return val;
	}
	return (word32)-1L;
}

void
debug_bp(const char *str)
{
	word32	addr, end_addr;

	printf("In debug_bp: %s\n", str);

	addr = debug_getnum(&str);
	// printf("getnum ret:%08x\n", addr);
	if(addr == (word32)-1L) {		// No argument
		show_bp();
		return;
	}
	end_addr = addr;
	if(*str == '-') {		// Range
		str++;
		end_addr = debug_getnum(&str);
		// printf("end_addr is %08x\n", end_addr);
		if(end_addr == (word32)-1L) {
			end_addr = addr;
		}
	}
	set_bp(addr, end_addr);
}

void
set_bp(word32 addr, word32 end_addr)
{
	int	count;

	dbg_printf("About to set BP at %06x - %06x\n", addr, end_addr);
	count = g_num_breakpoints;
	if(count >= MAX_BREAK_POINTS) {
		dbg_printf("Too many (0x%02x) breakpoints set!\n", count);
		return;
	}

	g_break_pts[count].start_addr = addr;
	g_break_pts[count].end_addr = end_addr;
	g_num_breakpoints = count + 1;
	fixup_brks();
}

void
show_bp()
{
	word32	addr, end_addr;
	int i;

	dbg_printf("Showing breakpoints set\n");
	for(i = 0; i < g_num_breakpoints; i++) {
		addr = g_break_pts[i].start_addr;
		end_addr = g_break_pts[i].end_addr;
		if(end_addr != addr) {
			dbg_printf("bp:%02x: %06x-%06x\n", i, addr, end_addr);
		} else {
			dbg_printf("bp:%02x: %06x\n", i, addr);
		}
	}
}

void
delete_bp(word32 addr)
{
	int	count;
	int	hit;
	int	i;

	dbg_printf("About to delete BP at %06x\n", addr);
	count = g_num_breakpoints;

	hit = -1;
	for(i = 0; i < count; i++) {
		if((g_break_pts[i].start_addr <= addr) &&
					(g_break_pts[i].end_addr >= addr)) {
			hit = i;
			break;
		}
	}

	if(hit < 0) {
		dbg_printf("Breakpoint not found!\n");
	} else {
		dbg_printf("Deleting brkpoint #0x%02x\n", hit);
		for(i = hit+1; i < count; i++) {
			g_break_pts[i-1] = g_break_pts[i];
		}
		g_num_breakpoints = count - 1;
		setup_pageinfo();
	}

	show_bp();
}

int
do_blank(int mode, int old_mode)
{
	int	tmp;

	switch(old_mode) {
	case 's':
		tmp = g_a2;
		if(tmp == 0) {
			tmp = 1;
		}
#if 0
		for(i = 0; i < tmp; i++) {
			g_stepping = 1;
			do_step();
			if(g_halt_sim != 0) {
				break;
			}
		}
#endif
		g_list_kpc = engine.kpc;
		/* video_update_through_line(262); */
		break;
	case ':':
		set_memory_c(((g_a3bank << 16) + g_a3), g_a2, 0);
		g_a3++;
		mode = old_mode;
		break;
	case '.':
	case 0:
		xam_mem(-1);
		break;
	case ',':
		xam_mem(16);
		break;
	case '+':
		dbg_printf("%x\n", g_a1 + g_a2);
		break;
	case '-':
		dbg_printf("%x\n", g_a1 - g_a2);
		break;
	default:
		dbg_printf("Unknown mode at space: %d\n", old_mode);
		break;
	}
	return mode;
}

void
do_go()
{
	/* also called by do_step */

	g_config_control_panel = 0;
	clear_halt();
}

void
do_step()
{
	int	size_mem_imm, size_x_imm;

	return;			// This is not correct

	do_go();

	size_mem_imm = 2;
	if(engine.psr & 0x20) {
		size_mem_imm = 1;
	}
	size_x_imm = 2;
	if(engine.psr & 0x10) {
		size_x_imm = 1;
	}
	dbg_printf("%s\n",
			do_dis(engine.kpc, size_mem_imm, size_x_imm, 0, 0, 0));
}

void
xam_mem(int count)
{
	show_hex_mem(g_a1bank, g_a1, g_a2bank, g_a2, count);
	g_a1 = g_a2 + 1;
}

void
show_hex_mem(word32 startbank, word32 start, word32 endbank, word32 end,
								int count)
{
	char	ascii[MAXNUM_HEX_PER_LINE];
	word32	i;
	int	val, offset;

	if(count < 0) {
		count = 16 - (start & 0xf);
	}

	offset = 0;
	ascii[0] = 0;
	dbg_printf("Showing hex mem: bank: %x, start: %x, end: %x\n",
		startbank, start, end);
	for(i = start; i <= end; i++) {
		if( (i==start) || (count == 16) ) {
			dbg_printf("%04x:",i);
		}
		dbg_printf(" %02x", get_memory_c((startbank <<16) + i, 0));
		val = get_memory_c((startbank << 16) + i, 0) & 0x7f;
		if((val < 32) || (val >= 0x7f)) {
			val = '.';
		}
		ascii[offset++] = val;
		ascii[offset] = 0;
		count--;
		if(count <= 0) {
			dbg_printf("   %s\n", ascii);
			offset = 0;
			ascii[0] = 0;
			count = 16;
		}
	}
	if(offset > 0) {
		dbg_printf("   %s\n", ascii);
	}
}

void
do_debug_list()
{
	char	*str;
	int	size, size_mem_imm, size_x_imm;
	int	i;

	dbg_printf("%d=m %d=x %d=LCBANK\n", (engine.psr >> 5)&1,
		(engine.psr >> 4) & 1, (g_c068_statereg & 0x4) >> 2);

	size_mem_imm = 2;
	if(engine.psr & 0x20) {
		size_mem_imm = 1;
	}
	size_x_imm = 2;
	if(engine.psr & 0x10) {
		size_x_imm = 1;
	}
	for(i = 0; i < 20; i++) {
		str = do_dis(g_list_kpc, size_mem_imm, size_x_imm, 0, 0, &size);
		g_list_kpc += size;
		dbg_printf("%s\n", str);
	}
}

void
dis_do_memmove()
{
	word32	val;

	dbg_printf("Memory move from %02x/%04x.%04x to %02x/%04x\n", g_a1bank,
						g_a1, g_a2, g_a4bank, g_a4);
	while(g_a1 <= (g_a2 & 0xffff)) {
		val = get_memory_c((g_a1bank << 16) + g_a1, 0);
		set_memory_c((g_a4bank << 16) + g_a4, val, 0);
		g_a1++;
		g_a4++;
	}
	g_a1 = g_a1 & 0xffff;
	g_a4 = g_a4 & 0xffff;
}

void
dis_do_pattern_search()
{
	dbg_printf("Memory pattern search for %04x in %02x/%04x.%04x\n", g_a4,
						g_a1bank, g_a1, g_a2);
}

void
dis_do_compare()
{
	word32	val1, val2;

	dbg_printf("Memory Compare from %02x/%04x.%04x with %02x/%04x\n",
					g_a1bank, g_a1, g_a2, g_a4bank, g_a4);
	while(g_a1 <= (g_a2 & 0xffff)) {
		val1 = get_memory_c((g_a1bank << 16) + g_a1, 0);
		val2 = get_memory_c((g_a4bank << 16) + g_a4, 0);
		if(val1 != val2) {
			dbg_printf("%02x/%04x: %02x vs %02x\n", g_a1bank, g_a1,
								val1, val2);
		}
		g_a1++;
		g_a4++;
	}
	g_a1 = g_a1 & 0xffff;
	g_a4 = g_a4 & 0xffff;
}

const char *
do_debug_unix(const char *str, int old_mode)
{
	char	localbuf[LINE_SIZE+2];
	byte	*bptr;
	word32	offset, len, a1_val;
	long	ret;
	int	fd, load;
	int	i;

	load = 0;
	switch(*str++) {
	case 'l': case 'L':
		dbg_printf("Loading..");
		load = 1;
		break;
	case 's': case 'S':
		dbg_printf("Saving...");
		break;
	default:
		dbg_printf("Unknown unix command: %c\n", *(str - 1));
		if(str[-1] == 0) {
			return str - 1;
		}
		return str;
	}
	while((*str == ' ') || (*str == '\t')) {
		str++;
	}
	i = 0;
	while(i < LINE_SIZE) {
		localbuf[i++] = *str++;
		if((*str==' ') || (*str == '\t') || (*str == '\n') ||
								(*str == 0)) {
			break;
		}
	}
	localbuf[i] = 0;

	dbg_printf("About to open: %s,len: %d\n", localbuf,
						(int)strlen(localbuf));
	if(load) {
		fd = open(localbuf, O_RDONLY | O_BINARY);
	} else {
		fd = open(localbuf, O_WRONLY | O_CREAT | O_BINARY, 0x1b6);
	}
	if(fd < 0) {
		dbg_printf("Open %s failed: %d. errno:%d\n", localbuf, fd,
								errno);
		return str;
	}
	if(load) {
		offset = g_a1 & 0xffff;
		len = 0x20000 - offset;
	} else {
		if(old_mode == '.') {
			len = g_a2 - g_a1 + 1;
		} else {
			len = 0x100;
		}
	}
	a1_val = (g_a1bank << 16) | g_a1;
	bptr = &g_memory_ptr[a1_val];
	if((g_a1bank >= 0xe0) && (g_a1bank < 0xe2)) {
		bptr = &g_slow_memory_ptr[a1_val & 0x1ffff];
	}
	if(load) {
		ret = read(fd, bptr, len);
	} else {
		ret = write(fd, bptr, len);
	}
	dbg_printf("Read/write: addr %06x for %04x bytes, ret: %lx bytes\n",
		a1_val, len, ret);
	if(ret < 0) {
		dbg_printf("errno: %d\n", errno);
	}
	g_a1 = g_a1 + (int)ret;
	return str;
}

void
do_debug_load()
{
	dbg_printf("Sorry, can't load now\n");
}

char *
do_dis(word32 kpc, int accsize, int xsize, int op_provided, word32 instr,
							int *size_ptr)
{
	char	buffer[MAX_DISAS_BUF];
	char	buffer2[MAX_DISAS_BUF];
	const char *str;
	word32	val, oldkpc, dtype;
	int	args, type, opcode, signed_val;
	int	i;

	oldkpc = kpc;
	if(op_provided) {
		opcode = (instr >> 24) & 0xff;
	} else {
		opcode = (int)get_memory_c(kpc, 0) & 0xff;
	}

	kpc++;

	dtype = disas_types[opcode];
	str = disas_opcodes[opcode];
	type = dtype & 0xff;
	args = dtype >> 8;

	if(args > 3) {
		if(args == 4) {
			args = accsize;
		} else if(args == 5) {
			args = xsize;
		}
	}

	val = -1;
	switch(args) {
	case 0:
		val = 0;
		break;
	case 1:
		if(op_provided) {
			val = instr & 0xff;
		} else {
			val = get_memory_c(kpc, 0);
		}
		break;
	case 2:
		if(op_provided) {
			val = instr & 0xffff;
		} else {
			val = get_memory16_c(kpc, 0);
		}
		break;
	case 3:
		if(op_provided) {
			val = instr & 0xffffff;
		} else {
			val = get_memory24_c(kpc, 0);
		}
		break;
	default:
		fprintf(stderr, "args out of rang: %d, opcode: %08x\n",
			args, opcode);
		break;
	}
	kpc += args;

	if(!op_provided) {
		instr = (opcode << 24) | (val & 0xffffff);
	}

	switch(type) {
	case ABS:
		if(args != 2) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%04x", str, val);
		break;
	case ABSX:
		if(args != 2) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%04x,X", str, val);
		break;
	case ABSY:
		if(args != 2) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%04x,Y", str, val);
		break;
	case ABSLONG:
		if(args != 3) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%06x", str, val);
		break;
	case ABSIND:
		if(args != 2) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s ($%04x)", str, val);
		break;
	case ABSXIND:
		if(args != 2) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s ($%04x,X)", str, val);
		break;
	case IMPLY:
	case ACCUM:
		if(args != 0) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF,  "%s", str);
		break;
	case IMMED:
		if(args == 1) {
			snprintf(&buffer[0], MAX_DISAS_BUF, "%s  #$%02x", str,
									val);
		} else if(args == 2) {
			snprintf(&buffer[0], MAX_DISAS_BUF, "%s  #$%04x", str,
									val);
		} else {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		break;
	case JUST8:
	case DLOC:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%02x", str, val);
		break;
	case DLOCX:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%02x,X", str, val);
		break;
	case DLOCY:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%02x,Y", str, val);
		break;
	case LONG:
		if(args != 3) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%06x", str, val);
		break;
	case LONGX:
		if(args != 3) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%06x,X", str, val);
		break;
	case DLOCIND:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  ($%02x)", str, val);
		break;
	case DLOCINDY:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  ($%02x),Y", str, val);
		break;
	case DLOCXIND:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  ($%02x,X)", str, val);
		break;
	case DLOCBRAK:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  [$%02x]", str, val);
		break;
	case DLOCBRAKY:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  [$%02x],y", str, val);
		break;
	case DISP8:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		signed_val = (signed char)val;
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%04x", str,
						(kpc + signed_val) & 0xffff);
		break;
	case DISP8S:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%02x,S", str,
								val & 0xff);
		break;
	case DISP8SINDY:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  ($%02x,S),Y", str,
								val & 0xff);
		break;
	case DISP16:
		if(args != 2) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%04x", str,
				(word32)(kpc+(signed)(word16)(val)) & 0xffff);
		break;
	case MVPMVN:
		if(args != 2) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  $%02x,$%02x", str,
							val & 0xff, val >> 8);
		break;
	case SEPVAL:
	case REPVAL:
		if(args != 1) {
			dbg_printf("arg # mismatch for opcode %x\n", opcode);
		}
		snprintf(&buffer[0], MAX_DISAS_BUF, "%s  #$%02x", str, val);
		break;
	default:
		dbg_printf("argument type: %d unexpected\n", type);
		break;
	}

	g_disas_buffer[0] = 0;
	snprintf(&g_disas_buffer[0], MAX_DISAS_BUF, "%02x/%04x: %02x ",
		oldkpc >> 16, oldkpc & 0xffff, opcode);
	for(i = 1; i <= args; i++) {
		snprintf(&buffer2[0], MAX_DISAS_BUF, "%02x ", instr & 0xff);
		cfg_strlcat(&g_disas_buffer[0], &buffer2[0], MAX_DISAS_BUF);
		instr = instr >> 8;
	}
	for(; i < 4; i++) {
		cfg_strlcat(&g_disas_buffer[0], "   ", MAX_DISAS_BUF);
	}
	cfg_strlcat(&g_disas_buffer[0], " ", MAX_DISAS_BUF);
	cfg_strlcat(&g_disas_buffer[0], &buffer[0], MAX_DISAS_BUF);

	if(size_ptr) {
		*size_ptr = args + 1;
	}
	return (&g_disas_buffer[0]);
}

int
debug_get_view_line(int back)
{
	int	pos;

	// where back==0 means return pos - 1.
	pos = g_debug_lines_pos - 1;
	pos = pos - back;
	if(pos < 0) {
		if(g_debug_lines_alloc >= g_debug_lines_max) {
			pos += g_debug_lines_alloc;
		} else {
			return 0;			// HACK: return -1
		}
	}
	return pos;
}

int
debug_add_output_line(char *in_str, int len)
{
	Debug_entry *line_ptr;
	byte	*out_bptr;
	int	pos, alloc, view, used_len, c;
	int	i;

	// printf("debug_add_output_line %s len:%d\n", in_str, len);
	pos = g_debug_lines_pos;
	line_ptr = g_debug_lines_ptr;
	alloc = g_debug_lines_alloc;
	if(pos >= alloc) {
		if(alloc < g_debug_lines_max) {
			alloc = MY_MAX(2048, alloc*3);
			alloc = MY_MAX(alloc, pos*3);
			alloc = MY_MIN(alloc, g_debug_lines_max);
			line_ptr = realloc(line_ptr,
						alloc * sizeof(Debug_entry));
			printf("realloc.  now %p, alloc:%d\n", line_ptr, alloc);
			g_debug_lines_ptr = line_ptr;
			g_debug_lines_alloc = alloc;
			printf("Alloced debug lines to %d\n", alloc);
		} else {
			pos = 0;
		}
	}
	// Convert to A2 format chars: set high bit of each byte, 80 chars
	//  per line
	out_bptr = &(line_ptr[pos].str_buf[0]);
	used_len = 0;
	for(i = 0; i < DEBUG_ENTRY_MAX_CHARS; i++) {
		c = ' ';
		if(*in_str) {
			c = *in_str++;
			used_len++;
		}
		c = c ^ 0x80;		// Set highbit if not already set
		out_bptr[i] = c;
	}
	pos++;
	g_debug_lines_pos = pos;
	g_debug_lines_total++;		// For updating the window
	g_debugwin_changed++;
	view = g_debug_lines_view;
	if(view >= 0) {
		view++;		// view is back from pos, so to stay the same,
				//  it must be incremented when pos incs
		if((view - 50) >= g_debug_lines_max) {
			// We were viewing the oldest page, and by wrapping
			//  around we're about to wipe out this old data
			// Jump to most recent data
			view = -1;
		}
		g_debug_lines_view = view;
	}

	return used_len;
}

void
debug_add_output_string(char *in_str, int len)
{
	int	ret, tries;

	tries = 0;
	ret = 0;
	while((len > 0) || (tries == 0)) {
		// printf("DEBUG: adding str: %s, len:%d, ret:%d\n", in_str,
		//						len, ret);
		ret = debug_add_output_line(in_str, len);
		len -= ret;
		in_str += ret;
		tries++;
	}
}

void
debug_add_output_chars(char *str)
{
	int	pos, c, tab_spaces;

	pos = g_debug_stage_pos;
	tab_spaces = 0;
	while(1) {
		if(tab_spaces > 0) {
			c = ' ';
			tab_spaces--;
		} else {
			c = *str++;
			if(c == '\t') {
				tab_spaces = 7 - (pos & 7);
				c = ' ';
			}
		}
		pos = MY_MIN(pos, (PRINTF_BUF_SIZE - 1));
		if((c == '\n') || (pos >= (PRINTF_BUF_SIZE - 1))) {
			g_debug_stage_buf[pos] = 0;
			debug_add_output_string(&g_debug_stage_buf[0], pos);
			pos = 0;
			g_debug_stage_pos = 0;
			continue;
		}
		if(c == 0) {
			g_debug_stage_pos = pos;
			return;
		}
		g_debug_stage_buf[pos++] = c;
	}
}

int
dbg_printf(const char *fmt, ...)
{
	va_list args;
	int	ret;

	va_start(args, fmt);
	ret = dbg_vprintf(fmt, args);
	va_end(args);
	return ret;
}

int
dbg_vprintf(const char *fmt, va_list args)
{
	int	ret;

	ret = vsnprintf(&g_debug_printf_buf[0], PRINTF_BUF_SIZE, fmt, args);
	debug_add_output_chars(&g_debug_printf_buf[0]);
	return ret;
}

void
halt_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	dbg_vprintf(fmt, args);
	va_end(args);

	set_halt(1);
}

void
halt2_printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	dbg_vprintf(fmt, args);
	va_end(args);

	set_halt(2);
}

