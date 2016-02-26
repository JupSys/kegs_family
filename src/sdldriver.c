/************************************************************************/
/*			KEGS: Apple //gs Emulator			*/
/*			Copyright 2002 by Kent Dickey			*/
/*									*/
/*		This code is covered by the GNU GPL			*/
/*									*/
/*	The KEGS web page is kegs.sourceforge.net			*/
/*	You may contact the author at: kadickey@alumni.princeton.edu	*/
/************************************************************************/

const char rcsid_xdriver_c[] = "@(#)$KmKId: xdriver.c,v 1.187 2004-11-15 16:24:19-05 kentd Exp $";

#include <SDL2/SDL.h>

void sdl_init_keycodes(void);
void sdl_handle_keysym(const SDL_KeyboardEvent *key);

#include <stdlib.h>
#include <signal.h>

#include "defc.h"

#define FONT_NAME_STATUS	"8x13"

extern int Verbose;

int     g_use_shmem = 0;

extern int g_warp_pointer;
extern int g_screen_depth;
extern int g_force_depth;
int g_screen_mdepth = 0;

extern int _Xdebug;

extern int g_send_sound_to_file;

extern int g_quit_sim_now;

int	g_has_focus = 0;
int	g_auto_repeat_on = -1;
int	g_x_shift_control_state = 0;


int	g_needs_cmap = 0;

extern word32 g_red_mask;
extern word32 g_green_mask;
extern word32 g_blue_mask;
extern int g_red_left_shift;
extern int g_green_left_shift;
extern int g_blue_left_shift;
extern int g_red_right_shift;
extern int g_green_right_shift;
extern int g_blue_right_shift;

extern Kimage g_mainwin_kimage;

extern int Max_color_size;


extern word32 g_palette_8to1624[256];
extern word32 g_a2palette_8to1624[256];

int	g_alt_left_up = 1;
int	g_alt_right_up = 1;

extern word32 g_full_refresh_needed;

extern int g_border_sides_refresh_needed;
extern int g_border_special_refresh_needed;
extern int g_status_refresh_needed;

extern int g_lores_colors[];
extern int g_cur_a2_stat;

extern int g_a2vid_palette;

extern int g_installed_full_superhires_colormap;

extern int g_screen_redraw_skip_amt;

extern word32 g_a2_screen_buffer_changed;

extern char *g_status_ptrs[MAX_STATUS_LINES];


SDL_Window *g_main_window=NULL;
SDL_Renderer *     sdlRenderer=NULL;



int	g_num_a2_keycodes = 0;


int
main(int argc, char **argv)
{
	return kegsmain(argc, argv);
}

void
x_dialog_create_kegs_conf(const char *str)
{
	/* do nothing -- not implemented yet */
	return;
}

int
x_show_alert(int is_fatal, const char *str)
{
	/* Not implemented yet */
	adb_all_keys_up();

	clear_fatal_logs();
	return 0;
}


#define MAKE_2(val)	( (val << 8) + val)

void
x_update_color(int col_num, int red, int green, int blue, word32 rgb)
{
//fprintf(stderr,"x_update_color\n");
}


void
x_update_physical_colormap()
{
fprintf(stderr,"x_update_physical_colormap\n");
}

void
show_xcolor_array()
{
	int i;

	for(i = 0; i < 256; i++) {
		printf("%02x: %08x\n", i, g_palette_8to1624[i]);
			
#if 0
		printf("%02x: %04x %04x %04x, %02x %x\n",
			i, xcolor_array[i].red, xcolor_array[i].green,
			xcolor_array[i].blue, (word32)xcolor_array[i].pixel,
			xcolor_array[i].flags);
#endif
	}
}



void
xdriver_end()
{

	printf("xdriver_end\n");
		x_auto_repeat_on(1);
}


void
x_badpipe(int signum)
{
	/* restore normal sigpipe handling */
	signal(SIGPIPE, SIG_DFL);

	/* attempt to xset r */
//	int result = system("xset r");
	my_exit(5);
}

void
dev_video_init()
{
fprintf(stderr,"AJS dev_video_init\n");
/* let's open the window and create the SDL context */

sdl_init_keycodes();


       int width  = X_A2_WINDOW_WIDTH;
       int height= X_A2_WINDOW_HEIGHT+ MAX_STATUS_LINES*16 + 8;


SDL_InitSubSystem(SDL_INIT_VIDEO);

g_screen_depth=32;
g_screen_mdepth=32;
    g_main_window = SDL_CreateWindow( "KEGS Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN  );
    sdlRenderer= SDL_CreateRenderer(g_main_window, -1, 0);



        int     lores_col;
        int     i;

        printf("Preparing graphics system\n");

        video_get_kimages();

        if(g_screen_depth != 8) {
                // Get g_mainwin_kimage
                video_get_kimage(&g_mainwin_kimage, 0, g_screen_depth,
                                                        g_screen_mdepth);
        }

        for(i = 0; i < 256; i++) {
                lores_col = g_lores_colors[i & 0xf];
                video_update_color_raw(i, lores_col);
                g_a2palette_8to1624[i] = g_palette_8to1624[i];
        }

        g_installed_full_superhires_colormap = 1;

        fflush(stdout);

fprintf(stderr,"Finished with video init\n");
}

void
x_get_kimage(Kimage *kimage_ptr) {
	int	width;
	int	height;
	int	depth;
	int	mdepth;

	width = kimage_ptr->width_req;
	height = kimage_ptr->height;
	depth = kimage_ptr->depth;
	mdepth = kimage_ptr->mdepth;

fprintf(stderr,"x_get_kimage %d %d %d %d\n",width,height,depth,mdepth);


        // create the texture that we will have the pixels draw into
       SDL_Surface *theSurface = SDL_CreateRGBSurface(0, width, height, mdepth,  0, 0, 0, 0);
       kimage_ptr->dev_handle = theSurface;
       int size = (width*height*mdepth) >> 3;
       byte *ptr = (byte *)malloc(size);
       kimage_ptr->data_ptr = ptr;



}


void
get_ximage(Kimage *kimage_ptr)
{
fprintf(stderr,"get_ximage\n");
}

void sdl_draw_string(SDL_Surface *surf, Sint16 x, Sint16 y, const unsigned char *string, Uint16 maxstrlen, Uint16 xscale, Uint16 yscale, Uint8 fg, Uint8 bg);

void
x_redraw_status_lines()
{
fprintf(stderr,"x_redraw_status_lines\n");
#if 0
    char *buf;
    int line;
    Sint16 x,y;
   

       y = A2_WINDOW_HEIGHT+8;
        x = 8;
	for(line = 0; line < MAX_STATUS_LINES; line++, y+=8) {
		buf = &(g_video_status_buf[line][0]);
        sdl_draw_string(screen, x, y, buf, X_LINE_LENGTH, 1, 1, 0xef, 0xe0);
	}
        
        /*sdl_update_dirty_rects(gRectTable, screen);*/
	 g_sdl_need_refresh = 1;
#endif

}

#if 0
void
sdl_draw_string(SDL_Surface *surf, Sint16 x, Sint16 y, const unsigned char *string, Uint16 maxstrlen, Uint16 xscale, Uint16 yscale, Uint8 fg, Uint8 bg)
{
    int strlen;
    SDL_Surface *linesurf;
    Sint16 ypixel;
    Uint8 * yptr;
    int col, bit;
    byte b;
    SDL_Rect  srcrect, dstrect;
    int xrepeat, yrepeat;

    assert(string!=NULL);
    for(strlen = 0; strlen<maxstrlen && string[strlen]; strlen++) {}
    srcrect.x = srcrect.y = 0;
    srcrect.w = strlen * 7 * xscale;
    srcrect.h = 8 * yscale;
    linesurf = SDL_CreateRGBSurface(SDL_SWSURFACE, srcrect.w, srcrect.h, 8, 0, 0, 0, 0);
    //SDL_SetPalette(linesurf, graymap, 0, 256);
    SDL_SetPaletteColors(linesurf->format->palette, graymap, 0, 256);
    //SDL_LockSurface(linesurf);
    yptr = linesurf->pixels;
    for(ypixel = 0; ypixel<8; ypixel++) {
        for(col=0; col<strlen; col++) {
            b = font_array[string[col]^0x80][ypixel];
            for(bit=0; bit<7; bit++, yptr++) {
                *yptr = (b & (1<<(7-bit))) ? fg : bg;
                for(xrepeat = 1; xrepeat < xscale; xrepeat++, yptr++) {
                    yptr[1] = *yptr;
                }
            }
        }
        yptr += linesurf->pitch - srcrect.w;
        for(yrepeat = 1; yrepeat < yscale; yrepeat++) {
            for(xrepeat = 0; xrepeat<srcrect.w; xrepeat++, yptr++)
                *yptr = yptr[-linesurf->pitch];
            yptr += linesurf->pitch - srcrect.w;
        }
    }
    //SDL_UnlockSurface(linesurf);
    dstrect.x = x;
    dstrect.y = y;
    SDL_BlitSurface(linesurf, &srcrect, surf, &dstrect);
    SDL_FreeSurface(linesurf);
    //sdl_add_dirty_rect(gRectTable, dstrect);
    
    SDL_RenderPresent(sdlRenderer);
    /*SDL_UpdateRect(surf, dstrect.x, dstrect.y, dstrect.w, dstrect.h);*/
}
#endif



void
x_push_kimage(Kimage *kimage_ptr, int destx, int desty, int srcx, int srcy,
	int width, int height)
{
//fprintf(stderr,"x_push_kimage (%p) destx %d desty %d srcx %d srcy %d width %d height %d\n",
 //        kimage_ptr,destx, desty, srcx, srcy, width, height);

	int	kwidth;
	int	kheight;
	//int	kdepth;
	int	kmdepth;

	kwidth = kimage_ptr->width_req;
	kheight = kimage_ptr->height;
	//kdepth = kimage_ptr->depth;
	kmdepth = kimage_ptr->mdepth;

       int size = (kwidth*kheight*kmdepth) >> 3;
   // fprintf(stderr,"CopyOffScreenToWindow %p\n",theActiveBits);
    SDL_Surface * theSurface = kimage_ptr->dev_handle;
    SDL_LockSurface(theSurface);
    //int pixel=0; for (int i=0;i<16;i++) for (int j=0;j<16;j++) theActiveBits[pixel++]=i;
//fprintf(stderr,"theSurface->w %d  theSurface->h %d\n",theSurface->w,theSurface->h);

    memcpy(theSurface->pixels,kimage_ptr->data_ptr,size);

    SDL_Texture *sdlTexture = SDL_CreateTextureFromSurface(sdlRenderer,theSurface);
    //SDL_RenderClear(sdlRenderer);
SDL_Rect srcrect;
SDL_Rect dstrect;
srcrect.x=srcx;
srcrect.y=srcy;
srcrect.w=width;
srcrect.h=height;
dstrect.x=destx;
dstrect.y=desty;
dstrect.w=width;
dstrect.h=height;

/*
srcrect.w=theSurface->w;
srcrect.h=theSurface->h;
dstrect.w=theSurface->w;
dstrect.h=theSurface->h;
*/

//fprintf(stderr,"Copy\n");
    SDL_RenderCopy(sdlRenderer, sdlTexture, &srcrect,&dstrect);
    SDL_RenderPresent(sdlRenderer);
    SDL_DestroyTexture(sdlTexture);
    SDL_UnlockSurface(theSurface);
}



void
x_push_done()
{
fprintf(stderr,"x_push_done\n");
}


#define KEYBUFLEN	128

int g_num_check_input_calls = 0;
int g_check_input_flush_rate = 2;

int
x_update_mouse(int raw_x, int raw_y, int button_states, int buttons_valid)
{
fprintf(stderr,"x_update_mouse\n");
return 0;
}
int
old_ajs_x_update_mouse(int raw_x, int raw_y, int button_states, int buttons_valid)
{
	int	x, y;

	x = raw_x - BASE_MARGIN_LEFT;
	y = raw_y - BASE_MARGIN_TOP;

	if(g_warp_pointer && (x == A2_WINDOW_WIDTH/2) &&
			(y == A2_WINDOW_HEIGHT/2) && (buttons_valid == 0) ) {
		/* tell adb routs to recenter but ignore this motion */
		update_mouse(x, y, 0, -1);
		return 0;
	}
	return update_mouse(x, y, button_states, buttons_valid & 7);
}

void
check_input_events()
{
//fprintf(stderr,"check input events\n");
    SDL_Event event; /* Event structure */
        int     motion;
        int     refresh_needed;

        motion = 0;
        refresh_needed = 0;

    /* Check for events */
    while(SDL_PollEvent(&event)){  /* Loop until there are no events left on the queue */
        //ki_printf("got event %d\n",event.type);
        switch(event.type){  /* Process the appropiate event type */
        case SDL_QUIT:
            exit(0);
            break;
        case SDL_WINDOWEVENT:

                if(event.window.event== SDL_WINDOWEVENT_EXPOSED) {
                        refresh_needed = -1;
                }
                else if(event.window.event== SDL_WINDOWEVENT_FOCUS_GAINED) {
                    /* Allow keyrepeat again! */
                    vid_printf("Left window, auto repeat on\n");
                    //video_auto_repeat_on_sdl(0);
                }
                else if (event.window.event== SDL_WINDOWEVENT_FOCUS_LOST){
                    /* Allow keyrepeat again! */
                    vid_printf("Enter window, auto repeat off\n");
                    //video_auto_repeat_off_sdl(0);
                }
            break;
        case SDL_MOUSEBUTTONDOWN:
                        vid_printf("Got button press of button %d!\n",
                                event.button.button);
                        if(event.button.button == 1) {
                                vid_printf("mouse button pressed\n");
                                motion = update_mouse(event.button.x,
                                                        event.button.y, 1, 1);
                        } else if(event.button.button == 2) {
                //function_execute(func_button2, event.button.x, event.button.y);
                        } else if(event.button.button == 3) {
                //function_execute(func_button3, event.button.x, event.button.y);
                        }
                        break;
        case SDL_MOUSEBUTTONUP:
                        if(event.button.button == 1) {
                                vid_printf("mouse button released\n");
                                motion = update_mouse(event.button.x,
                                                        event.button.y, 0, 1);
                        }
                        break;
        //case SDL_VIDEOEXPOSE:
        //              refresh_needed = -1;
         //   break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sdl_handle_keysym(&(event.key));
            break;
        case SDL_MOUSEMOTION:
                        motion = update_mouse(event.motion.x, event.motion.y, 0, 0);
            break;
        default: /* Report an unhandled event */
            //ki_printf("I don't know what this event %d is!\n",event.type);
            break;
        }
    }
        if(motion && g_warp_pointer) {
                //SDL_WarpMouse(X_A2_WINDOW_WIDTH/2, X_A2_WINDOW_HEIGHT/2);
                update_mouse(-1,-1,-1,-1);
        }

        if(refresh_needed) {
                fprintf(stderr,"Full refresh needed\n");
                //a2_screen_buffer_changed = -1;
                g_full_refresh_needed = -1;

                g_border_sides_refresh_needed = 1;
                g_border_special_refresh_needed = 1;
                g_status_refresh_needed = 1;

                /* x_refresh_ximage(); */
                /* redraw_border(); */
        }

}


void
x_hide_pointer(int do_hide)
{
fprintf(stderr,"x_hide_pointer\n");

}

void
x_auto_repeat_on(int must)
{
	if((g_auto_repeat_on <= 0) || must) {
		g_auto_repeat_on = 1;
		adb_kbd_repeat_off();
	}
}

void
x_auto_repeat_off(int must)
{
	if((g_auto_repeat_on != 0) || must) {
		g_auto_repeat_on = 0;
		adb_kbd_repeat_off();
	}
}

void
x_full_screen(int do_full)
{
	return;
}

/* ----- */
/*  AJS */
/* 00000 */

#define A2KEY_TAB 0x30
#define A2KEY_SPACE 0x31
#define A2KEY_BACKQUOTE 0x32
#define A2KEY_DELETE 0x33
#define A2KEY_ESCAPE 0x35
#define A2KEY_RCTRL 0x36
#define A2KEY_COMMAND 0x37
#define A2KEY_SHIFT 0x38
#define A2KEY_CAPSLOCK 0x39
#define A2KEY_OPTION 0x3a
#define A2KEY_LCTRL 0x3b
#define A2KEY_F1 0x7a
#ifdef APPLE_EXTENDED_KEYBOARD
#define A2KEY_F2 0x78
#else
#define A2KEY_F2 0x7b
#endif
#define A2KEY_F3 0x63
#define A2KEY_F4 0x76
#define A2KEY_F5 0x60
#define A2KEY_F6 0x61
#define A2KEY_F7 0x62
#define A2KEY_F8 0x64
#define A2KEY_F9 0x65
#define A2KEY_F10 0x6D
#define A2KEY_F11 0x67
#define A2KEY_F12 0x6F
#define A2KEY_F13 0x69
#define A2KEY_F14 0x6b
#define A2KEY_F15 0x71
#define A2KEY_INSERTHELP 0x72
#define A2KEY_HOME 0x73
#define A2KEY_PAGEUP 0x74
#define A2KEY_FWDDEL 0x75
#define A2KEY_END 0x77
#define A2KEY_PAGEDOWN 0x79
#ifdef APPLE_EXTENDED_KEYBOARD
#define A2KEY_LEFT 0x7b
#define A2KEY_RIGHT 0x7c
#define A2KEY_DOWN 0x7d
#define A2KEY_UP 0x7e
#define A2KEY_LCTRL 0x3b
#else
#define A2KEY_LEFT 0x3b
#define A2KEY_RIGHT 0x3c
#define A2KEY_DOWN 0x3d
#define A2KEY_UP 0x3e
#endif

#define A2KEY_RESET 0x7f
#define A2KEY_MASK 0x7f




static void process_keysym(SDL_Keycode, int);

static const int a2key_to_sdlksym[][3] = {
	{ A2KEY_ESCAPE,	SDL_SCANCODE_ESCAPE,	0 },
	{ A2KEY_F1,	SDL_SCANCODE_F1,	0 },
	{ A2KEY_F2,	SDL_SCANCODE_F2,	0 },
	{ A2KEY_F3,	SDL_SCANCODE_F3,	0 },
	{ A2KEY_F4,	SDL_SCANCODE_F4,	0 },
	{ A2KEY_F5,	SDL_SCANCODE_F5,	0 },
	{ A2KEY_F6,	SDL_SCANCODE_F6,	0 },
	{ A2KEY_F7,	SDL_SCANCODE_F7,	0 },
	{ A2KEY_F8,	SDL_SCANCODE_F8,	0 },
	{ A2KEY_F9,	SDL_SCANCODE_F9,	0 },
	{ A2KEY_F10,	SDL_SCANCODE_F10,	0 },
	{ A2KEY_F11,	SDL_SCANCODE_F11,	0 },
	{ A2KEY_F12,	SDL_SCANCODE_F12,	0 },
	{ A2KEY_F13,	SDL_SCANCODE_F13,	0 },
	{ A2KEY_F14,	SDL_SCANCODE_F14,	0 },
	{ A2KEY_F15,	SDL_SCANCODE_F15,	0 },
	{ A2KEY_RESET, SDL_SCANCODE_PAUSE, 0}, /* Reset */
	{ A2KEY_BACKQUOTE,	'`', '~' },		/* Key number 18? */
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
	{ A2KEY_DELETE,	SDL_SCANCODE_BACKSPACE, 0 }, /* Delete */
	{ A2KEY_INSERTHELP,	SDL_SCANCODE_INSERT, SDL_SCANCODE_HELP },	/* Help, insert */
#if 0
	{ A2KEY_HOME,	SDL_SCANCODE_HOME, 0 },		/* Home */
#endif
	{ A2KEY_PAGEUP,	SDL_SCANCODE_PAGEUP, 0 },	/* Page up */
	{ 0x47,	SDL_SCANCODE_NUMLOCKCLEAR, SDL_SCANCODE_CLEAR },	/* keypad Clear */
	{ 0x51,	SDL_SCANCODE_KP_EQUALS, SDL_SCANCODE_HOME },		/* Note SDL_SCANCODE_HOME alias! */
	{ 0x4b,	SDL_SCANCODE_KP_DIVIDE, 0 },
	{ 0x43,	SDL_SCANCODE_KP_MULTIPLY, 0 },

	{ A2KEY_TAB,	SDL_SCANCODE_TAB, 0 },
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
	{ 0x2a,	SDL_SCANCODE_BACKSLASH, '|' },	/* \, | */
	{ A2KEY_FWDDEL,	SDL_SCANCODE_DELETE, 0 },   /* keypad delete */
	{ A2KEY_END,	SDL_SCANCODE_END, 0 },	/* keypad end */
	{ A2KEY_PAGEDOWN,	SDL_SCANCODE_PAGEDOWN, 0 },	/* keypad page down */
	{ 0x59,	SDL_SCANCODE_KP_7, 0 },	/* keypad 7 */
	{ 0x5b,	SDL_SCANCODE_KP_8, 0 },	/* keypad 8 */
	{ 0x5c,	SDL_SCANCODE_KP_9, 0 },	/* keypad 9 */
	{ 0x4e,	SDL_SCANCODE_KP_MINUS, 0 },	/* keypad - */

	{ A2KEY_CAPSLOCK,	SDL_SCANCODE_CAPSLOCK, 0 },
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
	{ 0x24,	SDL_SCANCODE_RETURN, 0 },	/* return */
	{ 0x56,	SDL_SCANCODE_KP_4, 0 },	/* keypad 4 */
	{ 0x57,	SDL_SCANCODE_KP_5, 0 },	/* keypad 5 */
	{ 0x58,	SDL_SCANCODE_KP_6, 0 },	/* keypad 6 */
	{ 0x45,	SDL_SCANCODE_KP_PLUS, 0 },	/* keypad + */

	{ A2KEY_SHIFT,	SDL_SCANCODE_LSHIFT, SDL_SCANCODE_RSHIFT },	/* shift */
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
	{ A2KEY_UP,	SDL_SCANCODE_UP, 0 },	/* up arrow */
	{ 0x53,	SDL_SCANCODE_KP_1, 0 },	/* keypad 1 */
	{ 0x54,	SDL_SCANCODE_KP_2, 0 },	/* keypad 2 */
	{ 0x55,	SDL_SCANCODE_KP_3, 0 },	/* keypad 3 */

#ifdef APPLE_EXTENDED_KEYBOARD
	{ A2KEY_LCTRL,	SDL_SCANCODE_LCTRL,	0},	/* control */
	{ A2KEY_RCTRL,	SDL_SCANCODE_RCTRL,	0},	/* control */
#else
	{ A2KEY_RCTRL,	SDL_SCANCODE_LCTRL, SDL_SCANCODE_RCTRL },	/* control */
#endif

#if 0
	{ A2KEY_OPTION,	SDL_SCANCODE_PRINT, SDL_SCANCODE_SYSREQ },		/* Option */
	{ A2KEY_COMMAND,	SDL_SCANCODE_SCROLLOCK, 0 },		/* Command */
#else
	{ A2KEY_OPTION,	SDL_SCANCODE_LGUI, SDL_SCANCODE_RGUI},		/* Option.. add RSUPER and RSUPER? */
	{ A2KEY_COMMAND,	SDL_SCANCODE_LALT, SDL_SCANCODE_RALT },		/* Command/Open apple */
#endif
	{ A2KEY_SPACE,	SDL_SCANCODE_SPACE, 0 },
	{ A2KEY_LEFT,	SDL_SCANCODE_LEFT, 0 },	/* left */
	{ A2KEY_DOWN,	SDL_SCANCODE_DOWN, 0 },	/* down */
	{ A2KEY_RIGHT,	SDL_SCANCODE_RIGHT, 0 },	/* right */
	{ 0x52,	SDL_SCANCODE_KP_0, 0 },	/* keypad 0 */
	{ 0x41,	SDL_SCANCODE_KP_PERIOD, 0 },	/* keypad . */
	{ 0x4c,	SDL_SCANCODE_KP_ENTER, 0 },	/* keypad enter */
	{ -1, -1, -1 }
};

static int sdlksym_to_a2key[SDL_NUM_SCANCODES];


#if 0
static void
sdl_update_modifier_state(SDLMod state)
{
	SDLMod	state_xor;
	int	is_up;

	state = state & (KMOD_CTRL | KMOD_CAPS | KMOD_SHIFT);
	state_xor = g_mod_state ^ state;
	is_up = 0;
	if(state_xor & KMOD_CTRL) {
		is_up = ((state & KMOD_CTRL) == 0);
		adb_physical_key_update(0x36, is_up);
	}
	if(state_xor & KMOD_CAPS) {
		is_up = ((state & KMOD_CAPS) == 0);
		adb_physical_key_update(0x39, is_up);
	}
	if(state_xor & KMOD_SHIFT) {
		is_up = ((state & KMOD_SHIFT) == 0);
		adb_physical_key_update(0x38, is_up);
	}

	g_mod_state = state;
}
#endif

#if 0 /* AJS */
void
x_hide_pointer(int do_hide)
{
        if(do_hide) {
        	SDL_ShowCursor(SDL_ENABLE);
        } else {
        	SDL_ShowCursor(SDL_DISABLE);
        }
}

void
video_warp_pointer_sdl(void)
{
    if(g_fullscreen)
        g_warp_pointer |= 2;
    else
        g_warp_pointer &= ~2;
    if(g_warp_pointer) {
SDL_SetRelativeMouseMode(SDL_TRUE);
        //SDL_ShowCursor(SDL_DISABLE);
        //SDL_WM_GrabInput(SDL_GRAB_ON);
        //SDL_WarpMouse(X_A2_WINDOW_WIDTH/2,X_A2_WINDOW_HEIGHT/2);
	if (g_fullscreen) {	/* update screen to remove cursor shadow */
	    //SDL_UpdateRect(screen,0,0,0,0);
            SDL_RenderPresent(sdlRenderer);
	}
        ki_printf("Mouse Pointer grabbed\n");
    } else {
        SDL_ShowCursor(SDL_ENABLE);
        //SDL_WM_GrabInput(SDL_GRAB_OFF);
        ki_printf("Mouse Pointer released\n");
    }
}
#endif

void 
sdl_handle_keysym(const SDL_KeyboardEvent *key)
{
    SDL_Keysym keysym = key->keysym;
    const Uint8 type = key->type;
    const Uint8 state = key->state;
    int is_up;

	vid_printf("type: %d, state:%d, sym: %08x\n",
		type, state, keysym.sym);

    /* Check the state for caps lock,shift and control */
//    sdl_update_modifier_state(keysym.mod);

	is_up = 0;
    if( type == SDL_KEYUP ) {
      is_up = 1;
    }

#if 0
    if(keysym.sym == SDL_SCANCODE_F1) {
      /* Alias F1 to escape for OS/2 */
      keysym.sym = SDL_SCANCODE_ESCAPE;
    }
#endif
    /* ctrl-apple-tab = kegs configuration menu */
    if((keysym.sym == SDL_SCANCODE_TAB) && !is_up &&
       ((keysym.mod & KMOD_LCTRL) || (keysym.mod & KMOD_RCTRL)) &&
       ((keysym.mod & KMOD_LALT) || (keysym.mod & KMOD_RALT))) {
        //ki_printf("Configuration menu!\n");
        //configuration_menu_sdl();
fprintf(stderr,"config menu??\n");
        /*adb_init();*/
        adb_physical_key_update(A2KEY_TAB, 1);
        adb_physical_key_update(A2KEY_COMMAND, 1);
        adb_physical_key_update(A2KEY_RCTRL, 1);
    }
#if 0
    else if((keysym.sym == SDL_SCANCODE_F6) && !is_up) {
	if(function_execute(func_f6,-1,-1))
	    return;
    }
    else if((keysym.sym == SDL_SCANCODE_F7) && !is_up) {
	if(function_execute(func_f7,-1,-1))
	    return;
    }
    else if((keysym.sym == SDL_SCANCODE_F8) && !is_up) {
	if(function_execute(func_f8,-1,-1))
	    return;
    }
    else if((keysym.sym == SDL_SCANCODE_F9) && !is_up) {
	if(function_execute(func_f9,-1,-1))
	    return;
    }
    else if((keysym.sym == SDL_SCANCODE_F10) && !is_up) {
	if(function_execute(func_f10,-1,-1))
	    return;
    }
    else if((keysym.sym == SDL_SCANCODE_F11) && !is_up) {
	if(function_execute(func_f11,-1,-1))
	    return;
    }
    else if((keysym.sym == SDL_SCANCODE_F12) && !is_up) {
	if(function_execute(func_f12,-1,-1))
	    return;
    }
#endif
    /* ctrl-delete = Mac OS X reset  la Bernie ! */
    else if((keysym.sym == SDL_SCANCODE_BACKSPACE) && !is_up &&
	    ((keysym.mod & KMOD_LCTRL) || (keysym.mod & KMOD_RCTRL))) {
	fprintf(stderr,"Reset pressed !! \n");
        
	keysym.sym = SDL_SCANCODE_PAUSE;
    }
    process_keysym(keysym.sym, is_up);
}

void
process_keysym(SDL_Keycode sym, int is_up)
{
    int	a2code;
    
    a2code = sdlksym_to_a2key[sym];
      
    if (a2code >= 0) {
        adb_physical_key_update(a2code, is_up);
        /*ki_printf("keysym %x -> a2code %x\n",keysym.sym,a2code);*/
	} 
    else {
	if((sym >= SDL_SCANCODE_F6) && (sym <= SDL_SCANCODE_F12)) {
	    /* just get out quietly all FKeys */
	    return;
	}
	fprintf(stderr,"Keysym: %04x unknown\n",
               sym);
    }
}

void
sdl_init_keycodes(void)
{
    int keycode;

    for(keycode = 0; keycode<SDL_NUM_SCANCODES; keycode++) {
        sdlksym_to_a2key[keycode] = -1;
    }
    for(keycode=0; a2key_to_sdlksym[keycode][0] != -1; keycode++) {
        if (a2key_to_sdlksym[keycode][1] != 0)
            sdlksym_to_a2key[a2key_to_sdlksym[keycode][1]] =
                a2key_to_sdlksym[keycode][0];
        if (a2key_to_sdlksym[keycode][2] != 0)
            sdlksym_to_a2key[a2key_to_sdlksym[keycode][2]] =
                a2key_to_sdlksym[keycode][0];
    }
    /* add more aliases */
    /* option key */
    sdlksym_to_a2key[SDL_SCANCODE_LGUI] = sdlksym_to_a2key[SDL_SCANCODE_RGUI] = 0x3a;
    sdlksym_to_a2key[SDL_SCANCODE_MODE] = sdlksym_to_a2key[SDL_SCANCODE_APPLICATION] = 0x3a;
    /* reset on the mac keyboard */
    sdlksym_to_a2key[SDL_SCANCODE_POWER] = 0x7f;
#if 0
    for(keycode=0; keycode<SDL_SCANCODE_LAST; keycode++)
        if(sdlksym_to_a2key[keycode] != -1)
            ki_printf("%x->%x ",keycode,sdlksym_to_a2key[keycode]);
    ki_printf("\n");
#endif

    g_num_a2_keycodes = keycode;
}


#if 0
void
video_check_input_events_sdl()
{
    SDL_Event event; /* Event structure */
	int	motion;
	int	refresh_needed;

	motion = 0;
	refresh_needed = 0;

    /* Check for events */
    while(SDL_PollEvent(&event)){  /* Loop until there are no events left on the queue */
        //ki_printf("got event %d\n",event.type);
        switch(event.type){  /* Process the appropiate event type */
        case SDL_QUIT:
            set_halt(HALT_WANTTOQUIT);
            break;
        case SDL_WINDOWEVENT:
 
		if(event.window.event== SDL_WINDOWEVENT_EXPOSED) {
			refresh_needed = -1;
                }
                else if(event.window.event== SDL_WINDOWEVENT_FOCUS_GAINED) {
                    /* Allow keyrepeat again! */
                    vid_printf("Left window, auto repeat on\n");
                    video_auto_repeat_on_sdl(0);
                }
                else if (event.window.event== SDL_WINDOWEVENT_FOCUS_LOST){
                    /* Allow keyrepeat again! */
                    vid_printf("Enter window, auto repeat off\n");
                    video_auto_repeat_off_sdl(0);
                }
            break;
        case SDL_MOUSEBUTTONDOWN:
			vid_printf("Got button press of button %d!\n",
				event.button.button);
			if(event.button.button == 1) {
				vid_printf("mouse button pressed\n");
				motion = update_mouse(event.button.x,
							event.button.y, 1, 1);
			} else if(event.button.button == 2) {
                function_execute(func_button2, event.button.x, event.button.y);
			} else if(event.button.button == 3) {
                function_execute(func_button3, event.button.x, event.button.y);
			}
			break;
        case SDL_MOUSEBUTTONUP:
			if(event.button.button == 1) {
				vid_printf("mouse button released\n");
				motion = update_mouse(event.button.x,
							event.button.y, 0, 1);
			}
			break;
        //case SDL_VIDEOEXPOSE:
	//		refresh_needed = -1;
         //   break;
        case SDL_KEYDOWN:
        case SDL_KEYUP:
            sdl_handle_keysym(&(event.key));
            break;
        case SDL_MOUSEMOTION:
			motion = update_mouse(event.motion.x, event.motion.y, 0, 0);
            break;
        default: /* Report an unhandled event */
            ki_printf("I don't know what this event %d is!\n",event.type);
            break;
        }
    }

	if(motion && g_warp_pointer) {
		//SDL_WarpMouse(X_A2_WINDOW_WIDTH/2, X_A2_WINDOW_HEIGHT/2);
		update_mouse(-1,-1,-1,-1);
	}

	if(refresh_needed) {
		ki_printf("Full refresh needed\n");
		a2_screen_buffer_changed = -1;
		g_full_refresh_needed = -1;

		g_border_sides_refresh_needed = 1;
		g_border_special_refresh_needed = 1;
		g_status_refresh_needed = 1;

		/* x_refresh_ximage(); */
		/* redraw_border(); */
	}
}
#endif

