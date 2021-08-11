/****************************************************************/
/*    Apple IIgs emulator                                       */
/*    Copyright 1996 Kent Dickey                                */
/*                                                              */
/*    This code may not be used in a commercial product         */
/*    without prior written permission of the author.           */
/*                                                              */
/*    You may freely distribute this code.                      */ 
/*                                                              */
/****************************************************************/

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <winsock.h>
#if 0
#include <ddraw.h>
#endif
#include <conio.h>
#include "defc.h"
#include "protos_windriver.h"

extern int Verbose;
extern int lores_colors[];
extern int hires_colors[];
extern word32 g_full_refresh_needed;
extern word32 a2_screen_buffer_changed;
extern int g_limit_speed;
extern int g_border_sides_refresh_needed;
extern int g_border_special_refresh_needed;
extern byte *a2_line_ptr[];
extern void *a2_line_xim[];
extern int a2_line_stat[];
extern int a2_line_left_edge[];
extern int a2_line_right_edge[];
extern int a2_line_full_left_edge[];
extern int a2_line_full_right_edge[];
extern int g_status_refresh_needed;
extern int g_installed_full_superhires_colormap;
extern int g_a2vid_palette;
extern int g_fast_disk_emul;
extern int g_warp_pointer;
extern int g_cur_a2_stat;

int g_use_shmem=0;
int	g_x_shift_control_state = 0;
word32 g_cycs_in_xredraw = 0;
word32 g_refresh_bytes_xfer = 0;
int	g_num_a2_keycodes = 0;

byte *data_text[2];
byte *data_hires[2];
byte *data_superhires;
byte *data_border_special;
byte *data_border_sides;

byte *dint_border_sides;
byte *dint_border_special;
byte *dint_main_win;

#define MAX_STATUS_LINES	7
#define X_LINE_LENGTH		88
DWORD   dwchary;
char	g_status_buf[MAX_STATUS_LINES][X_LINE_LENGTH + 1];

// Windows specific stuff
HWND hwndMain;
HDC  hcdc;
HDC hdcRef;
RGBQUAD lores_palette[256];
RGBQUAD a2v_palette[256];
RGBQUAD shires_palette[256];
RGBQUAD *win_palette;
RGBQUAD dummy_color;
HBITMAP ximage_text[2];
HBITMAP ximage_hires[2];
HBITMAP ximage_superhires;
HBITMAP ximage_border_special;
HBITMAP ximage_border_sides;

#if 0
LPDIRECTDRAW lpDD=NULL;
LPDIRECTDRAWSURFACE primsurf=NULL;
LPDIRECTDRAWSURFACE backsurf=NULL; 
LPDIRECTDRAWCLIPPER lpClipper=NULL; 
#endif

int a2_key_to_wsym[][3] = {
    { 0x35,    VK_ESCAPE,    VK_F1 },
//    { 0x7a,    VK_F1,    0 },
//    { 0x7b,    VK_F2,    0 },
//    { 0x63,    VK_F3,    0 },
//    { 0x76,    VK_F4,    0 },
//    { 0x60,    VK_F5,    0 },
//    { 0x61,    VK_F6,    0 },
    { 0x7f,    VK_CANCEL, 0 },
    { 0x32,    0xc0, 0 },        /* Key number 18? */
    { 0x12,    '1', 0 },
    { 0x13,    '2', 0 },
    { 0x14,    '3', 0 },
    { 0x15,    '4', 0 },
    { 0x17,    '5', 0 },
    { 0x16,    '6', 0 },
    { 0x1a,    '7', 0 },
    { 0x1c,    '8', 0 },
    { 0x19,    '9', 0 },
    { 0x1d,    '0', 0 },
    { 0x1b,    0xbd, 0 },    /* Key minus */
    { 0x18,    0xbb, 0 },   /* Key equals */
    { 0x33,    VK_BACK, 0 },
//    { 0x72,    VK_INSERT, 0 },        /* Help? */
//    { 0x74,    VK_PRIOR, 0 },
//    { 0x47,    VK_NUMLOCK, 0 },    /* Clear */
    { 0x51,    VK_HOME, 0 },        /* Note VK_Home alias! */
    { 0x4b,    VK_DIVIDE, 0 },
    { 0x43,    VK_MULTIPLY, 0 },
    { 0x30,    VK_TAB, 0 },
    { 0x0c,    'Q' ,0 },
    { 0x0d,    'W' ,0 },
    { 0x0e,    'E' ,0 },
    { 0x0f,    'R' ,0 },
    { 0x11,    'T' ,0 },
    { 0x10,    'Y' ,0 },
    { 0x20,    'U' ,0 },
    { 0x22,    'I' ,0 },
    { 0x1f,    'O' ,0 },
    { 0x23,    'P' ,0 },
    { 0x21,    0xdb, 0 },    /* [{ */
    { 0x1e,    0xdd, 0 },    /* ]} */
    { 0x2a,    0xdc, 0 },    /* backslash, bar */
//    { 0x75,    VK_DELETE, 0 },
//    { 0x77,    VK_END, 0 },
//    { 0x79,    VK_PGDN, 0 },
    { 0x59,    VK_NUMPAD7, 0 },
    { 0x5b,    VK_NUMPAD8, 0 },
    { 0x5c,    VK_NUMPAD9, 0 },
    { 0x4e,    VK_SUBTRACT, 0 },
//    { 0x39,    VK_CAPITAL, 0 },
    { 0x00,    'A', 0 },
    { 0x01,    'S', 0 },
    { 0x02,    'D', 0 },
    { 0x03,    'F', 0 },
    { 0x05,    'G', 0 },
    { 0x04,    'H', 0 },
    { 0x26,    'J', 0 },
    { 0x28,    'K', 0 },
    { 0x25,    'L', 0 },
    { 0x29,    0xba, 0 },    /* semicolon */
    { 0x27,    0xde, 0 },    /* single quote */
    { 0x24,    VK_RETURN, 0 },
    { 0x56,    VK_NUMPAD4, 0},
    { 0x57,    VK_NUMPAD5, 0 },
    { 0x58,    VK_NUMPAD6, 0},
    { 0x45,    VK_ADD, 0 },
//    { 0x38,    VK_Shift_L, VK_Shift_R },
    { 0x06,    'Z',0 },
    { 0x07,    'X',0 },
    { 0x08,    'C',0 },
    { 0x09,    'V',0 },
    { 0x0b,    'B',0 },
    { 0x2d,    'N',0 },
    { 0x2e,    'M',0 },
    { 0x2b,    0xbc, 0 },    /* comma */
    { 0x2f,    0xbe, 0 },    /* dot */
    { 0x2c,    0xbf, 0 },
    { 0x3e,    VK_UP, 0 },
    { 0x53,    VK_NUMPAD1, 0 },
    { 0x54,    VK_NUMPAD2, 0 },
    { 0x55,    VK_NUMPAD3, 0 },
//    { 0x36,    VK_CONTROL_L, VK_CONTORL_R },
    { 0x3a,    0x5b, VK_F2 },         /* Option */
    { 0x37,    VK_MENU, VK_F3 },      /* Command */
    { 0x31,    ' ', 0 },
    { 0x3b,    VK_LEFT, 0 },
    { 0x3d,    VK_DOWN, 0 },
    { 0x3c,    VK_RIGHT, 0 },
    { 0x52,    VK_NUMPAD0, 0 },
    { 0x41,    VK_DECIMAL, VK_SEPARATOR },
//    { 0x4c,    VK_ENTER, 0 },
    { -1, -1, -1 }
};

void win_update_modifier_state(int state)
{
	int	state_xor;
	int	is_up;

    #define CAPITAL  4  
    #define SHIFT    2
    #define CONTROL  1 

	state = state & (CONTROL | CAPITAL | SHIFT);
	state_xor = g_x_shift_control_state ^ state;
	is_up = 0;
	if(state_xor & CONTROL) {
		is_up = ((state & CONTROL) == 0);
		adb_physical_key_update(0x36, is_up);
	}
	if(state_xor & CAPITAL) {
		is_up = ((state & CAPITAL) == 0);
		adb_physical_key_update(0x39, is_up);
	}
	if(state_xor & SHIFT) {
		is_up = ((state & SHIFT) == 0);
		adb_physical_key_update(0x38, is_up);
	}

	g_x_shift_control_state = state;
}

int
handle_keysym(UINT vk,BOOL fdown, int cRepeat,UINT flags)
{
    int    a2code;
    int    type;

	if((vk == VK_F7) && fdown) {
        g_fast_disk_emul = !g_fast_disk_emul;
        printf("g_fast_disk_emul is now %d\n", g_fast_disk_emul);
    }

    if((vk == VK_F9 || vk == VK_F8) && fdown) {
        g_warp_pointer = !g_warp_pointer;
        if(g_warp_pointer) {
            SetCapture(hwndMain);
            ShowCursor(FALSE);
            printf("Mouse Pointer grabbed\n");
        } else {
            ReleaseCapture();
            ShowCursor(TRUE);
            printf("Mouse Pointer released\n");
		}
	}

    // Fixup the numlock & other keys

    if (!(flags & 0x100)) {
        switch(vk) {
        case VK_UP:
            vk=VK_NUMPAD8;
            break;
        case VK_LEFT:
            vk=VK_NUMPAD4;
            break;
        case VK_RIGHT:
            vk=VK_NUMPAD6;
            break;
        case VK_DOWN:
            vk=VK_NUMPAD2;
            break;
        case VK_HOME:
            vk=VK_NUMPAD7;
            break;
        case VK_PRIOR:
            vk=VK_NUMPAD9;
            break;
        case VK_END:
            vk=VK_NUMPAD1;
            break;
        case VK_NEXT:
            vk=VK_NUMPAD3;
            break;
        case VK_CLEAR:
            vk=VK_NUMPAD5;
            break;
        }
    } else {
        if (vk == VK_MENU) {
            vk=0x5b;
        } else if (vk == VK_RETURN) {
            vk=VK_HOME;
        }
    }

    win_update_modifier_state(
        ((GetKeyState(VK_CAPITAL)&0x1) ?1:0) << 2 | 
        ((GetKeyState(VK_SHIFT)&0x80)?1:0) << 1 | 
        ((GetKeyState(VK_CONTROL)&0x80)?1:0) << 0); 
    a2code = win_keysym_to_a2code(vk, !fdown);
    if (fdown) {
        //printf ("Keysym=0x%x a2code=0x%x cRepeat=0x%x flag=0x%x\n",
        //        vk,a2code,cRepeat,flags);
    }
    if(a2code >= 0) {
        adb_physical_key_update(a2code, !fdown);
        return 0;    
    } 
    return -1;
}

int
win_keysym_to_a2code(int keysym, int is_up)
{
    int    i;

    if(keysym == 0) {
        return -1;
    }

    /* Look up Apple 2 keycode */
    for(i = g_num_a2_keycodes - 1; i >= 0; i--) {
        if((keysym == a2_key_to_wsym[i][1]) ||
                    (keysym == a2_key_to_wsym[i][2])) {

            vid_printf("Found keysym:%04x = a[%d] = %04x or %04x\n",
                (int)keysym, i, a2_key_to_wsym[i][1],
                a2_key_to_wsym[i][2]);

            return a2_key_to_wsym[i][0];
        }
    }

    return -1;
}

void update_color_array(int col_num, int a2_color) {
    byte r,g,b;
    int palette;
    int full;
    int doit=0;

    doit=1;

    if(col_num >= 256 || col_num < 0) {
        halt_printf("update_color_array called: col: %03x\n", col_num);
        return;
    }

    r = ((a2_color >> 8) & 0xf)<<4;
    g = ((a2_color >> 4) & 0xf)<<4;
    b = ((a2_color) & 0xf)<<4;
    a2v_palette[col_num].rgbBlue       = b;
    a2v_palette[col_num].rgbGreen      = g;
    a2v_palette[col_num].rgbRed        = r;
    a2v_palette[col_num].rgbReserved   = 0;

    full = g_installed_full_superhires_colormap;
    palette = col_num >> 4;

    if(!full && palette == g_a2vid_palette) {
        r = dummy_color.rgbRed;
        g = dummy_color.rgbGreen;
        b = dummy_color.rgbBlue;
        doit = 0;
    }

    shires_palette[col_num].rgbBlue       = b;
    shires_palette[col_num].rgbGreen      = g;
    shires_palette[col_num].rgbRed        = r;
    shires_palette[col_num].rgbReserved   = 0;

    g_full_refresh_needed = -1;
}

void update_physical_colormap(void) {
    int    palette;
    int    full;
    int    i;
    byte r,g,b;
    int value;

    full = g_installed_full_superhires_colormap;

    if(!full) {
        palette = g_a2vid_palette << 4;
        for(i = 0; i < 16; i++) {
            value=lores_colors[i];

            b = (lores_colors[i%16]    & 0xf) << 4;
            g = (lores_colors[i%16]>>4 & 0xf) << 4;
            r = (lores_colors[i%16]>>8 & 0xf) << 4;

            a2v_palette[palette+i].rgbBlue       = b;
            a2v_palette[palette+i].rgbGreen      = g;
            a2v_palette[palette+i].rgbRed        = r;
            a2v_palette[palette+i].rgbReserved   = 0;
        }
    }

    if(full) {
        win_palette=(RGBQUAD *)&shires_palette;
    } else {
        win_palette=(RGBQUAD *)&a2v_palette;;
    }

    a2_screen_buffer_changed = -1;
    g_full_refresh_needed = -1;
    g_border_sides_refresh_needed = 1;
    g_border_special_refresh_needed = 1;
    g_status_refresh_needed = 1;

}

void show_xcolor_array(void) {
}

void x_auto_repeat_on(int must) {
}

void xdriver_end(void) {
    printf ("Close Window\n");

    // Cleanup direct draw objects
    #if 0
    if (lpClipper)
        IDirectDrawClipper_Release(lpClipper);
    if (backsurf)
        IDirectDrawSurface_Release(backsurf);
    if (primsurf)
        IDirectDrawSurface_Release(primsurf);
    if (lpDD)
        IDirectDraw_Release(lpDD);
    #endif

    DeleteObject(ximage_text[0]);
    ximage_text[0]=NULL;
    DeleteObject(ximage_text[1]);
    ximage_text[1]=NULL;
    DeleteObject(ximage_hires[0]);
    ximage_hires[0]=NULL;
    DeleteObject(ximage_hires[1]);
    ximage_hires[1]=NULL;
    DeleteObject(ximage_superhires);
    ximage_superhires=NULL;
    DeleteObject(ximage_border_special);
    ximage_border_special=NULL;
    DeleteObject(ximage_border_sides);
    ximage_border_sides=NULL;

    DeleteDC(hcdc);
    hcdc=NULL;

    WSACleanup();
}

void win_refresh_image (HDC hdc,HBITMAP hbm, int srcx, int srcy,
                        int destx,int desty, int width,int height) {
    HBITMAP hbmOld;
    HBITMAP holdbm=NULL;
    HDC hdcBack;
    POINT p;
    RECT rcRectSrc,rect;

    int i,hr;

    if (hwndMain == NULL)
        return;

    if (width==0 || height == 0) {
        return;
    }

    p.x = 0; p.y = 0;
    ClientToScreen(hwndMain, &p);
    hbmOld = SelectObject(hcdc,hbm);
    SetDIBColorTable(hcdc,0,256,(RGBQUAD *)win_palette);
    BitBlt(hdc,destx,desty,width,height,hcdc,srcx,srcy,SRCCOPY);
 
    #if 0 
    if (hr=IDirectDrawSurface_GetDC(backsurf,&hdcBack) != DD_OK) {
        printf ("Error blitting surface (back_surf) %x\n",hr);
        exit(0);
        return;
    }

   
    BitBlt(hdcBack,destx,desty,width,height,hcdc,srcx,srcy,SRCCOPY);
    IDirectDrawSurface_ReleaseDC(backsurf,hdcBack);

    SetRect(&rect,p.x+destx,p.y+desty,
            p.x+destx+width,p.y+desty+height);
    SetRect(&rcRectSrc,destx,desty,destx+width,desty+height);
    IDirectDrawSurface_Restore(primsurf);
    hr=IDirectDrawSurface_Blt(primsurf, 
                              &rect,backsurf,&rcRectSrc,DDBLT_WAIT, NULL);
    if (hr != DD_OK) {
        printf ("Error blitting surface (prim_surf) %x\n",hr);
        exit(0);
    }
    #endif 
    
    SelectObject(hcdc,hbmOld);
    holdbm=hbm;

}

BOOL A2W_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)  {
   
    RECT rect;
    RECT wrect;
    int  adjx,adjy;
    GetClientRect(hwnd,&rect);
    GetWindowRect(hwnd,&wrect);
    adjx=(wrect.right-wrect.left)-(rect.right-rect.left);
    adjy=(wrect.bottom-wrect.top)-(rect.bottom-rect.top);
    SetWindowPos(hwnd,NULL,0,0,BASE_WINDOW_WIDTH+adjx,
                 //BASE_WINDOW_HEIGHT+adjy,
                 BASE_MARGIN_TOP+A2_WINDOW_HEIGHT+BASE_MARGIN_BOTTOM+adjy+
                 MAX_STATUS_LINES*15,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW );
    return TRUE;
}

void A2W_OnKeyChar(HWND hwnd, UINT vk, BOOL fDown, int cRepeat, UINT flags) {
    handle_keysym(vk,fDown,cRepeat,flags);
}

void A2W_OnDestroy(HWND hwnd) {
    ReleaseDC(hwndMain,hdcRef);
    hdcRef=NULL;
    hwndMain=NULL;
    PostQuitMessage(0);
}

BOOL A2W_OnEraseBkgnd(HWND hwnd, HDC hdc)  {
    if (ximage_superhires == NULL) {
        return FALSE;   
    }
    a2_screen_buffer_changed = -1;
    g_full_refresh_needed = -1;
    g_border_sides_refresh_needed = 1;
    g_border_special_refresh_needed = 1;
    g_status_refresh_needed = 1;
    x_refresh_ximage();
    redraw_status_lines();
    return TRUE;
}

void A2W_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags) {
    int motion=0;
    motion=update_mouse(x,y, 0, 0);
    if (motion && g_warp_pointer) {
        update_mouse(-1,-1,-1,-1);
    }
}

void A2W_OnLButtonDown(HWND hwnd, BOOL fDClick, int x, int y, UINT keyFlags) {
    int motion=0;
    motion=update_mouse(x,y,1,1);
    if (motion && g_warp_pointer) {
        update_mouse(-1,-1,-1,-1);
    }
}

void A2W_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags) {
    int motion=0;
    motion=update_mouse(x,y,0,1);
    if (motion && g_warp_pointer) {
        update_mouse(-1,-1,-1,-1);
    }
}

void A2W_OnRButtonDown(HWND hwnd, BOOL fDClick, int x, int y, UINT keyFlags) {
    g_limit_speed++;
    if(g_limit_speed > 2) {
        g_limit_speed = 0;
    }

    printf("Toggling g_limit_speed to %d\n",g_limit_speed);
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
}

LRESULT CALLBACK A2WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,  
                           LPARAM lParam) {
    switch (uMsg) 
    { 
        HANDLE_MSG(hwnd,WM_CREATE,A2W_OnCreate); 
        HANDLE_MSG(hwnd,WM_DESTROY,A2W_OnDestroy);
        HANDLE_MSG(hwnd,WM_ERASEBKGND,A2W_OnEraseBkgnd);
        HANDLE_MSG(hwnd,WM_MOUSEMOVE,A2W_OnMouseMove);
        HANDLE_MSG(hwnd,WM_KEYUP,A2W_OnKeyChar);
        HANDLE_MSG(hwnd,WM_KEYDOWN,A2W_OnKeyChar);
        HANDLE_MSG(hwnd,WM_SYSKEYUP,A2W_OnKeyChar);
        HANDLE_MSG(hwnd,WM_SYSKEYDOWN,A2W_OnKeyChar);
        HANDLE_MSG(hwnd,WM_LBUTTONDOWN,A2W_OnLButtonDown);
        HANDLE_MSG(hwnd,WM_LBUTTONUP,A2W_OnLButtonUp);
        HANDLE_MSG(hwnd,WM_RBUTTONDOWN,A2W_OnRButtonDown);
    } 
    return DefWindowProc(hwnd, uMsg, wParam, lParam); 
}

#if 0
BOOL initDirectDraw()
{
    HRESULT ddrval;
    DDSURFACEDESC ddsd;
    HMODULE hmod;

    ddrval = DirectDrawCreate( NULL, &lpDD, NULL );
    if (FAILED(ddrval))
    {
        printf ("Error creating direct draw\n");
        return(FALSE);
    }
   
    ddrval = IDirectDraw_SetCooperativeLevel(lpDD,hwndMain,DDSCL_NORMAL);
    if (FAILED(ddrval))
    {
        printf ("Error setting cooperative level\n");
        IDirectDraw_Release(lpDD);
        return(FALSE);
    }
    memset( &ddsd, 0, sizeof(ddsd) );
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    ddrval = IDirectDraw_CreateSurface(lpDD,&ddsd, &primsurf, NULL );
    if (FAILED(ddrval))
    {
        printf ("Error creating primary surface = %x\n",ddrval);
        IDirectDraw_Release(lpDD);
        return(FALSE);
    }
    
    ddrval = IDirectDraw_CreateClipper(lpDD,0, &lpClipper, NULL );
    if (FAILED(ddrval))
    {
        printf ("Error creating direct draw clipper = %x\n",ddrval);
        IDirectDrawSurface_Release(primsurf);
        IDirectDraw_Release(lpDD);
        return(FALSE);
    }

    // setting it to our hwnd gives the clipper the coordinates from our window
    ddrval = IDirectDrawClipper_SetHWnd(lpClipper, 0, hwndMain );
    if (FAILED(ddrval))
    {
        printf ("Error attaching direct draw clipper\n");
        IDirectDrawClipper_Release(lpClipper);
        IDirectDrawSurface_Release(primsurf);
        IDirectDraw_Release(lpDD);
        return(FALSE);
    }

    // attach the clipper to the primary surface
    ddrval = IDirectDrawSurface_SetClipper(primsurf,lpClipper );
    if (FAILED(ddrval))
    {
        printf ("Error setting direct draw clipper\n");
        IDirectDrawClipper_Release(lpClipper);
        IDirectDrawSurface_Release(primsurf);
        IDirectDraw_Release(lpDD);
        return(FALSE);
    }

    memset( &ddsd, 0, sizeof(ddsd) );
    ddsd.dwSize = sizeof( ddsd );
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.dwWidth =  BASE_WINDOW_WIDTH;
    ddsd.dwHeight = BASE_MARGIN_TOP+A2_WINDOW_HEIGHT+BASE_MARGIN_BOTTOM;

    // create the backbuffer separately
    ddrval = IDirectDraw_CreateSurface(lpDD,&ddsd, &backsurf, NULL );
    if (FAILED(ddrval))
    {
        IDirectDrawClipper_Release(lpClipper);
        IDirectDrawSurface_Release(primsurf);
        IDirectDraw_Release(lpDD);
        return(FALSE);
    }

    return(TRUE);
}
#endif

void dev_video_init(void) {
    int    width;
    int    height;
    int    depth;
    int    mdepth;
    int i;
	int	keycode;
	int	tmp_array[0x80];
    byte *ptr;
    WNDCLASS wc; 
    LPBITMAPINFOHEADER pDib;
    TEXTMETRIC tm;  
    WORD wVersionRequested;
    WSADATA wsaData;

    // Create key-codes
	g_num_a2_keycodes = 0;
	for(i = 0; i <= 0x7f; i++) {
		tmp_array[i] = 0;
	}
	for(i = 0; i < 0x7f; i++) {
		keycode = a2_key_to_wsym[i][0];
		if(keycode < 0) {
			g_num_a2_keycodes = i;
			break;
		} else if(keycode > 0x7f) {
			printf("a2_key_to_wsym[%d] = %02x!\n", i, keycode);
				exit(2);
		} else {
			if(tmp_array[keycode]) {
				printf("a2_key_to_x[%d] = %02x used by %d\n",
					i, keycode, tmp_array[keycode] - 1);
			}
			tmp_array[keycode] = i + 1;
		}
	}

    // Initialize Winsock
    wVersionRequested = MAKEWORD(1, 1);

    if (WSAStartup(wVersionRequested, &wsaData)) {
        printf ("Unable to initialize winsock\n");
        exit(0);
    }

    // Create a window
    
    wc.style = 0; 
    wc.lpfnWndProc = (WNDPROC) A2WndProc; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
    wc.hInstance =  GetModuleHandle(NULL);
    wc.hIcon = LoadIcon((HINSTANCE) NULL, 
        IDI_APPLICATION); 
    wc.hCursor = LoadCursor((HINSTANCE) NULL, 
        IDC_ARROW); 
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  "Kegs32"; 
    wc.lpszClassName = "Kegs32"; 

    printf ("Registering Window\n");
    if (!RegisterClass(&wc)) {
        printf ("Register window failed\n");
        return; 
    } 
 
    // Create the main window. 
 
    printf ("Creating Window\n");
    hwndMain = CreateWindow("Kegs32", "Kegs32", 
        WS_OVERLAPPEDWINDOW&(~(WS_MAXIMIZEBOX | WS_SIZEBOX)), 
        //WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        CW_USEDEFAULT, CW_USEDEFAULT, NULL,NULL,GetModuleHandle(NULL),NULL); 
   
    // If the main window cannot be created, terminate 
    // the application. 
 
    if (!hwndMain) {
        printf ("Window create failed\n");
        return;
    }

    SetWindowLong(hwndMain,GWL_STYLE,
                  GetWindowLong(hwndMain,GWL_STYLE) & (~WS_MAXIMIZE));

    #if 0
    if (!initDirectDraw()) {
        printf ("Unable to initialize Direct Draw\n");
        exit(0);
    }
    #endif

    // Create Bitmaps
    pDib = (LPBITMAPINFOHEADER)GlobalAlloc(GPTR,
            (sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)));
    pDib->biSize = sizeof(BITMAPINFOHEADER);
    pDib->biWidth = A2_WINDOW_WIDTH;;
    pDib->biHeight =-A2_WINDOW_HEIGHT;
    pDib->biPlanes = 1;
    pDib->biBitCount = 8;

    hdcRef = GetDC(hwndMain);

    // Set Text color and background color;
    SetTextColor(hdcRef,0xffffff);
    SetBkColor(hdcRef,0);
    SelectObject(hdcRef,GetStockObject(SYSTEM_FIXED_FONT));
    GetTextMetrics(hdcRef,&tm);
    dwchary=tm.tmHeight; 
 
    hcdc = CreateCompatibleDC(hdcRef);

    ximage_text[0]=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,DIB_RGB_COLORS,
                                   (VOID **)&data_text[0],NULL,0);
    ximage_text[1]=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,DIB_RGB_COLORS,
                                   (VOID **)&data_text[1],NULL,0);
    ximage_hires[0]=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,DIB_RGB_COLORS,
                                   (VOID **)&data_hires[0],NULL,0);
    ximage_hires[1]=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,DIB_RGB_COLORS,
                                   (VOID **)&data_hires[1],NULL,0);
    ximage_superhires=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,DIB_RGB_COLORS,
                                   (VOID **)&data_superhires,NULL,0);
    pDib->biWidth = EFF_BORDER_WIDTH;
    ximage_border_sides=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,
                                         DIB_RGB_COLORS,
                                         (VOID **)&data_border_sides,NULL,0);
    pDib->biWidth = (X_A2_WINDOW_WIDTH);
    pDib->biHeight = - (X_A2_WINDOW_HEIGHT - A2_WINDOW_HEIGHT + 2*8);
    ximage_border_special=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,
                                           DIB_RGB_COLORS,
                                          (VOID **)&data_border_special,NULL,0);
    GlobalFree(pDib);

    ShowWindow(hwndMain, SW_SHOWDEFAULT); 
    UpdateWindow(hwndMain); 
}

void update_colormap(int mode) {
    
}

void redraw_border(void) {

}

void check_input_events(void) {
    MSG msg;

    if (PeekMessage(&msg, NULL ,0,0, PM_NOREMOVE)) 
    { 
        if (GetMessage(&msg, NULL ,0,0)) {
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } else {
            my_exit(0);
        }
    } 
}

void update_status_line(int line, const char *string) {
	char	*buf;
	const char *ptr;
	int	i;

	if(line >= MAX_STATUS_LINES || line < 0) {
		printf("update_status_line: line: %d!\n", line);
		exit(1);
	}

	ptr = string;
	buf = &(g_status_buf[line][0]);
	for(i = 0; i < X_LINE_LENGTH; i++) {
		if(*ptr) {
			buf[i] = *ptr++;
		} else {
			buf[i] = ' ';
		}
	}

	buf[X_LINE_LENGTH] = 0;
}

void redraw_status_lines(void) {
    int line=0;
	char	*buf;
    int height=dwchary;

	for(line = 0; line < MAX_STATUS_LINES; line++) {
		buf = &(g_status_buf[line][0]);
		TextOut(hdcRef, 0, X_A2_WINDOW_HEIGHT + height*line,
			buf, strlen(buf));
	}
}

void
x_refresh_ximage()
{
    register word32 start_time;
    register word32 end_time;
    int    start;
    word32    mask;
    int    line;
    int    left_pix, right_pix;
    int    left, right;
    HBITMAP last_xim, cur_xim;
    HDC  hbmOld;

    if(g_border_sides_refresh_needed) {
        g_border_sides_refresh_needed = 0;
        win_refresh_border_sides();
    }
    if(g_border_special_refresh_needed) {
        g_border_special_refresh_needed = 0;
        win_refresh_border_special();
    }

    if(a2_screen_buffer_changed == 0) {
        return;
    }

    GET_ITIMER(start_time);

    start = -1;
    mask = 1;
    last_xim = NULL;

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
                win_refresh_lines(last_xim, start, line,
                    left_pix, right_pix);
                start = -1;
                left_pix = 640;
                right_pix =  0;
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
                win_refresh_lines(last_xim, start, line,
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
        win_refresh_lines(last_xim, start, 25, left_pix, right_pix);
    }

    a2_screen_buffer_changed = 0;

    g_full_refresh_needed = 0;

    /* And redraw border rectangle? */

    GET_ITIMER(end_time);

    g_cycs_in_xredraw += (end_time - start_time);
}

void
win_refresh_lines(HBITMAP xim, int start_line, int end_line, int left_pix,
        int right_pix)
{
    int srcy;
    HDC hdc;

    if(left_pix >= right_pix || left_pix < 0 || right_pix <= 0) {
        halt_printf("win_refresh_lines: lines %d to %d, pix %d to %d\n",
            start_line, end_line, left_pix, right_pix);
        printf("a2_screen_buf_ch:%08x, g_full_refr:%08x\n",
            a2_screen_buffer_changed, g_full_refresh_needed);
        show_a2_line_stuff();
    }

    srcy = 16*start_line;

    if(xim == ximage_border_special) {
        /* fix up y pos in src */
        printf("win_refresh_lines called, ximage_border_special!!\n");
        srcy = 0;
    }

    g_refresh_bytes_xfer += 16*(end_line - start_line) *
                            (right_pix - left_pix);

    if (hwndMain == NULL) {
        return;
    }
    if (g_cur_a2_stat & 0xa0) {
        win_refresh_image(hdcRef,xim,left_pix,srcy,BASE_MARGIN_LEFT+left_pix,
                          BASE_MARGIN_TOP+16*start_line,
                          right_pix-left_pix,16*(end_line-start_line));
    } else {
        win_refresh_image(hdcRef,xim,left_pix,srcy,BASE_MARGIN_LEFT+left_pix+
                          BORDER_WIDTH,
                          BASE_MARGIN_TOP+16*start_line,
                          right_pix-left_pix,16*(end_line-start_line));
    }
}

void
x_redraw_border_sides_lines(int end_x, int width, int start_line,
    int end_line)
{
    HBITMAP    xim;
    HDC hdc;

    if(start_line < 0 || width < 0) {
        return;
    }

#if 0
    printf("redraw_border_sides lines:%d-%d from %d to %d\n",
        start_line, end_line, end_x - width, end_x);
#endif
    xim = ximage_border_sides;
    g_refresh_bytes_xfer += 16 * (end_line - start_line) * width;

    if (hwndMain == NULL) {
        return;
    }
    win_refresh_image (hdcRef,xim,0,16*start_line,end_x-width,
                       BASE_MARGIN_TOP+16*start_line,width,
                       16*(end_line-start_line));
}

void
win_refresh_border_sides()
{
    int    old_width;
    int    prev_line;
    int    width;
    int    mode;
    int    i;

#if 0
    printf("refresh border sides!\n");
#endif

    /* redraw left sides */
    if (g_cur_a2_stat & 0xa0) {
        x_redraw_border_sides_lines(BORDER_WIDTH, 
                                    BORDER_WIDTH, 0, 25);
    } else {
        x_redraw_border_sides_lines(BASE_MARGIN_LEFT+BORDER_WIDTH, 
                                    BASE_MARGIN_LEFT+BORDER_WIDTH, 0, 25);
    }

    /* right side--can be "jagged" */
    prev_line = -1;
    old_width = -1;
    for(i = 0; i < 25; i++) {
        mode = (a2_line_stat[i] >> 4) & 7;
        width = EFF_BORDER_WIDTH;
        if(mode == MODE_SUPER_HIRES) {
            width = BORDER_WIDTH;
        }
        if(width != old_width) {
            if (g_cur_a2_stat & 0xa0) {
                x_redraw_border_sides_lines(X_A2_WINDOW_WIDTH,
                    old_width, prev_line, i);    
            } else {
                x_redraw_border_sides_lines(X_A2_WINDOW_WIDTH,
                    old_width-BORDER_WIDTH, prev_line, i);    
            }

            prev_line = i;
            old_width = width;
        }
    }
    if (g_cur_a2_stat & 0xa0) {
        x_redraw_border_sides_lines(X_A2_WINDOW_WIDTH, 
                                    old_width, prev_line,25);
    } else {
        x_redraw_border_sides_lines(X_A2_WINDOW_WIDTH, 
                                    old_width-BORDER_WIDTH, prev_line,25);
    }
}

void
win_refresh_border_special()
{
    HBITMAP    xim;
    HDC hdc;
    int    width, height;

    width = X_A2_WINDOW_WIDTH;
    height = BASE_MARGIN_TOP;

    xim = ximage_border_special;
    g_refresh_bytes_xfer += 16 * width *
                (BASE_MARGIN_TOP + BASE_MARGIN_BOTTOM);

    if (hwndMain == NULL) {
        return;
    }
    win_refresh_image (hdcRef,xim,0,0,0,BASE_MARGIN_TOP+A2_WINDOW_HEIGHT,
                       width,BASE_MARGIN_BOTTOM);
    win_refresh_image (hdcRef,xim,0,BASE_MARGIN_BOTTOM,0,0,
                       width,BASE_MARGIN_TOP);
}
