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

const char rcsid_protos_x_h[] = "@(#)$Header: protos_xdriver.h,v 1.7 97/09/07 14:27:53 kentd Exp $";

/* END_HDR */

/* xdriver.c */
void update_color_array(int col_num, int a2_color);
void convert_to_xcolor(XColor *xcol, XColor *xcol2, int a2_color);
void update_physical_colormap(void);
void show_xcolor_array(void);
int my_error_handler(Display *dp, XErrorEvent *ev);
void xdriver_end(void);
void dev_video_init(void);
int xhandle_shm_error(Display *display, XErrorEvent *event);
int get_shm(XImage **xim_in, Display *display, byte **databuf, Visual *visual, XShmSegmentInfo *seginfo, int extended_info);
XImage *get_ximage(Display *display, byte **data_ptr, Visual *vis, int extended_info);
void draw_status(int line_num, const char *string);
void x_refresh_ximage(void);
void x_refresh_lines(XImage *xim, int start_line, int end_line, int left_pix, int right_pix);
void x_redraw_border_sides_lines(int end_x, int width, int start_line, int end_line);
void x_refresh_border_sides(void);
void x_refresh_border_special(void);
void check_input_events(void);
void handle_keysym(XEvent *xev_in);
void x_auto_repeat_on(int must);
void x_auto_repeat_off(int must);

