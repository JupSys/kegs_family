/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_protos_mac_h[] = "@(#)$KmKId: protos_macdriver.h,v 1.2 2002-11-19 00:10:38-08 kadickey Exp $";

/* END_HDR */

/* macdriver.c */
void update_color_array(int col_num, int a2_color);
void convert_to_xcolor(int col_num, int a2_color);
void update_physical_colormap(void);
void show_xcolor_array(void);
void xdriver_end(void);
void dev_video_init(void);
byte *mac_get_ximage(byte **data_ptr, int extended_info);
void update_status_line(int line, const char *string);
void redraw_status_lines(void);
void x_refresh_ximage(void);
void x_refresh_lines(byte *xim, int start_line, int end_line, int left_pix, int right_pix);
void x_convert_8to16(byte *ptr_in, byte *ptr_out, int startx, int starty, int width, int height);
void x_convert_8to24(byte *ptr_in, byte *ptr_out, int startx, int starty, int width, int height);
void check_input_events(void);
void x_auto_repeat_on(int must);
void x_auto_repeat_off(int must);

