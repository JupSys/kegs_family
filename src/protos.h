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
const char rcsid_protos_h[] = "@(#)$Header: protos.h,v 1.138 2000/07/07 00:39:09 kentd Exp $";
#endif

/* xdriver.c */
void update_color_array(int col_num, int a2_color);
void set_border_color(int val);
void update_physical_colormap(void);
void show_xcolor_array(void);
void x_auto_repeat_on(int must);
void install_text_colormap(void);
void set_border_color(int val);
void draw_status(int line_num, const char *string1);
void xdriver_end(void);
void dev_video_init(void);
void update_colormap(int mode);
void redraw_border(void);
void check_input_events(void);
void update_status_line(int line, const char *string);
void redraw_status_lines(void);
void x_refresh_ximage(void);

/* test65.c */
void do_gen_test(int got_num, int base_seed);


/* engine.s and engine_c.c */
void fixed_memory_ptrs_init();
word32 get_itimer(void);

word32 get_memory_c(word32 addr, int cycs);
word32 get_memory16_c(word32 addr, int cycs);
word32 get_memory24_c(word32 addr, int cycs);

int get_memory_asm(word32 addr, int cycs);
int get_memory16_asm(word32 addr, int cycs);
int get_memory16_asm_fail(word32 addr, int cycs);
int get_memory_act_stub_asm(word32 addr, int cycs);
int get_memory16_act_stub_asm(word32 addr, int cycs);

void set_memory_c(word32 addr, word32 val, int cycs);
void set_memory16_c(word32 addr, word32 val, int cycs);

int enter_engine(Engine_reg *ptr);
void clr_halt_act(void);
void set_halt_act(int val);

/* END_HDR */

/* adb.c */
void adb_init(void);
void adb_reset(void);
void adb_log(word32 addr, int val);
void show_adb_log(void);
void adb_error(void);
void adb_add_kbd_srq(void);
void adb_clear_kbd_srq(void);
void adb_add_data_int(void);
void adb_add_mouse_int(void);
void adb_clear_data_int(void);
void adb_clear_mouse_int(void);
void adb_send_bytes(int num_bytes, word32 val0, word32 val1, word32 val2);
void adb_send_1byte(word32 val);
void adb_response_packet(int num_bytes, word32 val);
void adb_kbd_reg0_data(int a2code, int is_up);
void adb_kbd_talk_reg0(void);
void adb_set_config(word32 val0, word32 val1, word32 val2);
void adb_set_new_mode(word32 val);
int adb_read_c026(void);
void adb_write_c026(int val);
void do_adb_cmd(void);
int adb_read_c027(void);
void adb_write_c027(int val);
int read_adb_ram(word32 addr);
void write_adb_ram(word32 addr, int val);
int update_mouse(int x, int y, int button_state, int button_valid);
int mouse_read_c024(void);
void adb_key_event(int a2code, int is_up);
word32 adb_read_c000(void);
word32 adb_access_c010(void);
word32 adb_read_c025(void);
int adb_is_cmd_key_down(void);
int adb_is_option_key_down(void);
void adb_physical_key_update(int a2code, int is_up);
void adb_virtual_key_update(int a2code, int is_up);
void adb_kbd_repeat_off(void);

/* clock.c */
double get_dtime(void);
int micro_sleep(double dtime);
void setup_bram(void);
void update_cur_time(void);
void clock_update(void);
void clock_update_if_needed(void);
word32 clock_read_c033(void);
word32 clock_read_c034(void);
void clock_write_c033(word32 val);
void clock_write_c034(word32 val);
void do_clock_data(void);

/* compile_time.c */

/* dis.c */
int get_num(void);
void do_debug_intfc(void);
word32 dis_get_memory_ptr(word32 addr);
void show_one_toolset(FILE *toolfile, int toolnum, word32 addr);
void show_toolset_tables(word32 a2bank, word32 addr);
void do_gen_test(int got_num, int base_seed);
void set_bp(word32 addr);
void show_bp(void);
void delete_bp(word32 addr);
void do_blank(void);
void do_go(void);
void do_step(void);
void xam_mem(int count);
void show_hex_mem(int startbank, word32 start, int endbank, word32 end, int count);
int read_line(char *buf, int len);
void do_debug_list(void);
void load_roms(void);
void do_debug_unix(void);
void do_debug_load(void);
int do_dis(FILE *outfile, word32 kpc, int accsize, int xsize, int op_provided, word32 instr);
void show_line(FILE *outfile, word32 kaddr, word32 operand, int size, char *string);
void halt_printf(const char *fmt, ...);

/* scc.c */
void scc_init(void);
void scc_reset(void);
void scc_hard_reset_port(int port);
void scc_reset_port(int port);
void scc_regen_clocks(int port);
void scc_update(double dcycs);
void do_scc_event(int type, double dcycs);
void show_scc_state(void);
void scc_log(int regnum, word32 val, double dcycs);
void show_scc_log(void);
word32 scc_read_reg(int port, double dcycs);
void scc_write_reg(int port, word32 val, double dcycs);
void scc_maybe_br_event(int port, double dcycs);
void scc_evaluate_ints(int port);
void scc_maybe_rx_event(int port, double dcycs);
void scc_maybe_rx_int(int port, double dcycs);
void scc_clr_rx_int(int port);
void scc_handle_tx_event(int port, double dcycs);
void scc_maybe_tx_event(int port, double dcycs);
void scc_clr_tx_int(int port);
void scc_set_zerocnt_int(int port);
void scc_clr_zerocnt_int(int port);
void scc_add_to_readbuf(int port, word32 val, double dcycs);
void scc_add_to_writebuf(int port, word32 val, double dcycs);
word32 scc_read_data(int port, double dcycs);
void scc_write_data(int port, word32 val, double dcycs);
int scc_socket_init(int port);
void scc_accept_socket(int port);
void scc_try_fill_readbuf(int port, double dcycs);
void scc_try_to_empty_writebuf(int port);

/* iwm.c */
void iwm_init_drive(Disk *dsk, int smartport, int drive, int disk_525);
void iwm_init(void);
void iwm_reset(void);
void draw_iwm_status(int line, char *buf);
void iwm_flush_disk_to_unix(Disk *dsk);
void iwm_vbl_update(void);
void iwm_show_stats(void);
void iwm_touch_switches(int loc, double dcycs);
void iwm_move_to_track(Disk *dsk, int new_track);
void iwm525_phase_change(int drive, int phase);
int iwm_read_status35(double dcycs);
void iwm_do_action35(double dcycs);
void iwm_set_apple35_sel(int newval);
int iwm_read_c0ec(double dcycs);
int read_iwm(int loc, double dcycs);
void write_iwm(int loc, int val, double dcycs);
int iwm_read_enable2(double dcycs);
int iwm_read_enable2_handshake(double dcycs);
void iwm_write_enable2(int val, double dcycs);
int iwm_read_data(Disk *dsk, int fast_disk_emul, double dcycs);
void iwm_write_data(Disk *dsk, word32 val, int fast_disk_emul, double dcycs);
int iwm_read_data_35(Disk *dsk, int fast_disk_emul, double dcycs);
void iwm_write_data_35(Disk *dsk, word32 val, int fast_disk_emul, double dcycs);
int iwm_read_data_525(Disk *dsk, int fast_disk_emul, double dcycs);
void iwm_write_data_525(Disk *dsk, word32 val, int fast_disk_emul, double dcycs);
void sector_to_partial_nib(byte *in, byte *nib_ptr);
int disk_unnib_4x4(Disk *dsk);
int iwm_denib_track525(Disk *dsk, Track *trk, int qtr_track, byte *outbuf);
int iwm_denib_track35(Disk *dsk, Track *trk, int qtr_track, byte *outbuf);
int disk_track_to_unix(Disk *dsk, int qtr_track, byte *outbuf);
void show_hex_data(byte *buf, int count);
void disk_check_nibblization(Disk *dsk, int qtr_track, byte *buf, int size);
void disk_unix_to_nib(Disk *dsk, int qtr_track, int unix_pos, int unix_len, int nib_len);
void iwm_nibblize_track_nib525(Disk *dsk, Track *trk, byte *track_buf, int qtr_track);
void iwm_nibblize_track_525(Disk *dsk, Track *trk, byte *track_buf, int qtr_track);
void iwm_nibblize_track_35(Disk *dsk, Track *trk, byte *track_buf, int qtr_track);
void disk_4x4_nib_out(Disk *dsk, word32 val);
void disk_nib_out(Disk *dsk, byte val, int size);
void disk_nib_end_track(Disk *dsk);
void iwm_show_track(int slot_drive, int track);
void iwm_show_a_track(Track *trk);
void maybe_parse_disk_conf_file(void);
void insert_disk(Disk *dsk, char *name, int virtual_image, int size);
void eject_named_disk(Disk *dsk, char *name);
void eject_if_untouched(Disk *dsk);
void eject_disk(Disk *dsk);
void kegs_file_copy(char *orig_name, char *new_name);
void eject_disk_by_num(int slot, int drive);

/* joystick_driver.c */
void joystick_init(void);
void joystick_update(void);

/* moremem.c */
void fixup_brks(void);
void fixup_hires_on(void);
void fixup_bank0_2000_4000(void);
void fixup_bank0_0400_0800(void);
void fixup_any_bank_any_page(int start_page, int num_pages, byte *mem0rd, byte *mem0wr);
void fixup_intcx(void);
void fixup_wrdefram(int new_wrdefram);
void fixup_st80col(double dcycs);
void fixup_altzp(void);
void fixup_page2(double dcycs);
void fixup_ramrd(void);
void fixup_ramwrt(void);
void fixup_lcbank2(void);
void fixup_rdrom(void);
void set_statereg(double dcycs, int val);
void fixup_shadow_txt1(void);
void fixup_shadow_txt2(void);
void fixup_shadow_hires1(void);
void fixup_shadow_hires2(void);
void fixup_shadow_shr(void);
void fixup_shadow_iolc(void);
void update_shadow_reg(int val);
void fixup_shadow_all_banks(void);
void setup_pageinfo(void);
void show_bankptrs_bank0rdwr(void);
void show_bankptrs(int bnk);
void show_addr(byte *ptr);
int io_read(word32 loc, double *cyc_ptr);
void io_write(word32 loc, int val, double *cyc_ptr);
word32 get_lines_since_vbl(double dcycs);
int in_vblank(double dcycs);
int read_vid_counters(int loc, double dcycs);

/* paddles.c */
void paddle_trigger(double dcycs);
void paddle_trigger_mouse(double dcycs);
void paddle_trigger_linux(double dcycs);
int read_paddles(int paddle, double dcycs);

/* sim65816.c */
void show_pc_log(void);
word32 toolbox_debug_4byte(word32 addr);
void toolbox_debug_c(word32 xreg, word32 stack, double *cyc_ptr);
void show_toolbox_log(void);
word32 get_memory_io(word32 loc, double *cyc_ptr);
void set_memory_io(word32 loc, int val, double *cyc_ptr);
void show_regs_act(Engine_reg *eptr);
void show_regs(void);
void my_exit(int ret);
void do_reset(void);
void check_engine_asm_defines(void);
byte *memalloc_align(int size, int skip_amt);
void memory_ptr_init(void);
int main(int argc, char **argv);
void setup_kegs_file(char *outname, int maxlen, int ok_if_missing, const char **name_ptr);
void initialize_events(void);
void check_for_one_event_type(int type);
void add_event_entry(double dcycs, int type);
double remove_event_entry(int type);
void add_event_stop(double dcycs);
void add_event_doc(double dcycs, int osc);
void add_event_scc(double dcycs, int type);
void add_event_vbl(void);
double remove_event_doc(int osc);
double remove_event_scc(int type);
void show_all_events(void);
void show_pmhz(void);
void run_prog(void);
void add_irq(void);
void remove_irq(void);
void take_irq(int is_it_brk);
void show_dtime_array(void);
void update_60hz(double dcycs, double dtime_now);
void do_vbl_int(void);
void do_scan_int(double dcycs, int line);
void check_scan_line_int(double dcycs, int cur_video_line);
void check_for_new_scan_int(double dcycs);
void init_reg(void);
void handle_action(word32 ret);
void do_break(word32 ret);
void do_cop(word32 ret);
void do_wdm(void);
void do_wai(void);
void do_stp(void);
void size_fail(int val, word32 v1, word32 v2);

/* smartport.c */
int get_fd_size(int fd);
void read_partition_block(int fd, void *buf, int blk, int blk_size);
int find_partition_by_name(int fd, char *name, Disk *dsk);
void smartport_error(void);
void smartport_log(word32 start_addr, int cmd, int rts_addr, int cmd_list);
void do_c70d(word32 arg0);
void do_c70a(word32 arg0);
int do_read_c7(int unit_num, word32 buf, int blk);
int do_write_c7(int unit_num, word32 buf, int blk);
int do_format_c7(int unit_num);
void do_c700(word32 ret);

/* sound.c */
void doc_log_rout(char *msg, int osc, double dsamps, int etc);
void show_doc_log(void);
void sound_init(void);
void parent_sound_get_sample_rate(int read_fd);
void set_audio_rate(int rate);
void sound_reset(double dcycs);
void sound_shutdown(void);
void sound_update(double dcycs);
void open_sound_file(void);
void close_sound_file(void);
void check_for_range(word32 *addr, int num_samps, int offset);
void send_sound_to_file(word32 *addr, int shm_pos, int num_samps);
void send_sound(int fd, int real_samps, int size);
void show_c030_state(void);
void show_c030_samps(int *outptr, int num);
void sound_play(double dsamps);
void doc_handle_event(int osc, double dcycs);
void doc_sound_end(int osc, int can_repeat, double eff_dsamps, double dsamps);
void add_sound_irq(int osc);
void remove_sound_irq(int osc, int must);
void start_sound(int osc, double eff_dsamps, double dsamps);
void wave_end_estimate(int osc, double eff_dsamps, double dsamps);
void remove_sound_event(int osc);
void doc_write_ctl_reg(int osc, int val, double dsamps);
void doc_recalc_sound_parms(int osc, double eff_dcycs, double dsamps);
int doc_read_c030(double dcycs);
int doc_read_c03c(double dcycs);
int doc_read_c03d(double dcycs);
void doc_write_c03c(int val, double dcycs);
void doc_write_c03d(int val, double dcycs);
void doc_write_c03e(int val);
void doc_write_c03f(int val);
void doc_show_ensoniq_state(int osc);

/* sound_driver.c */
void reliable_buf_write(word32 *shm_addr, int pos, int size);
void reliable_zero_write(int amt);
void child_sound_loop(int read_fd, int write_fd, word32 *shm_addr);
void child_sound_init_hpdev(void);
void child_sound_init_alib(void);

/* video.c */
void video_init(void);
void show_a2_line_stuff(void);
void video_reset(void);
void video_update(void);
void change_display_mode(double dcycs);
int get_line_stat(int line, int new_all_stat);
void update_a2_ptrs(int line, int new_stat);
void change_a2vid_palette(int new_palette);
void check_a2vid_palette(void);
void update_a2_line_info(void);
void change_border_color(double dcycs, int val);
void update_border_info(void);
void update_border_line(int line_in, int color);
void redraw_changed_text_40(int start_offset, int start_line, int reparse, byte *screen_data, int altcharset, int bg_val, int fg_val);
void redraw_changed_text_80(int start_offset, int start_line, int reparse, byte *screen_data, int altcharset, int bg_val, int fg_val);
void redraw_changed_gr(int start_offset, int start_line, int reparse, byte *screen_data);
void redraw_changed_dbl_gr(int start_offset, int start_line, int reparse, byte *screen_data);
void redraw_changed_hires(int start_offset, int start_line, int color, int reparse, byte *screen_data);
void redraw_changed_hires_bw(int start_offset, int start_line, int reparse, byte *screen_data);
void redraw_changed_hires_color(int start_offset, int start_line, int reparse, byte *screen_data);
void redraw_changed_dbl_hires(int start_offset, int start_line, int color, int reparse, byte *screen_data);
void redraw_changed_dbl_hires_bw(int start_offset, int start_line, int reparse, byte *screen_data);
void redraw_changed_dbl_hires_color(int start_offset, int start_line, int reparse, byte *screen_data);
void check_super_hires_palette_changes(int reparse);
void redraw_changed_super_hires_oneline_norm_320(byte *screen_data, int y, int scan, word32 ch_mask);
void redraw_changed_super_hires_oneline_norm_640(byte *screen_data, int y, int scan, word32 ch_mask);
void redraw_changed_super_hires_oneline_a2vid_320(byte *screen_data, int y, int scan, word32 ch_mask);
void redraw_changed_super_hires_oneline_a2vid_640(byte *screen_data, int y, int scan, word32 ch_mask);
void redraw_changed_super_hires_oneline_fill_320(byte *screen_data, int y, int scan, word32 ch_mask);
void redraw_changed_super_hires_oneline_a2vid_fill_320(byte *screen_data, int y, int scan, word32 ch_mask);
void redraw_changed_super_hires(int start_offset, int start_line, int in_reparse, byte *screen_data);
void display_screen(void);
void refresh_screen(void);
void refresh_line(int line);
void refresh_border(void);
void end_screen(void);
int font_fail(int num);
void read_a2_font(void);
int skip_to_brace(FILE *font_file);
int get_file_byte(FILE *font_file);
int get_token(FILE *font_file);

