/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

// $KmKId: protos_windriver.h,v 1.3 2002-11-19 00:10:38-08 kadickey Exp $

/* windriver.c */
void update_color_array(int col_num, int a2_color);
void update_physical_colormap(void);
void xdriver_end(void);
void dev_video_init(void);
void update_status_line(int line, const char *string);
void redraw_status_lines(void);
void x_refresh_ximage(void);
void check_input_events(void);
int x_keysym_to_a2code(int keysym, int is_up);
void x_auto_repeat_on(int must);
void x_auto_repeat_off(int must);
void win_refresh_lines(HBITMAP xim, int start_line, int end_line, int left_pix, int right_pix);
void win_redraw_border_sides_lines(int end_x, int width, int start_line, int end_line);
void win_refresh_border_sides(void);
void win_refresh_border_special(void);

