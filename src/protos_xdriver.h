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

const char rcsid_protos_x_h[] = "@(#)$Header: protos_xdriver.h,v 1.13 2000/10/03 01:14:41 kentd Exp $";

/* END_HDR */

/* xdriver.c */
void update_color_array(int col_num, int a2_color);
void convert_to_xcolor(XColor *xcol, XColor *xcol2, int col_num, int a2_color, int doit);
void update_physical_colormap(void);
void show_xcolor_array(void);
int my_error_handler(Display *dp, XErrorEvent *ev);
void xdriver_end(void);
void show_colormap(char *str, Colormap cmap, int index1, int index2, int index3);
void dev_video_init(void);
Visual *x_try_find_visual(Display *display, int depth, int screen_num, XVisualInfo **visual_list_ptr);
void x_set_mask_and_shift(word32 x_mask, word32 *mask_ptr, int *shift_left_ptr, int *shift_right_ptr);
int xhandle_shm_error(Display *display, XErrorEvent *event);
int get_shm(XImage **xim_in, Display *display, byte **databuf, Visual *visual, XShmSegmentInfo *seginfo, int extended_info);
XImage *get_ximage(Display *display, byte **data_ptr, Visual *vis, int extended_info);
void update_status_line(int line, const char *string);
void redraw_status_lines(void);
void x_refresh_ximage(void);
void x_refresh_lines(XImage *xim, int start_line, int end_line, int left_pix, int right_pix);
void x_redraw_border_sides_lines(int end_x, int width, int start_line, int end_line);
void x_refresh_border_sides(void);
void x_refresh_border_special(void);
void x_convert_8to16(XImage *xim, XImage *xout, int startx, int starty, int width, int height);
void x_convert_8to24(XImage *xim, XImage *xout, int startx, int starty, int width, int height);
void check_input_events(void);
void handle_keysym(XEvent *xev_in);
int x_keysym_to_a2code(int keysym, int is_up);
void x_update_modifier_state(int state);
void x_auto_repeat_on(int must);
void x_auto_repeat_off(int must);

