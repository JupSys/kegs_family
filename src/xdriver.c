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

const char rcsid_xdriver_c[] = "@(#)$Header: xdriver.c,v 1.123 97/09/08 21:27:50 kentd Exp $";

#define X_SHARED_MEM

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <time.h>
#include <stdlib.h>

#ifdef X_SHARED_MEM
# include <sys/ipc.h>
# include <sys/shm.h>
# include <X11/extensions/XShm.h>
#endif

int XShmQueryExtension(Display *display);

#include "defc.h"

#include "protos_xdriver.h"


#define FONT_NAME_STATUS	"8x13"

extern int Verbose;

extern int shift_key_down;
extern int ctrl_key_down;
extern int capslock_key_down;
extern int numlock_key_down;
extern int option_key_down;
extern int cmd_key_down;
extern int keypad_key_down;
extern int updated_mod_latch;
extern int g_limit_speed;

extern int mouse_phys_x;
extern int mouse_phys_y;
extern int mouse_button;
extern int g_swap_paddles;
extern int g_invert_paddles;
extern int _Xdebug;

extern int g_send_sound_to_file;

int g_has_focus = 0;
int g_auto_repeat_on = -1;


Display *display = 0;
Window base_win;
Window status_win;
Window a2_win;
GC base_winGC;
GC a2_winGC;
XFontStruct *text_FontSt;
Colormap a2_colormap;
Colormap base_colormap;

XColor def_colors[256];

#ifdef X_SHARED_MEM
int use_shmem = 1;
#else
int use_shmem = 0;
#endif

byte *data_text[2];
byte *data_hires[2];
byte *data_superhires;
byte *data_border_special;
byte *data_border_sides;
XImage *ximage_text[2];
XImage *ximage_hires[2];
XImage *ximage_superhires;
XImage *ximage_border_special;
XImage *ximage_border_sides;

#ifdef X_SHARED_MEM
XShmSegmentInfo shm_text_seginfo[2];
XShmSegmentInfo shm_hires_seginfo[2];
XShmSegmentInfo shm_superhires_seginfo;
XShmSegmentInfo shm_border_special_seginfo;
XShmSegmentInfo shm_border_sides_seginfo;
#endif

int Max_color_size = 256;

XColor xcolor_a2vid_array[256];
XColor xcolor_superhires_array[256];
XColor border_xcolor;
XColor dummy_xcolor;

int	g_pixel_black = 0;
int	g_pixel_white = 1;

int g_Control_L_up = 1;
int g_Control_R_up = 1;
int g_Shift_L_up = 1;
int g_Shift_R_up = 1;

int	g_alt_left_up = 1;
int	g_alt_right_up = 1;

extern word32 g_full_refresh_needed;

extern int g_border_sides_refresh_needed;
extern int g_border_special_refresh_needed;

extern int lores_colors[];
extern int hires_colors[];
extern int g_cur_a2_stat;

extern int g_a2vid_palette;

extern int g_installed_full_superhires_colormap;

extern word32 a2_screen_buffer_changed;
extern byte *a2_line_ptr[];
extern void *a2_line_xim[];
extern int a2_line_stat[];
extern int a2_line_left_edge[];
extern int a2_line_right_edge[];
extern int a2_line_full_left_edge[];
extern int a2_line_full_right_edge[];



#define X_EVENT_LIST_ALL_WIN						\
	(ExposureMask | ButtonPressMask | ButtonReleaseMask |		\
	 OwnerGrabButtonMask | KeyPressMask | KeyReleaseMask |		\
	 KeymapStateMask | ColormapChangeMask | FocusChangeMask)

#define X_BASE_WIN_EVENT_LIST						\
	(X_EVENT_LIST_ALL_WIN | PointerMotionMask | ButtonMotionMask)

#define X_A2_WIN_EVENT_LIST						\
	(X_BASE_WIN_EVENT_LIST)

int	g_num_a2_keycodes = 0;

int a2_key_to_xsym[][3] = {
	{ 0x35,	XK_Escape,	0 },
	{ 0x7a,	XK_F1,	0 },
	{ 0x7b,	XK_F2,	0 },
	{ 0x63,	XK_F3,	0 },
	{ 0x76,	XK_F4,	0 },
	{ 0x60,	XK_F5,	0 },
	{ 0x61,	XK_F6,	0 },
	{ 0x62,	XK_F7,	0 },
	{ 0x64,	XK_F8,	0 },
	{ 0x65,	XK_F9,	0 },
	{ 0x6d,	XK_F10,	0 },
	{ 0x67,	XK_F11,	0 },
	{ 0x6f,	XK_F12,	0 },
	{ 0x69,	XK_F13,	0 },
	{ 0x6b,	XK_F14,	0 },
	{ 0x71,	XK_F15,	0 },
	{ 0x7f, XK_Pause, XK_Break },
	{ 0x32,	'`', '~' },		/* Key number 18? */
	{ 0x12,	'1', '!' },
	{ 0x13,	'2', '@' },
	{ 0x14,	'3', '#' },
	{ 0x15,	'4', '$' },
	{ 0x17,	'5', '%' },
	{ 0x16,	'6', '^' },
	{ 0x1a,	'7', '&' },
	{ 0x1c,	'8', '*' },
	{ 0x19,	'9', '(' },
	{ 0x1d,	'0', ')' },
	{ 0x1b,	'-', '_' },
	{ 0x18,	'=', '+' },
	{ 0x33,	XK_BackSpace, 0 },
	{ 0x72,	XK_Insert, 0 },		/* Help? */
	{ 0x73,	XK_Home, 0 },
	{ 0x74,	XK_Page_Up, 0 },
	{ 0x47,	XK_Num_Lock, XK_Clear },	/* Clear */
	{ 0x51,	XK_KP_Equal, 0 },
	{ 0x4b,	XK_KP_Divide, 0 },
	{ 0x43,	XK_KP_Multiply, 0 },

	{ 0x30,	XK_Tab, 0 },
	{ 0x0c,	'q', 'Q' },
	{ 0x0d,	'w', 'W' },
	{ 0x0e,	'e', 'E' },
	{ 0x0f,	'r', 'R' },
	{ 0x11,	't', 'T' },
	{ 0x10,	'y', 'Y' },
	{ 0x20,	'u', 'U' },
	{ 0x22,	'i', 'I' },
	{ 0x1f,	'o', 'O' },
	{ 0x23,	'p', 'P' },
	{ 0x21,	'[', '{' },
	{ 0x1e,	']', '}' },
	{ 0x2a,	0x5c, '|' },	/* backslash, bar */
	{ 0x75,	XK_Delete, 0 },
	{ 0x77,	XK_End, 0 },
	{ 0x79,	XK_Page_Down, 0 },
	{ 0x59,	XK_KP_7, XK_KP_Home },
	{ 0x5b,	XK_KP_8, XK_KP_Up },
	{ 0x5c,	XK_KP_9, XK_KP_Page_Up },
	{ 0x4e,	XK_KP_Subtract, 0 },

	{ 0x39,	XK_Caps_Lock, 0 },
	{ 0x00,	'a', 'A' },
	{ 0x01,	's', 'S' },
	{ 0x02,	'd', 'D' },
	{ 0x03,	'f', 'F' },
	{ 0x05,	'g', 'G' },
	{ 0x04,	'h', 'H' },
	{ 0x26,	'j', 'J' },
	{ 0x28,	'k', 'K' },
	{ 0x25,	'l', 'L' },
	{ 0x29,	';', ':' },
	{ 0x27,	0x27, '"' },	/* single quote */
	{ 0x24,	XK_Return, 0 },
	{ 0x56,	XK_KP_4, XK_KP_Left },
	{ 0x57,	XK_KP_5, 0 },
	{ 0x58,	XK_KP_6, XK_KP_Right },
	{ 0x45,	XK_KP_Add, 0 },

	{ 0x38,	XK_Shift_L, XK_Shift_R },
	{ 0x06,	'z', 'Z' },
	{ 0x07,	'x', 'X' },
	{ 0x08,	'c', 'C' },
	{ 0x09,	'v', 'V' },
	{ 0x0b,	'b', 'B' },
	{ 0x2d,	'n', 'N' },
	{ 0x2e,	'm', 'M' },
	{ 0x2b,	',', '<' },
	{ 0x2f,	'.', '>' },
	{ 0x2c,	'/', '?' },
	{ 0x3e,	XK_Up, 0 },
	{ 0x53,	XK_KP_1, XK_KP_End },
	{ 0x54,	XK_KP_2, XK_KP_Down },
	{ 0x55,	XK_KP_3, XK_KP_Page_Down },

	{ 0x36,	XK_Control_L, XK_Control_R },
	{ 0x3a,	XK_Print, XK_Sys_Req },		/* Option */
	{ 0x37,	XK_Scroll_Lock, 0 },		/* Command */
	{ 0x31,	' ', 0 },
	{ 0x3b,	XK_Left, 0 },
	{ 0x3d,	XK_Down, 0 },
	{ 0x3c,	XK_Right, 0 },
	{ 0x52,	XK_KP_0, XK_KP_Insert },
	{ 0x41,	XK_KP_Decimal, XK_KP_Separator },
	{ 0x4c,	XK_KP_Enter, 0 },
	{ -1, -1, -1 }
};


void
update_color_array(int col_num, int a2_color)
{
	XColor	*xcol, *xcol2;
	int	palette;

	if(col_num >= 256 || col_num < 0) {
		printf("update_color_array called: col: %03x\n", col_num);
		set_halt(1);
		return;
	}

	xcol = &xcolor_superhires_array[col_num];
	xcol2 = &xcolor_a2vid_array[col_num];
	palette = col_num >> 4;
	if(palette == g_a2vid_palette) {
		xcol2 = &dummy_xcolor;
	}

	convert_to_xcolor(xcol, xcol2, a2_color);
}

#define HAS(a,b)  (((a) & (b)) == (b))


#define MAKE_4(val)	( (val << 12) + (val << 8) + (val << 4) + val)

void
convert_to_xcolor(XColor *xcol, XColor *xcol2, int a2_color)
{
	int	tmp;

	tmp = (a2_color >> 8) & 0xf;
	xcol->red = MAKE_4(tmp);
	xcol2->red = MAKE_4(tmp);
	tmp = (a2_color >> 4) & 0xf;
	xcol->green = MAKE_4(tmp);
	xcol2->green = MAKE_4(tmp);
	tmp = (a2_color) & 0xf;
	xcol->blue = MAKE_4(tmp);
	xcol2->blue = MAKE_4(tmp);

	xcol->flags = DoRed | DoGreen | DoBlue;
	xcol2->flags = DoRed | DoGreen | DoBlue;
}


extern int flash_state;

void
update_physical_colormap()
{
	int	palette;
	int	i;

	palette = g_a2vid_palette << 4;

	for(i = 0; i < 16; i++) {
		convert_to_xcolor(&xcolor_a2vid_array[palette + i],
			&dummy_xcolor, lores_colors[i]);
	}

	if(g_installed_full_superhires_colormap) {
		XStoreColors(display, a2_colormap, &xcolor_superhires_array[0],
			Max_color_size);
	} else {
		XStoreColors(display, a2_colormap, &xcolor_a2vid_array[0],
			Max_color_size);
	}
}

void
show_xcolor_array()
{
	int i;

	for(i = 0; i < 256; i++) {
#if 0
		printf("%02x: %04x %04x %04x, %02x %x\n",
			i, xcolor_array[i].red, xcolor_array[i].green,
			xcolor_array[i].blue, (word32)xcolor_array[i].pixel,
			xcolor_array[i].flags);
#endif
	}
}


int
my_error_handler(Display *dp, XErrorEvent *ev)
{
	char msg[1024];
	XGetErrorText(dp, ev->error_code, msg, 1000);
	printf("X Error code %s\n", msg);
	fflush(stdout);

	return 0;
}

void
xdriver_end()
{

	printf("xdriver_end\n");
	if(display) {
		x_auto_repeat_on(1);
		XFlush(display);
	}
}

void
dev_video_init()
{
	XGCValues new_gc;
	XVisualInfo vTemplate;
	XSetWindowAttributes win_attr;
	XSizeHints my_winSizeHints;
	XClassHint my_winClassHint;
	XTextProperty my_winText;
	XVisualInfo *visualList;
	Visual	*vis;
	char	**font_ptr;
	int	cnt;
	int	font_height;
	int	screen_num;
	int	visualsMatched;
	char	*myTextString[1];
	int	visual_chosen;
	int	ret;
	int	i;
	int	keycode;
	int	tmp_array[0x80];

	printf("Preparing X Windows graphics system\n");

	g_num_a2_keycodes = 0;
	for(i = 0; i < 0x7f; i++) {
		tmp_array[i] = 0;
	}
	for(i = 0; i < 0x7f; i++) {
		keycode = a2_key_to_xsym[i][0];
		if(keycode < 0) {
			g_num_a2_keycodes = i;
			break;
		} else if(keycode > 0x7f) {
			printf("a2_key_to_xsym[%d] = %02x!\n", i, keycode);
				exit(2);
		} else {
			if(tmp_array[keycode]) {
				printf("a2_key_to_x[%d] = %02x used by %d\n",
					i, keycode, tmp_array[keycode] - 1);
			}
			tmp_array[keycode] = i + 1;
		}
	}

#if 0
	printf("Setting _Xdebug = 1, makes X synchronous\n");
	_Xdebug = 1;
#endif

	display = XOpenDisplay(NULL);
	if(display == NULL) {
		fprintf(stderr, "Can't open display\n");
		exit(1);
	}

	vid_printf("Just opened display = %08x\n", (word32)display);
	fflush(stdout);

	screen_num = DefaultScreen(display);
	g_pixel_black = BlackPixel(display, screen_num);
	g_pixel_white = WhitePixel(display, screen_num);

	vTemplate.screen = screen_num;
	vTemplate.depth = 8;

	visualList = XGetVisualInfo(display, VisualScreenMask | VisualDepthMask,
		&vTemplate, &visualsMatched);

	vid_printf("visuals matched: %d\n", visualsMatched);
	if(visualsMatched == 0) {
		fprintf(stderr, "no visuals!\n");
		exit(2);
	}

	visual_chosen = -1;
	for(i = 0; i < visualsMatched; i++) {
		printf("Visual %d\n", i);
		printf("	id: %08x, screen: %d, depth: %d, class: %d\n",
			(word32)visualList[i].visualid,
			visualList[i].screen,
			visualList[i].depth,
			visualList[i].class);
		printf("	red: %08lx, green: %08lx, blue: %08lx\n",
			visualList[i].red_mask,
			visualList[i].green_mask,
			visualList[i].blue_mask);
		printf("	cmap size: %d, bits_per_rgb: %d\n",
			visualList[i].colormap_size,
			visualList[i].bits_per_rgb);
		if(visualList[i].class == PseudoColor) {
			visual_chosen = i;
			Max_color_size = visualList[i].colormap_size;
			break;
		}
	}

	if(visual_chosen < 0) {
		fprintf(stderr,"Couldn't find any good visuals!\n");
		exit(3);
	}

	printf("Chose visual: %d, max_colors: %d\n", visual_chosen,
		Max_color_size);

	vis = visualList[visual_chosen].visual;


	base_colormap = XDefaultColormap(display, screen_num);
	if(!base_colormap) {
		printf("base_colormap == 0!\n");
		exit(4);
	}


	a2_colormap = XCreateColormap (display, RootWindow(display, screen_num),
		vis, AllocAll);

	vid_printf("a2_colormap: %08x, main: %08x\n",
				(word32)a2_colormap, (word32)base_colormap);

	if(a2_colormap == base_colormap) {
		printf("A2_colormap = default colormap!\n");
		exit(4);
	}

	ret = XAllocColorCells(display, base_colormap, False,
		NULL, 0, &border_xcolor.pixel, 1);

	vid_printf("XAllocColorCells ret: %d, border_xcolor.pixel: %ld\n",
		ret, border_xcolor.pixel);

	if(ret == 0) {
		printf("XAllocColorCells = 0!!!\n");
		exit(5);
	}

	border_xcolor.red = 0xffff;
	border_xcolor.green = 0;
	border_xcolor.blue = 0;
	border_xcolor.flags = DoRed | DoGreen | DoBlue;

	XStoreColor(display, base_colormap, &border_xcolor);

	XFlush(display);

	win_attr.background_pixel = border_xcolor.pixel;
	win_attr.event_mask = X_BASE_WIN_EVENT_LIST;

	win_attr.colormap = base_colormap;

	vid_printf("About to base_win\n");
	fflush(stdout);

	base_win = XCreateWindow(display, RootWindow(display, screen_num),
		0, 0, BASE_WINDOW_WIDTH, BASE_WINDOW_HEIGHT,
		0, vTemplate.depth, InputOutput,
		DefaultVisual(display, screen_num),
		CWEventMask | CWBackPixel,
		&win_attr);

	win_attr.background_pixel = g_pixel_black;
	win_attr.event_mask = X_A2_WIN_EVENT_LIST;
	status_win = XCreateWindow(display, base_win,
		0, X_A2_WINDOW_HEIGHT,
		BASE_WINDOW_WIDTH, STATUS_WINDOW_HEIGHT,
		0, vTemplate.depth, InputOutput,
		DefaultVisual(display, screen_num),
		CWBackPixel | CWEventMask,
		&win_attr);

	XFlush(display);

	win_attr.colormap = a2_colormap;
	win_attr.backing_store = WhenMapped;

	vid_printf("About to a2_win\n");
	fflush(stdout);

	a2_win = XCreateWindow(display, base_win,
		0, 0,
		X_A2_WINDOW_WIDTH, X_A2_WINDOW_HEIGHT,
		0, vTemplate.depth, InputOutput, vis,
		CWEventMask | CWColormap | CWBackingStore,
		&win_attr);

	XSetWindowColormap(display, a2_win, a2_colormap);
	ret = XSetWMColormapWindows(display, base_win, &a2_win, 1);
	vid_printf("XSetWMcolormapWindows ret: %08x\n", ret);
	if(ret == 0) {
		printf("XSetWMColormapWindows ret= 0\n");
	}

	XFlush(display);

/* Check for XShm */
#ifdef X_SHARED_MEM
	if(use_shmem) {
		ret = XShmQueryExtension(display);
		if(ret == 0) {
			printf("XShmQueryExt ret: %d\n", ret);
			printf("not using shared memory\n");
			use_shmem = 0;
		} else {
			printf("Will use shared memory for X\n");
		}
	}

	if(use_shmem) {
		use_shmem = get_shm(&ximage_text[0], display, &data_text[0],
			vis, &shm_text_seginfo[0], 0);
	}
	if(use_shmem) {
		use_shmem = get_shm(&ximage_text[1], display, &data_text[1],
			vis, &shm_text_seginfo[1], 0);
	}
	if(use_shmem) {
		use_shmem = get_shm(&ximage_hires[0], display, &data_hires[0],
			vis, &shm_hires_seginfo[0], 0);
	}
	if(use_shmem) {
		use_shmem = get_shm(&ximage_hires[1], display, &data_hires[1],
			vis, &shm_hires_seginfo[1], 0);
	}
	if(use_shmem) {
		use_shmem = get_shm(&ximage_superhires, display,
			&data_superhires, vis, &shm_superhires_seginfo, 0);
	}
	if(use_shmem) {
		use_shmem = get_shm(&ximage_border_special, display,
			&data_border_special,vis,&shm_border_special_seginfo,1);
	}
	if(use_shmem) {
		use_shmem = get_shm(&ximage_border_sides, display,
			&data_border_sides, vis, &shm_border_sides_seginfo, 2);
	}
#endif
	if(!use_shmem) {
		printf("Calling get_ximage!\n");

		ximage_text[0] = get_ximage(display, &data_text[0], vis, 0);
		ximage_text[1] = get_ximage(display, &data_text[1], vis, 0);
		ximage_hires[0] = get_ximage(display, &data_hires[0], vis, 0);
		ximage_hires[1] = get_ximage(display, &data_hires[1], vis, 0);
		ximage_superhires = get_ximage(display, &data_superhires,vis,0);
		ximage_border_special = get_ximage(display,
						&data_border_special, vis, 1);
		ximage_border_sides = get_ximage(display, &data_border_sides,
									vis, 2);
	}

	vid_printf("data_text[0]: %08x, use_shmem: %d\n",
				(word32)data_text[0], use_shmem);

	/* Done with visualList now */
	XFree(visualList);

	for(i = 0; i < 256; i++) {
		xcolor_a2vid_array[i].flags = DoRed | DoGreen | DoBlue;
		xcolor_a2vid_array[i].pixel = i;
		xcolor_a2vid_array[i].red = i*256;
		xcolor_a2vid_array[i].green = i*256;
		xcolor_a2vid_array[i].blue = i*256;

		xcolor_superhires_array[i].flags = DoRed | DoGreen | DoBlue;
		xcolor_superhires_array[i].pixel = i;
		xcolor_superhires_array[i].red = i*256;
		xcolor_superhires_array[i].green = i*256;
		xcolor_superhires_array[i].blue = i*256;
	}

	XStoreColors(display, a2_colormap, &xcolor_a2vid_array[0],
		Max_color_size);

	g_installed_full_superhires_colormap = 0;
	
	myTextString[0] = "Sim65";

	XStringListToTextProperty(myTextString, 1, &my_winText);

	my_winSizeHints.flags = PSize | PMinSize | PMaxSize;
	my_winSizeHints.width = BASE_WINDOW_WIDTH;
	my_winSizeHints.height = BASE_WINDOW_HEIGHT;
	my_winSizeHints.min_width = BASE_WINDOW_WIDTH;
	my_winSizeHints.min_height = BASE_WINDOW_HEIGHT;
	my_winSizeHints.max_width = BASE_WINDOW_WIDTH;
	my_winSizeHints.max_height = BASE_WINDOW_HEIGHT;
	my_winClassHint.res_name = "sim65";
	my_winClassHint.res_class = "Sim65";

	XSetWMProperties(display, base_win, &my_winText, &my_winText, 0,
		0, &my_winSizeHints, 0, &my_winClassHint);
	XMapRaised(display, base_win);

	XSetWMProperties(display, a2_win, &my_winText, &my_winText, 0,
		0, &my_winSizeHints, 0, &my_winClassHint);
	XMapRaised(display, a2_win);

	XSetWMProperties(display, status_win, &my_winText, &my_winText, 0,
		0, &my_winSizeHints, 0, &my_winClassHint);
	XMapRaised(display, status_win);

	XSync(display, False);

	base_winGC = XCreateGC(display, base_win, 0, (XGCValues *) 0);
	a2_winGC = XCreateGC(display, a2_win, 0, (XGCValues *) 0);
	font_ptr = XListFonts(display, FONT_NAME_STATUS, 4, &cnt);

	vid_printf("act_cnt of fonts: %d\n", cnt);
	for(i = 0; i < cnt; i++) {
		vid_printf("Font %d: %s\n", i, font_ptr[i]);
	}
	fflush(stdout);
	text_FontSt = XLoadQueryFont(display, FONT_NAME_STATUS);
	vid_printf("font # returned: %08x\n", (word32)(text_FontSt->fid));
	font_height = text_FontSt->ascent + text_FontSt->descent;
	vid_printf("font_height: %d\n", font_height);

	vid_printf("widest width: %d\n", text_FontSt->max_bounds.width);

	new_gc.font = text_FontSt->fid;
	new_gc.fill_style = FillSolid;
	XChangeGC(display, base_winGC, GCFillStyle | GCFont, &new_gc);
	XChangeGC(display, a2_winGC, GCFillStyle | GCFont, &new_gc);

	XSetForeground(display, base_winGC, border_xcolor.pixel);

	XSync(display, False);
	XFillRectangle(display, base_win, base_winGC, 0, 0,
		BASE_WINDOW_WIDTH,
		BASE_MARGIN_TOP + BASE_MARGIN_BOTTOM + X_A2_WINDOW_HEIGHT);

	XFlush(display);

	/* XSync(display, False); */


	XFlush(display);
	fflush(stdout);
}

#ifdef X_SHARED_MEM
int xshm_error = 0;

int
xhandle_shm_error(Display *display, XErrorEvent *event)
{
	xshm_error = 1;
	return 0;
}

int
get_shm(XImage **xim_in, Display *display, byte **databuf, Visual *visual,
		XShmSegmentInfo *seginfo, int extended_info)
{
	XImage *xim;
	int	(*old_x_handler)(Display *, XErrorEvent *);
	int	width;
	int	height;

	width = A2_WINDOW_WIDTH;
	height = A2_WINDOW_HEIGHT;
	if(extended_info == 1) {
		/* border at top and bottom of screen */
		width = X_A2_WINDOW_WIDTH;
		height = X_A2_WINDOW_HEIGHT - A2_WINDOW_HEIGHT + 2*8;
	}
	if(extended_info == 2) {
		/* border at sides of screen */
		width = EFF_BORDER_WIDTH;
		height = A2_WINDOW_HEIGHT;
	}

	xim = XShmCreateImage(display, visual, 8, ZPixmap,
		(char *)0, seginfo, width, height);

	vid_printf("xim: %08x\n", (word32)xim);
	*xim_in = xim;
	if(xim == 0) {
		return 0;
	}

	/* It worked, we got it */
	seginfo->shmid = shmget(IPC_PRIVATE, xim->bytes_per_line * xim->height,
		IPC_CREAT | 0777);
	vid_printf("seginfo->shmid = %d\n", seginfo->shmid);
	if(seginfo->shmid < 0) {
		XDestroyImage(xim);
		return 0;
	}

	/* Still working */
	seginfo->shmaddr = (char *)shmat(seginfo->shmid, 0, 0);
	vid_printf("seginfo->shmaddr: %08x\n", (word32)seginfo->shmaddr);
	if(seginfo->shmaddr == ((char *) -1)) {
		XDestroyImage(xim);
		return 0;
	}

	/* Still working */
	xim->data = seginfo->shmaddr;
	seginfo->readOnly = False;

	/* XShmAttach will trigger X error if server is remote, so catch it */
	xshm_error = 0;
	old_x_handler = XSetErrorHandler(xhandle_shm_error);

	XShmAttach(display, seginfo);
	XSync(display, False);


	vid_printf("about to RMID the shmid\n");
	shmctl(seginfo->shmid, IPC_RMID, 0);

	if(xshm_error) {
		XFlush(display);
		XDestroyImage(xim);
		/* We could release the shared mem segment, but by doing the */
		/* RMID, it will go away when we die now, so just leave it */
		XSetErrorHandler(old_x_handler);
		printf("Not using shared memory\n");
		return 0;
	}

	*databuf = (byte *)xim->data;
	vid_printf("Sharing memory. xim: %08x, xim->data: %08x\n",
		(word32)xim, (word32)xim->data);

	return 1;
}
#endif	/* X_SHARED_MEM */

XImage *
get_ximage(Display *display, byte **data_ptr, Visual *vis, int extended_info)
{
	XImage	*xim;
	byte	*ptr;
	int	width;
	int	height;

	width = A2_WINDOW_WIDTH;
	height = A2_WINDOW_HEIGHT;
	if(extended_info == 1) {
		/* border at top and bottom of screen */
		width = X_A2_WINDOW_WIDTH;
		height = X_A2_WINDOW_HEIGHT - A2_WINDOW_HEIGHT;
	}
	if(extended_info == 2) {
		/* border at sides of screen */
		width = EFF_BORDER_WIDTH;
		height = A2_WINDOW_HEIGHT;
	}

	ptr = (byte *)malloc(width * height);

	vid_printf("ptr: %08x\n", (word32)ptr);

	if(ptr == 0) {
		printf("malloc for data failed\n");
		exit(2);
	}

	*data_ptr = ptr;

	xim = XCreateImage(display, vis, 8, ZPixmap, 0, (char *)ptr,
		width, height, 8, width);

	vid_printf("xim.data: %08x\n", (word32)xim->data);

	return xim;
}




double status1_time = 0.0;
double status2_time = 0.0;
double status3_time = 0.0;

#define X_LINE_LENGTH	84

void
draw_status(int line_num, const char *string)
{
	char	buf[256];
	const char *ptr;
	int	height;
	int	margin;
	int	i;

	height = text_FontSt->ascent + text_FontSt->descent;
	margin = text_FontSt->ascent;

	ptr = string;
	for(i = 0; i < X_LINE_LENGTH; i++) {
		if(*ptr) {
			buf[i] = *ptr++;
		} else {
			buf[i] = ' ';
		}
	}

	buf[X_LINE_LENGTH] = 0;
	
	XSetForeground(display, base_winGC, g_pixel_white);
	XSetBackground(display, base_winGC, g_pixel_black);

	XDrawImageString(display, status_win, base_winGC, 0,
		height*line_num + margin,
		buf, strlen(buf));

	XFlush(display);
}



word32 g_cycs_in_xredraw = 0;

void
x_refresh_ximage()
{
	register word32 start_time;
	register word32 end_time;
	int	start;
	word32	mask;
	int	line;
	int	left_pix, right_pix;
	int	left, right;
	XImage	*last_xim, *cur_xim;

	if(g_border_sides_refresh_needed) {
		g_border_sides_refresh_needed = 0;
		x_refresh_border_sides();
	}
	if(g_border_special_refresh_needed) {
		g_border_special_refresh_needed = 0;
		x_refresh_border_special();
	}

	if(a2_screen_buffer_changed == 0) {
		return;
	}

	GET_ITIMER(start_time);

	start = -1;
	mask = 1;
	last_xim = (XImage *)-1;

	left_pix = 640;
	right_pix = 0;

	for(line = 0; line < 25; line++) {
		if((g_full_refresh_needed & (1 << line)) != 0) {
			left = a2_line_full_left_edge[line];
			right = a2_line_full_right_edge[line];
		} else {
			left = a2_line_left_edge[line];
			right = a2_line_right_edge[line];
		}

		if(!(a2_screen_buffer_changed & mask)) {
			/* No need to update this line */
			/* Refresh previous chunks of lines, if any */
			if(start >= 0) {
				x_refresh_lines(last_xim, start, line,
					left_pix, right_pix);
				start = -1;
				left_pix = 640;
				right_pix = 0;
			}
		} else {
			/* Need to update this line */
			cur_xim = a2_line_xim[line];
			if(start < 0) {
				start = line;
				last_xim = cur_xim;
			}
			if(cur_xim != last_xim) {
				/* do the refresh */
				x_refresh_lines(last_xim, start, line,
					left_pix, right_pix);
				last_xim = cur_xim;
				start = line;
				left_pix = left;
				right_pix = right;
			}
			left_pix = MIN(left, left_pix);
			right_pix = MAX(right, right_pix);
		}
		mask = mask << 1;
	}

	if(start >= 0) {
		x_refresh_lines(last_xim, start, 25, left_pix, right_pix);
	}

	a2_screen_buffer_changed = 0;

	g_full_refresh_needed = 0;

	/* And redraw border rectangle? */

	XFlush(display);

	GET_ITIMER(end_time);

	g_cycs_in_xredraw += (end_time - start_time);
}

void
x_refresh_lines(XImage *xim, int start_line, int end_line, int left_pix,
		int right_pix)
{
	int	srcy;

	if(left_pix >= right_pix || left_pix < 0 || right_pix <= 0) {
		printf("x_refresh_lines: lines %d to %d, pix %d to %d\n",
			start_line, end_line, left_pix, right_pix);
		printf("a2_screen_buf_ch:%08x, g_full_refr:%08x\n",
			a2_screen_buffer_changed, g_full_refresh_needed);
		show_a2_line_stuff();
		U_STACK_TRACE();
		set_halt(1);
	}

	srcy = 16*start_line;

	if(xim == ximage_border_special) {
		/* fix up y pos in src */
		srcy = 0;
	}

#ifdef X_SHARED_MEM
	if(use_shmem) {
		XShmPutImage(display, a2_win, a2_winGC,
			xim, left_pix, srcy,
			BASE_MARGIN_LEFT + left_pix,
					BASE_MARGIN_TOP + 16*start_line,
			right_pix - left_pix, 16*(end_line - start_line),False);
	}
#endif
	if(!use_shmem) {
		XPutImage(display, a2_win, a2_winGC, xim,
			left_pix, srcy,
			BASE_MARGIN_LEFT + left_pix,
					BASE_MARGIN_TOP + 16*start_line,
			right_pix - left_pix, 16*(end_line - start_line));
	}
}

void
x_redraw_border_sides_lines(int end_x, int width, int start_line,
	int end_line)
{

	if(start_line < 0 || width < 0) {
		return;
	}

#if 0
	printf("redraw_border_sides lines:%d-%d from %d to %d\n",
		start_line, end_line, end_x - width, end_x);
#endif

#ifdef X_SHARED_MEM
	if(use_shmem) {
		XShmPutImage(display, a2_win, a2_winGC, ximage_border_sides,
			0, 16*start_line,
			end_x - width, BASE_MARGIN_TOP + 16*start_line,
			width, 16*(end_line - start_line), False);
	}
#endif
	if(!use_shmem) {
		XPutImage(display, a2_win, a2_winGC, ximage_border_sides,
			0, 16*start_line,
			end_x - width, BASE_MARGIN_TOP + 16*start_line,
			width, 16*(end_line - start_line));
	}


}

void
x_refresh_border_sides()
{
	int	old_width;
	int	prev_line;
	int	width;
	int	mode;
	int	i;

#if 0
	printf("refresh border sides!\n");
#endif

	/* redraw left sides */
	x_redraw_border_sides_lines(BORDER_WIDTH, BORDER_WIDTH, 0, 25);

	/* right side--can be "jagged" */
	prev_line = -1;
	old_width = -1;
	for(i = 0; i < 25; i++) {
		mode = (a2_line_stat[i] >> 4) & 7;
		width = EFF_BORDER_WIDTH;
		if(mode == MODE_SUPER_HIRES || mode == MODE_BORDER) {
			width = BORDER_WIDTH;
		}
		if(width != old_width) {
			x_redraw_border_sides_lines(X_A2_WINDOW_WIDTH,
				old_width, prev_line, i);
			prev_line = i;
			old_width = width;
		}
	}

	x_redraw_border_sides_lines(X_A2_WINDOW_WIDTH, old_width, prev_line,25);

	XFlush(display);
}

void
x_refresh_border_special()
{
	int	width, height;

	width = X_A2_WINDOW_WIDTH;
	height = BASE_MARGIN_TOP;
#ifdef X_SHARED_MEM
	if(use_shmem) {
		XShmPutImage(display, a2_win, a2_winGC, ximage_border_special,
			0, 0,
			0, BASE_MARGIN_TOP + A2_WINDOW_HEIGHT,
			width, BASE_MARGIN_BOTTOM, False);

		XShmPutImage(display, a2_win, a2_winGC, ximage_border_special,
			0, BASE_MARGIN_BOTTOM,
			0, 0,
			width, BASE_MARGIN_TOP, False);
	}
#endif
	if(!use_shmem) {
		XPutImage(display, a2_win, a2_winGC, ximage_border_special,
			0, 0,
			0, BASE_MARGIN_TOP + A2_WINDOW_HEIGHT,
			width, BASE_MARGIN_BOTTOM);
		XPutImage(display, a2_win, a2_winGC, ximage_border_special,
			0, BASE_MARGIN_BOTTOM,
			0, 0,
			width, BASE_MARGIN_TOP);
	}
}

#if 0
void
redraw_border()
{

}
#endif


#define KEYBUFLEN	128

int g_num_check_input_calls = 0;
int g_check_input_flush_rate = 2;

void
check_input_events()
{
	XEvent	ev;
	int	len;
	int	delta_x, delta_y;

	g_num_check_input_calls--;
	if(g_num_check_input_calls < 0) {
		len = XPending(display);
		g_num_check_input_calls = g_check_input_flush_rate;
	} else {
		len = QLength(display);
	}

	while(len > 0) {
		XNextEvent(display, &ev);
		len--;
		switch(ev.type) {
		case FocusIn:
		case FocusOut:
			if(ev.xfocus.type == FocusOut) {
				/* Allow keyrepeat again! */
				vid_printf("Left window, auto repeat on\n");
				x_auto_repeat_on(0);
				g_has_focus = 0;
			} else if(ev.xfocus.type == FocusIn) {
				/* Allow keyrepeat again! */
				vid_printf("Enter window, auto repeat off\n");
				x_auto_repeat_off(0);
				g_has_focus = 1;
			}
			break;
		case EnterNotify:
		case LeaveNotify:
			/* These events are disabled now */
			printf("Enter/Leave event for winow %08x, sub: %08x\n",
				(word32)ev.xcrossing.window,
				(word32)ev.xcrossing.subwindow);
			printf("Enter/L mode: %08x, detail: %08x, type:%02x\n",
				ev.xcrossing.mode, ev.xcrossing.detail,
				ev.xcrossing.type);
			break;
		case ButtonPress:
			vid_printf("Got button press of button %d!\n",
				ev.xbutton.button);
			if(ev.xbutton.button == 1) {
				vid_printf("mouse button pressed\n");
				mouse_button = 1;
			} else if(ev.xbutton.button == 2) {
				g_limit_speed++;
				if(g_limit_speed > 2) {
					g_limit_speed = 0;
				}

				printf("Toggling g_limit_speed to %d\n",
					g_limit_speed);
				switch(g_limit_speed) {
				case 0:
					printf("...as fast as possible!\n");
					break;
				case 1:
					printf("...1.024MHz\n");
					break;
				case 2:
					printf("...2.5MHz\n");
					break;
				}
			} else {
				/* Re-enable kbd repeat for X */
				x_auto_repeat_on(0);
				set_halt(1);
				fflush(stdout);
			}
			break;
		case ButtonRelease:
			if(ev.xbutton.button == 1) {
				vid_printf("mouse button released\n");
				mouse_button = 0;
			}
			break;
		case Expose:
			g_full_refresh_needed = -1;
			break;
		case NoExpose:
			/* do nothing */
			break;
		case KeyPress:
		case KeyRelease:
			handle_keysym(&ev);
			break;
		case KeymapNotify:
			break;
		case ColormapNotify:
			vid_printf("ColormapNotify for %08x\n",
				(word32)(ev.xcolormap.window));
			vid_printf("colormap: %08x, new: %d, state: %d\n",
				(word32)ev.xcolormap.colormap,
				ev.xcolormap.new, ev.xcolormap.state);
			break;
		case MotionNotify:
			delta_x = 0;
			delta_y = 0;
			if(ev.xmotion.window == base_win) {
				delta_x = -BASE_MARGIN_LEFT;
				delta_y = -BASE_MARGIN_TOP;
			} else if(ev.xmotion.window == a2_win) {
				delta_x = 0;
				delta_y = 0;
			} else if(ev.xmotion.window == status_win) {
				delta_x = -BASE_MARGIN_LEFT;
				delta_y = X_A2_WINDOW_HEIGHT +
					BASE_MARGIN_BOTTOM;
			} else {
				printf("Motion in window %08x unknown!\n",
					(word32)ev.xmotion.window);
			}
			mouse_phys_x = delta_x + ev.xmotion.x;
			mouse_phys_y = delta_y + ev.xmotion.y;
			break;
		default:
			printf("X event 0x%08x is unknown!\n",
				ev.type);
			break;
		}
	}

	if(g_full_refresh_needed) {
		a2_screen_buffer_changed = -1;
		g_full_refresh_needed = -1;

		g_border_sides_refresh_needed = 1;
		g_border_special_refresh_needed = 1;

		/* x_refresh_ximage(); */
		/* redraw_border(); */
	}

}

void
handle_keysym(XEvent *xev_in)
{
	KeySym	keysym;
	int	keycode;
	int	type;
	int	is_up;
	word32	state;
	int	i;

	keycode = xev_in->xkey.keycode;
	type = xev_in->xkey.type;

	keysym = XLookupKeysym(&(xev_in->xkey), 0);

	state = xev_in->xkey.state;

	vid_printf("keycode: %d, type: %d, state:%d, sym: %08x\n",
		keycode, type, state, (word32)keysym);

	is_up = 0;
	if(type == KeyRelease) {
		is_up = 1;
	}

	if(keysym == XK_F10 && !is_up) {
		change_a2vid_palette((g_a2vid_palette + 1) & 0xf);
	}

	if(keysym == XK_F11 && !is_up) {
		g_swap_paddles = !g_swap_paddles;
		printf("Swap paddles is now: %d\n", g_swap_paddles);
	}
	if(keysym == XK_F12 && !is_up) {
		g_invert_paddles = !g_invert_paddles;
		printf("Invert paddles is now: %d\n", g_invert_paddles);
	}

	if(keysym == XK_Alt_L || keysym == XK_Meta_L) {
		g_alt_left_up = is_up;
	}

	if(keysym == XK_Alt_R || keysym == XK_Meta_R) {
		g_alt_right_up = is_up;
	}

	if(g_alt_left_up == 0 && g_alt_right_up == 0) {
		printf("Sending sound to file\n");
		g_send_sound_to_file = 1;
	} else {
		if(g_send_sound_to_file) {
			printf("Stopping sending sound to file\n");
			close_sound_file();
		}
		g_send_sound_to_file = 0;
	}

	/* first, do conversions */
	switch(keysym) {
	case XK_Alt_L:
	case XK_Meta_L:
	case XK_Menu:
		keysym = XK_Print;		/* option */
		break;
	case XK_Alt_R:
	case XK_Meta_R:
	case XK_Cancel:
		keysym = XK_Scroll_Lock;	/* cmd */
		break;
	case NoSymbol:
		switch(keycode) {
		case 0x0095:
			/* left windows key = option */
			keysym = XK_Print;
			break;
		case 0x0096:
		case 0x0094:
			/* right windows key = cmd */
			keysym = XK_Scroll_Lock;
			break;
		}
	}

	/* Look up Apple 2 keycode */
	for(i = g_num_a2_keycodes - 1; i >= 0; i--) {
		if((keysym == a2_key_to_xsym[i][1]) ||
					(keysym == a2_key_to_xsym[i][2])) {

			/* Special: handle shift, control multiple keys */
			/* Make user hitting Shift_L, Shift_R, and then */
			/*  releasing Shift_L work. */
			if(keysym == XK_Shift_L) {
				g_Shift_L_up = is_up;
				if(is_up && (g_Shift_R_up == 0)) {
					/* shift is still down, don't do it! */
					is_up = 0;
				}
			}
			if(keysym == XK_Shift_R) {
				g_Shift_R_up = is_up;
				if(is_up && (g_Shift_L_up == 0)) {
					/* shift is still down, don't do it! */
					is_up = 0;
				}
			}
			if(keysym == XK_Control_L) {
				g_Control_L_up = is_up;
				if(is_up && (g_Control_R_up == 0)) {
					/* shift is still down, don't do it! */
					is_up = 0;
				}
			}
			if(keysym == XK_Control_R) {
				g_Control_R_up = is_up;
				if(is_up && (g_Control_L_up == 0)) {
					/* shift is still down, don't do it! */
					is_up = 0;
				}
			}

			adb_physical_key_update(a2_key_to_xsym[i][0], is_up);
			return;
		}
	}

	printf("Keysym: %04x of keycode: %02x unknown\n", (word32)keysym,
		keycode);
}

void
x_auto_repeat_on(int must)
{
	if((g_auto_repeat_on <= 0) || must) {
		g_auto_repeat_on = 1;
		XAutoRepeatOn(display);
		XFlush(display);
		adb_kbd_repeat_off();
	}
}

void
x_auto_repeat_off(int must)
{
	if((g_auto_repeat_on != 0) || must) {
		XAutoRepeatOff(display);
		XFlush(display);
		g_auto_repeat_on = 0;
		adb_kbd_repeat_off();
	}
}
