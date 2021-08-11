/****************************************************************/
/*            Apple IIgs emulator                               */
/*            Copyright 1996 Kent Dickey                        */
/*                                                              */
/*    This code may not be used in a commercial product         */
/*    without prior written permission of the author.           */
/*                                                              */
/*    You may freely distribute this code.                      */ 
/*                                                              */
/*    You can contact the author at kentd@cup.hp.com.           */
/*                                                              */
/****************************************************************/

/* xdriver.c */
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

