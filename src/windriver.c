/****************************************************************/
/*    Apple IIgs emulator                                       */
/*    Copyright 1996 Kent Dickey                                */
/*                                                              */
/*    This code may not be used in a commercial product         */
/*    without prior written permission of the author.           */
/*                                                              */
/*    Windows Code by akilgard@yahoo.com                        */
/*    You may freely distribute this code.                      */ 
/*                                                              */
/****************************************************************/

//#define KEGS_DIRECTDRAW
#define STRICT
#define WIN32_LEAN_AND_MEAN
#define DIRECTDRAW_VERSION 0x0300
#include <windows.h>
#include <shellapi.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <winsock.h>
#include <commctrl.h>
#include <Commdlg.h>
#include <process.h>
#include <tchar.h>
#if defined(KEGS_DIRECTDRAW)
#include <ddraw.h>
#endif
#include <conio.h>
#include "defc.h"
#include "protos_windriver.h"
#include "resource.h"

extern int Verbose;
extern int lores_colors[];
extern int hires_colors[];
extern word32 g_full_refresh_needed;
extern word32 a2_screen_buffer_changed;
extern int g_limit_speed;
extern double g_desired_pmhz;
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
extern int g_doc_vol;
extern char g_kegs_conf_name[256];
extern int g_joystick_type;

double desired_pmhz=1.0;
int g_use_shmem=0;
int    g_x_shift_control_state = 0;
word32 g_cycs_in_xredraw = 0;
word32 g_refresh_bytes_xfer = 0;
int    g_num_a2_keycodes = 0;
int    g_fullscreen=0;
int    g_interlace_mode=0;

byte *data_text[2];
byte *data_hires[2];
byte *data_superhires;
byte *data_border_special;
byte *data_border_sides;
byte *data_interlace;

byte *dint_border_sides;
byte *dint_border_special;
byte *dint_main_win;

#define MAX_STATUS_LINES    7
#define X_LINE_LENGTH       88
#define STATUS_HEIGHT       ((BASE_MARGIN_TOP)+(A2_WINDOW_HEIGHT)+\
                            (BASE_MARGIN_BOTTOM)+\
                            (win_toolHeight.bottom-win_toolHeight.top)+\
                            (win_statHeight.bottom-win_statHeight.top)+\
                            ((win_stat_debug>0) ? MAX_STATUS_LINES*15:0)\
                            )

DWORD   dwchary;
char    g_status_buf[MAX_STATUS_LINES][X_LINE_LENGTH + 1];

// Windows specific stuff
BOOL    isNT;
HWND    hwndMain=NULL;
HWND    hwndHidden=NULL;
HDC     hcdc=NULL;
HDC     hdcRef=NULL;
HBRUSH  hbr=NULL;
TCHAR   szToolText[256];
RGBQUAD lores_palette[256];
RGBQUAD a2v_palette[256];
RGBQUAD shires_palette[256];
RGBQUAD *win_palette;
RGBQUAD dummy_color;
HBITMAP ximage_text[2]={0};
HBITMAP ximage_hires[2]={0};
HBITMAP ximage_superhires=NULL;
HBITMAP ximage_border_special=NULL;
HBITMAP ximage_border_sides=NULL;
HBITMAP ximage_interlace=NULL;
int     win_pause=0;
int     win_stat_debug=0;
HMENU   win_menu=NULL;
HWND    win_status=NULL;
HWND    win_toolbar=NULL;
RECT    win_rect_oldpos;
RECT    win_toolHeight;
RECT    win_statHeight;

// Edit original window proc
WNDPROC oldEditWndProc;

#ifdef KEGS_DIRECTDRAW 
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
    { 0x32,    0xc0,0 },        /* Key number 18? */
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

void toggleInterlaceMode() {
    g_interlace_mode = ! g_interlace_mode;
	a2_screen_buffer_changed = -1;
    g_full_refresh_needed = -1;
    g_border_sides_refresh_needed = 1;
    g_border_special_refresh_needed = 1;
    g_status_refresh_needed = 1;
}

void displaySpeedStatus() {   
    printf("Toggling g_limit_speed to %d\n",g_limit_speed);
    switch(g_limit_speed) {
    case 0:
        if (g_desired_pmhz > 0 ){
            printf ("...%.2fMHz\n",g_desired_pmhz);
        } else {
            printf("...as fast as possible!\n");
        }
        break;
    case 1:
        printf("...1.024MHz\n");
        break;
    case 2:
        printf("...2.5MHz\n");
        break;
    }
}

void PopulateToolTip(LPTOOLTIPTEXT ptool) {
    TCHAR buffer[] = TEXT("Unable to load resource");
    memset(szToolText,0,sizeof(TCHAR)*80);
    _tcscpy(buffer,szToolText);
    LoadString(GetModuleHandle(NULL),ptool->hdr.idFrom, szToolText,80);
    ptool->hinst=NULL;
    ptool->lpszText = szToolText;
}

// Converts ANSI equivalent pathname to long pathname
// The long pathname may contain UNICODE 
void ConvertAnsiToUnicode(TCHAR *file, wchar_t *wfile) {
    wchar_t buffer[2048]={0};
    wchar_t temp[2048]={0};
    wchar_t tfile[2048]={0};

    WIN32_FIND_DATAW fdata;
    HANDLE handle;
    int i;
    int flen;
   
    if (_tcslen(file)==0) return;

    // Internally it works in UNICODE
    // Convert ANSI to UNICODE as necessary
    if (sizeof(TCHAR) < 2) {
        mbstowcs(tfile,(char *)file,strlen((char *)file)); 
    } else {
        wcscpy(tfile,(wchar_t *)file);
    }

    // Ignore UNC paths
    if (wcslen(tfile) >2) {
        if (tfile[0]=='\\' && tfile[1]=='\\') {
            memset(file,0,2048*sizeof(TCHAR)); 
            if (sizeof(TCHAR) <2) {
                wcstombs((char *)file,tfile,wcslen(tfile));
            } else {
                wcscpy((wchar_t *)file,wfile);
            }
            return;
        }
    }

    while (1) {
        memset(fdata.cFileName,0,sizeof(WCHAR)*MAX_PATH);
        handle=FindFirstFileW(tfile,&fdata);
        if (handle != INVALID_HANDLE_VALUE) {
            FindClose(handle);
        }

        flen = wcslen(fdata.cFileName); 

        // Here I assume that i can always get alternate filename
        // when the filename contains unicode character greater than 255
        for (i=0;i<flen;i++) {
            if (fdata.cFileName[i]>127) {
                flen=wcslen(fdata.cAlternateFileName);
                break;
            }
        }

        memset(temp,0,2048*2);

        wcscat(temp,L"\\");
        wcscat(temp,fdata.cFileName);
        wcscat(temp,buffer);
        wcscpy(buffer,temp);

        tfile[wcslen(tfile)-flen-1]=0;
        if (wcsrchr(tfile,'\\')==NULL) {
            memset(temp,0,2048*2);
            wcscpy(temp,tfile);
            wcscat(temp,buffer);
            memset(wfile,0,2048*2); 
            wcscpy((wchar_t *)wfile,temp);
            break;
        }
    }
}

// Converts unicode pathname to ANSI equivalent pathname
// If cannot convert to ANSI file then convert to short file name
void ConvertUnicodeToAnsi(wchar_t *wfile,TCHAR *file) {
    char buffer[2048]={0};
    char temp[2048]={0};
    wchar_t tfile[2048]={0};

    WIN32_FIND_DATAW fdata;
    HANDLE handle;
    int i;
    int flen;
    int flag=0;
    
    if (wcslen(wfile)==0) return;

    wcscpy(tfile,wfile);

    // Ignore UNC paths
    if (wcslen(tfile) >2) {
        if (tfile[0]=='\\' && tfile[1]=='\\') {
            memset(file,0,2048*sizeof(TCHAR)); 
            if (sizeof(TCHAR) <2) {
                wcstombs((char *)file,tfile,wcslen(tfile));
            } else {
                wcscpy((wchar_t *)file,wfile);
            }
            return;
        }
    }

    while (1) {
        memset(fdata.cFileName,0,sizeof(WCHAR)*MAX_PATH);
        handle=FindFirstFileW(tfile,&fdata);
        if (handle != INVALID_HANDLE_VALUE) {
            FindClose(handle);
        }
    
        flen = wcslen(fdata.cFileName); 

        // Here I assume that i can always get alternate filename
        // when the filename contains unicode character greater than 255
        for (i=0;i<flen;i++) {
            if (fdata.cFileName[i]>127) {
                flag=1;
                break;
            }
        }

        memset(temp,0,2048);
    
        if (!flag) {
            wcstombs(temp,fdata.cFileName,flen);  
            if (buffer[0]==0) {
                strcat(buffer,temp);
            } else {
                strcat(temp,"\\");
                strcat(temp,buffer);
                strcpy(buffer,temp);
            }
        } else {
    
            if (wcslen(fdata.cAlternateFileName)==0) {
                memset(file,0,2048*sizeof(TCHAR)); 
                if (sizeof(TCHAR) <2) {
                    wcstombs((char *)file,tfile,wcslen(tfile));
                } else {
                    wcscpy((wchar_t *)file,wfile);
                }
                return;
            }
    
            // Here I assume that i can always get alternate filename
            // when the filename contains unicode character greater than 255
            wcstombs(temp,fdata.cAlternateFileName,
                     wcslen(fdata.cAlternateFileName));  
            if (buffer[0]==0) {
                strcat(buffer,temp);
            } else {
                strcat(temp,"\\");
                strcat(temp,buffer);
                strcpy(buffer,temp);
            }
        }
     
        flag=0;
        tfile[wcslen(tfile)-flen-1]=0;

        if (wcsrchr(tfile,'\\')==NULL) {
            memset(temp,0,2048);
            wcstombs(temp,wfile,wcslen(tfile));
            strcat(temp,"\\");
            strcat(temp,buffer);
           
            memset(file,0,2048*sizeof(TCHAR)); 
            if (sizeof(TCHAR)<2) {
                strcpy((char *)file,temp);
            } else {
                memset(tfile,0,2048*sizeof(TCHAR));
                mbstowcs(tfile,temp,strlen(temp));
                wcscpy((wchar_t *)file,tfile);
            }
            break;
        }
    }
}

//
// read kegs config into disk config dialog
// entry. The Entry control ID are specially
// organized
//
int
read_disk_entry(HWND hDlg)
{
    wchar_t wbuf[2048]={0};
    char    buf[2048]={0}, buf2[2048]={0};
    TCHAR   tbuf[2048]={0};

    FILE    *fconf;
    char    *ptr;
    char    *sptr;
    int    line, slot, drive;
    HWND hwnd;

    fconf = fopen(g_kegs_conf_name, "rt");
    if(fconf == 0) {
        return 1;
    }
    line = 0;
    while(1) {
        ptr = fgets(buf, 2048, fconf);
        if(ptr == 0) {
            /* done */
            break;
        }
        line++;
    
        if((buf[0] == 's') && (buf[2] == 'd')) {
           slot = (buf[1]- 0x30);
           drive = (buf[3] -0x30);
        
           if (drive>=1 && drive<=2 && slot>=5 && slot<=7) {
               hwnd = GetDlgItem(hDlg,10000+slot*10+drive );
               if (hwnd) {
                   buf2[0]='\0';
                   sptr=strstr(buf+4,"=")+1; 
                   // Trim out leading spaces
                   while (*sptr && (*sptr) == ' ') sptr++;
                   memcpy(buf2,sptr,strlen(sptr)-1);
                   buf2[strlen(sptr)-1]=0;
                   //sscanf(strstr(buf+4,"=")+1,"%s",buf2);
                   if (isNT) {
                       FILE *file;

                       // Check whether the file could be opened
                       file=fopen(buf2,"r");
                       if (file) {
                           fclose(file);
                           if (sizeof(TCHAR) <2) {
                               strcpy((char *)tbuf,buf2);
                           } else {
                               mbstowcs((wchar_t *)tbuf,buf2,
                                        strlen(buf2));
                           }
                           ConvertAnsiToUnicode(tbuf,wbuf);
                           SetWindowTextW(hwnd,wbuf);
                       } else {
                           SetWindowTextA(hwnd,buf2);
                       }
                   } else {    
                       SetWindowTextA(hwnd,buf2);
                   }
               }
           }
        }
    };
    return 0;
}

int
write_disk_entry(HWND hDlg)
{
    char    buf[2048];
    TCHAR   tbuf[2048];
    wchar_t wbuf[2048];
    FILE    *fconf;
    int     slot, drive;
    HWND hwnd;
    int     skipspace; 

    printf ("Writing disk configuration to %s\n",g_kegs_conf_name);
    fconf = fopen(g_kegs_conf_name, "wt");
    if(fconf == 0) {
        return 1;
    }
    for (slot=1;slot<=7;slot++) {
        for (drive=1;drive<=2;drive++) {
            if (slot<5) continue;
            hwnd = GetDlgItem(hDlg,10000+slot*10+drive);
            if (!hwnd) continue;
            ZeroMemory(buf,2048);
            if (isNT) {
                BOOL defaultCharUsed;
                FILE *file;
                memset(wbuf,0,2*2048);
                memset(tbuf,0,sizeof(TCHAR)*2048);
                GetWindowTextW(hwnd,wbuf,2048);

                // Check whether wide characters can be localized
                defaultCharUsed=FALSE; 
                WideCharToMultiByte(CP_ACP,0,wbuf,wcslen(wbuf),
                                    buf,2048,NULL,&defaultCharUsed);

                if (defaultCharUsed == TRUE) { 
                    // If not, use short filename
                    file = _wfopen(wbuf,L"r");
                    if (file) {
                        fclose(file);
                        ConvertUnicodeToAnsi(wbuf,tbuf);

                        if (sizeof(TCHAR) <2) {
                            strcpy(buf,(char *)tbuf);
                        } else {
                            wcstombs(buf,(wchar_t *)tbuf,
                                          wcslen((wchar_t *)tbuf));
                        }
                    } else {
                        wcstombs(buf,(wchar_t *)wbuf,wcslen((wchar_t *)wbuf));
                    }
                }
        
            } else {    
                GetWindowTextA(hwnd,buf,2048);
            }
            if (lstrlenA(buf)>0) {
                skipspace=0;
                // Trim out leading spaces
                while (*(buf+skipspace) && *(buf+skipspace)==' ') skipspace++;
                fprintf(fconf,"s%dd%d = %s\n",slot,drive,buf+skipspace);   
            } else {
                fprintf(fconf,"#s%dd%d = <nofile>\n",slot,drive);
            }
        }
    };
    fclose(fconf);
    return 0;
}

void win_update_modifier_state(int state)
{
    int    state_xor;
    int    is_up;

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

// Use the way keys update_mouse works
// to get this to work
void win_warp_cursor(HWND hwnd)
{
    POINT p;
    p.x = X_A2_WINDOW_WIDTH/2;
    p.y = X_A2_WINDOW_HEIGHT/2;
    ClientToScreen(hwnd,&p);
    SetCursorPos(p.x,p.y);
}

int
win_toggle_mouse_cursor(HWND hwnd)
{
    RECT rect;
    g_warp_pointer = !g_warp_pointer;
    if(g_warp_pointer) {
        SetCapture(hwnd);
        ShowCursor(FALSE);
        GetWindowRect(hwnd,&rect);
        ClipCursor(&rect);
        win_warp_cursor(hwnd);
        printf("Mouse Pointer grabbed\n");
    } else {
	    ClipCursor(NULL);
        ShowCursor(TRUE);
	    ReleaseCapture();
        printf("Mouse Pointer released\n");
    }
	return g_warp_pointer;
}

void 
handle_vk_keys(UINT vk)
{
    int    style;
    int    adjx,adjy;
    RECT   wrect,rect;
    DEVMODE dmScreenSettings;

    // Check the state for caps lock,shift and control
    // This checking must be done in the main window and not in the hidden
    // window.  Some configuration of windows doesn't work in the hidden
    // window
    win_update_modifier_state(
        ((GetKeyState(VK_CAPITAL)&0x1) ?1:0) << 2 | 
        ((GetKeyState(VK_SHIFT)&0x80)?1:0) << 1 | 
        ((GetKeyState(VK_CONTROL)&0x80)?1:0) << 0); 

    // Toggle fast disk
    if (vk == VK_F7) {
        g_fast_disk_emul = !g_fast_disk_emul;
        printf("g_fast_disk_emul is now %d\n", g_fast_disk_emul);
    }

    // Handle mouse pointer display toggling
    if (vk == VK_F9 || vk == VK_F8) {
        win_toggle_mouse_cursor(hwndMain);
    }

	if (vk == VK_F12) {
		toggleInterlaceMode();
	}

    // Set full screen 
    if (vk == VK_F11) {
        g_fullscreen = !g_fullscreen;
        if (g_fullscreen) {
   
            // Change Display Resolution
		    memset(&dmScreenSettings,0,sizeof(dmScreenSettings));
		    dmScreenSettings.dmSize=sizeof(dmScreenSettings);
		    dmScreenSettings.dmPelsWidth	= 800;
		    dmScreenSettings.dmPelsHeight	= 600;
		    dmScreenSettings.dmBitsPerPel	= 24;	
		    dmScreenSettings.dmFields=DM_BITSPERPEL|DM_PELSWIDTH|
                                      DM_PELSHEIGHT;

            if (ChangeDisplaySettings(&dmScreenSettings, 2)
                !=DISP_CHANGE_SUCCESSFUL) {
                // If 24-bit palette does not work, try 32-bit            
		        dmScreenSettings.dmBitsPerPel	= 32;	
                if (ChangeDisplaySettings(&dmScreenSettings, 2) 
                    !=DISP_CHANGE_SUCCESSFUL) {
                    printf ("-- Unable to switch to fullscreen mode\n");
                    printf ("-- No 24-bit or 32-bit mode for 800x600\n");
                    dmScreenSettings.dmBitsPerPel=-1;
                } 
            }

            if (dmScreenSettings.dmBitsPerPel >0) {
                ChangeDisplaySettings(&dmScreenSettings, 4); 
            }
            GetWindowRect(hwndMain,&win_rect_oldpos);
            style=GetWindowLong(hwndMain,GWL_STYLE);
            style &= ~WS_CAPTION;
            SetWindowLong(hwndMain,GWL_STYLE,style);
            win_menu=GetMenu(hwndMain);
            SetMenu(hwndMain,NULL);
            ShowWindow(win_status,FALSE);
            ShowWindow(win_toolbar,FALSE);
           
            SetWindowPos(hwndMain,HWND_TOPMOST,0,0,
                         GetSystemMetrics(SM_CXSCREEN),
                         GetSystemMetrics(SM_CYSCREEN),
                         SWP_SHOWWINDOW);
           
            PatBlt(hdcRef,
                   0,0,
                   GetSystemMetrics(SM_CXSCREEN),
                   GetSystemMetrics(SM_CYSCREEN),
                   BLACKNESS
            );

			GdiFlush();

            if (g_warp_pointer) {
        		GetWindowRect(hwndMain,&rect);
		        ClipCursor(&rect);
                win_warp_cursor(hwndMain);
	        }             
            a2_screen_buffer_changed = -1;
            g_full_refresh_needed = -1;
            g_border_sides_refresh_needed = 1;
            g_border_special_refresh_needed = 1;
            g_status_refresh_needed = 1;
            //x_refresh_ximage();
            //redraw_status_lines();
        } else {

            // Restore Display Resolution
            ChangeDisplaySettings(NULL,0);

            style=GetWindowLong(hwndMain,GWL_STYLE);
            style |= WS_CAPTION;
            SetWindowLong(hwndMain,GWL_STYLE,style);
            SetMenu(hwndMain,win_menu);
            ShowWindow(win_status,TRUE);
            ShowWindow(win_toolbar,TRUE);
            win_menu=NULL;
            GetClientRect(hwndMain,&rect);
            GetWindowRect(hwndMain,&wrect);
            adjx=(wrect.right-wrect.left)-(rect.right-rect.left);
            adjy=(wrect.bottom-wrect.top)-(rect.bottom-rect.top);
            SetWindowPos(hwndMain,HWND_NOTOPMOST,
                 win_rect_oldpos.left,win_rect_oldpos.top,
                 BASE_WINDOW_WIDTH+adjx,
                 //BASE_WINDOW_HEIGHT+adjy,
                 //BASE_MARGIN_TOP+A2_WINDOW_HEIGHT+BASE_MARGIN_BOTTOM+adjy+
                 //MAX_STATUS_LINES*15+48,
                 STATUS_HEIGHT+adjy,
                 SWP_SHOWWINDOW );
            ZeroMemory(&win_rect_oldpos,sizeof(RECT));

            if (g_warp_pointer) {
		        GetWindowRect(hwndMain,&rect);
		        ClipCursor(&rect);
	        }      
        }
    }
}

int
handle_keysym(UINT vk,BOOL fdown, int cRepeat,UINT flags)
{
    int    a2code;


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
    win_pause=1;
    printf ("Close Window.\n");

    // Wait for display thread to finish and stall
    // so we can free resources
    Sleep(300);  

    // Cleanup direct draw objects
    #if defined(KEGS_DIRECTDRAW)
    if (lpClipper)
        IDirectDrawClipper_Release(lpClipper);
    if (backsurf)
        IDirectDrawSurface_Release(backsurf);
    if (primsurf)
        IDirectDrawSurface_Release(primsurf);
    if (lpDD)
        IDirectDraw_Release(lpDD);
    #endif

    if (ximage_text[0] != NULL)
        DeleteObject(ximage_text[0]);
    ximage_text[0]=NULL;
    if (ximage_text[1] != NULL)
        DeleteObject(ximage_text[1]);
    ximage_text[1]=NULL;
    if (ximage_hires[0] != NULL)
        DeleteObject(ximage_hires[0]);
    ximage_hires[0]=NULL;
    if (ximage_hires[1] != NULL)
        DeleteObject(ximage_hires[1]);
    ximage_hires[1]=NULL;
    if (ximage_superhires != NULL)
        DeleteObject(ximage_superhires);
    ximage_superhires=NULL;
    if (ximage_border_special != NULL)
        DeleteObject(ximage_border_special);
    ximage_border_special=NULL;
    if (ximage_border_sides != NULL)
        DeleteObject(ximage_border_sides);
    ximage_border_sides=NULL;
    if (ximage_interlace != NULL)
        DeleteObject(ximage_interlace);
    ximage_interlace=NULL;

    if (hbr != NULL) {
        DeleteObject(hbr);
        hbr=NULL;
    }

	if (hcdc != NULL) 
		DeleteDC(hcdc);
    hcdc=NULL;
}

void win_centroid (int *centerx, int *centery) {

    RECT wrect,rect;
    int  adjx,adjy;

    GetClientRect(hwndMain,&rect);
    GetWindowRect(hwndMain,&wrect);
    adjx=(wrect.right-wrect.left)-(rect.right-rect.left);
    adjy=(wrect.bottom-wrect.top)-(rect.bottom-rect.top);

    if (wrect.right-wrect.left > BASE_WINDOW_WIDTH+adjx) {
        *centerx=((wrect.right-wrect.left)-(BASE_WINDOW_WIDTH+adjx));
        *centerx=(*centerx)/2;
    }

    if (wrect.bottom-wrect.top > STATUS_HEIGHT+adjy) {
        *centery=((wrect.bottom-wrect.top)-(STATUS_HEIGHT+adjy));
        *centery=(*centery)/2;
    }

}

void win_refresh_image (HDC hdc,HBITMAP hbm, int srcx, int srcy,
                        int destx,int desty, int width,int height) {
    HBITMAP hbmOld;
    HBITMAP holdbm=NULL;
    HDC     hdcBack;
    HBRUSH  oldhbr;
    POINT p;
    RECT  rcRectSrc,rect;
    int   centerx,centery;
    int hr;

    if (hwndMain == NULL || hdc == NULL || hcdc == NULL)
        return;

    if (width==0 || height == 0) {
        return;
    }

    centerx=centery=0;
    win_centroid(&centerx,&centery);
    hbmOld = SelectObject(hcdc,hbm);

	if (win_palette) {
		SetDIBColorTable(hcdc,0,256,(RGBQUAD *)win_palette);
	}

    if (!g_fullscreen) {
        centery+=(win_toolHeight.bottom-win_toolHeight.top);
    }

    #if !defined(KEGS_DIRECTDRAW)

	if (g_interlace_mode) {
		oldhbr=SelectBrush(hdc,hbr);
		BitBlt(hdc,destx+centerx,desty+centery,
		       width,height,hcdc,srcx,srcy,MERGECOPY);
		SelectBrush(hdc,oldhbr);
	} else {
		BitBlt(hdc,destx+centerx,desty+centery,
		       width,height,hcdc,srcx,srcy,SRCCOPY);
	}
    
    #else 
    if ((hr=IDirectDrawSurface_GetDC(backsurf,&hdcBack)) != DD_OK) {
        printf ("Error blitting surface (back_surf) %x\n",hr);
        exit(0);
        return;
    }

    BitBlt(hdcBack,destx,desty,width,height,hcdc,srcx,srcy,SRCCOPY);
    IDirectDrawSurface_ReleaseDC(backsurf,hdcBack);

    SetRect(&rcRectSrc,destx,desty,destx+width,desty+height);
    hr=IDirectDrawSurface_Restore(primsurf);
    if (hr != DD_OK) {
        printf ("Error restoring surface (prim_surf) %x\n",hr);
        exit(0);
    }

    p.x = 0; p.y = 0;
    ClientToScreen(hwndMain, &p);
    SetRect(&rect,p.x+destx+centerx,p.y+desty+centery,
            p.x+destx+width+centerx,p.y+desty+height+centery);
    hr=IDirectDrawSurface_Blt(primsurf, 
                              &rect,backsurf,&rcRectSrc,DDBLT_WAIT, NULL);
    if (hr != DD_OK) {
        printf ("Error blitting surface (prim_surf) %x\n",hr);
        printf ("Primary rect (%ld %ld -- %ld %ld)\n",
                rect.top,rect.left,rect.bottom,rect.right);
        printf ("Back surf rect (%ld %ld -- %ld %ld)\n",
                rcRectSrc.top,rcRectSrc.left,rcRectSrc.bottom,rcRectSrc.right);
        exit(0);
    }

    #endif 
    
    SelectObject(hcdc,hbmOld);
    holdbm=hbm;
}

void initWindow(HWND hwnd) {
    RECT rect;
    RECT wrect;
	
	int  adjx,adjy;
    ZeroMemory(&win_rect_oldpos,sizeof(RECT));
    GetClientRect(hwnd,&rect);
    GetWindowRect(hwnd,&wrect);
    adjx=(wrect.right-wrect.left)-(rect.right-rect.left);
    adjy=(wrect.bottom-wrect.top)-(rect.bottom-rect.top);

    SetWindowPos(hwnd,NULL,0,0,BASE_WINDOW_WIDTH+adjx,
                 //BASE_WINDOW_HEIGHT+adjy,
                 STATUS_HEIGHT+adjy,
                 SWP_NOACTIVATE | SWP_NOZORDER);

    SendMessage(win_status,WM_SIZE,0,0);
}

/*
BOOL A2W_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)  {

    RECT rect;
    RECT wrect;
    
	int  adjx,adjy;
    ZeroMemory(&win_rect_oldpos,sizeof(RECT));
    GetClientRect(hwnd,&rect);
    GetWindowRect(hwnd,&wrect);
    adjx=(wrect.right-wrect.left)-(rect.right-rect.left);
    adjy=(wrect.bottom-wrect.top)-(rect.bottom-rect.top);

    SetWindowPos(hwnd,NULL,0,0,BASE_WINDOW_WIDTH+adjx,
                 //BASE_WINDOW_HEIGHT+adjy,
                 BASE_MARGIN_TOP+A2_WINDOW_HEIGHT+BASE_MARGIN_BOTTOM+adjy+48,
                 SWP_SHOWWINDOW );
  
    return TRUE;
}
*/

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
    RECT rect;
    RECT wrect;
    int  adjx,adjy;
    int  centerx,centery;

    if (ximage_superhires == NULL) {
        return FALSE;   
    }

    if (win_toolbar == NULL || win_status == NULL || win_palette == NULL) {
        return FALSE;
    }

    if (!g_fullscreen) {
        SendMessage(win_toolbar,TB_AUTOSIZE,0,0);
        SendMessage(win_status,WM_SIZE,0,0);
    }

    GetClientRect(hwnd,&rect);
    GetWindowRect(hwnd,&wrect);
    adjx=(wrect.right-wrect.left)-(rect.right-rect.left);
    adjy=(wrect.bottom-wrect.top)-(rect.bottom-rect.top);

    centerx=centery=0;
    if (wrect.right-wrect.left > BASE_WINDOW_WIDTH+adjx) {
        centerx=((wrect.right-wrect.left)-(BASE_WINDOW_WIDTH+adjx))/2;
    }
    if (wrect.bottom-wrect.top > STATUS_HEIGHT+adjy) {
        centery=((wrect.bottom-wrect.top)-(STATUS_HEIGHT+adjy))/2;
    }

    if (wrect.right-wrect.left > BASE_WINDOW_WIDTH+adjx) {
        PatBlt(hdcRef,
               BASE_WINDOW_WIDTH+centerx,0,
               (wrect.right-wrect.left)-BASE_WINDOW_WIDTH-adjx, 
               (wrect.bottom-wrect.top),
               BLACKNESS
              );
        PatBlt(hdcRef,
               0,0,
               centerx, 
               (wrect.bottom-wrect.top),
               BLACKNESS
              );
    }

    if (wrect.bottom-wrect.top > (STATUS_HEIGHT+adjy)) {
        PatBlt(hdcRef,
               centerx, centery+(BASE_MARGIN_TOP)+(A2_WINDOW_HEIGHT)+
               (BASE_MARGIN_BOTTOM),
               BASE_WINDOW_WIDTH+adjx,
               wrect.bottom-
               (centery+(BASE_MARGIN_TOP)+(A2_WINDOW_HEIGHT)+
               (BASE_MARGIN_BOTTOM)),
               BLACKNESS
              );
        PatBlt(hdcRef,
               centerx,0,
               BASE_WINDOW_WIDTH+adjx,
               centery,
               BLACKNESS
              );

    }

	GdiFlush();

    a2_screen_buffer_changed = -1;
    g_full_refresh_needed = -1;
    g_border_sides_refresh_needed = 1;
    g_border_special_refresh_needed = 1;
    g_status_refresh_needed = 1;
    x_refresh_ximage();
    redraw_status_lines();
    return TRUE;
}

void CenterDialog (HWND hDlg) {
    RECT rc, rcDlg,rcOwner;
    HWND hwndOwner;

    if ((hwndOwner = GetParent(hDlg)) == NULL) {
        hwndOwner = GetDesktopWindow();
    }

    GetWindowRect(hwndOwner,&rcOwner);
    GetWindowRect(hDlg,&rcDlg);
    CopyRect(&rc,&rcOwner);
    OffsetRect(&rcDlg,-rcDlg.left,-rcDlg.top);
    OffsetRect(&rc,-rc.left,-rc.top);
    OffsetRect(&rc,-rcDlg.right,-rcDlg.bottom);

    SetWindowPos(hDlg,hwndMain,rcOwner.left+(rc.right/2),
                 rcOwner.top+(rc.bottom/2), 0,0,
                 SWP_SHOWWINDOW | SWP_NOSIZE );
}

// Message handler for about box.
LRESULT CALLBACK A2W_Dlg_About_Dialog(HWND hDlg, UINT message, WPARAM wParam, 
                                      LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
          CenterDialog(hDlg);
          return TRUE;
    case WM_COMMAND:
       switch (LOWORD(wParam)) {
       case IDOK:
           EndDialog(hDlg, LOWORD(wParam));
           return TRUE;
       case IDCANCEL:
           EndDialog(hDlg, LOWORD(wParam));
           return FALSE;

       }
    }
    return FALSE;
}

void setup_speed_dialog(HWND hDlg)
{
    int nID;
    TCHAR buffer[2048];

    nID=IDC_NORMAL;
    if (g_limit_speed ==1 ) nID=IDC_SLOW;
    else if (g_limit_speed==2) nID=IDC_NORMAL;
    else if (g_limit_speed==0) {
        nID=IDC_FASTEST;
        if (g_desired_pmhz > 0.0) {
            nID=IDC_CUSTOM;
        }
    }

    SendMessage(GetDlgItem(hDlg,nID),BM_SETCHECK,(WPARAM) TRUE,(LPARAM) 0);
    _stprintf(buffer,_T("%.2f"),desired_pmhz);
    SetDlgItemText(hDlg,IDC_EDITCUSTOM,buffer);
}

void get_speed_dialog(HWND hDlg)
{
    TCHAR buffer[2048]={0};
    double speed;

    if (SendMessage(GetDlgItem(hDlg,IDC_SLOW),BM_GETCHECK,0,0) == BST_CHECKED) {
        g_limit_speed = 1;
    } else if (SendMessage(GetDlgItem(hDlg,IDC_NORMAL),BM_GETCHECK,0,0) == BST_CHECKED) {
        g_limit_speed = 2;
    } else if (SendMessage(GetDlgItem(hDlg,IDC_FASTEST),BM_GETCHECK,0,0) == BST_CHECKED) {
        g_limit_speed = 0;
        g_desired_pmhz= 0;
    } else if (SendMessage(GetDlgItem(hDlg,IDC_CUSTOM),BM_GETCHECK,0,0) == BST_CHECKED) {
        g_limit_speed = 0;
        GetDlgItemText(hDlg,IDC_EDITCUSTOM,buffer,2048);
        speed=0.0;
        printf ("Speed=%s\n",buffer);
        if (_stscanf(buffer,"%lf",&speed) > 0) {
            if (speed >0) {
               desired_pmhz=speed;
            }
        }
        g_desired_pmhz=desired_pmhz;
    }
    displaySpeedStatus();
}

// Message handler for about box.
LRESULT CALLBACK A2W_Dlg_Speed_Dialog(HWND hDlg, UINT message, WPARAM wParam, 
                                      LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
          CenterDialog(hDlg);
          // Clean entry field (init)
          SetWindowText(GetDlgItem(hDlg,IDC_EDITCUSTOM),_T(""));
          setup_speed_dialog(hDlg);
          return TRUE;
    case WM_COMMAND:
       switch (LOWORD(wParam)) {
       case IDOK:
           get_speed_dialog(hDlg);
           EndDialog(hDlg, LOWORD(wParam));
           return TRUE;
       case IDCANCEL:
           EndDialog(hDlg, LOWORD(wParam));
           return FALSE;
       }
    }
    return FALSE;
}

// Message handler for handling edit control
// For dropping files
LRESULT CALLBACK A2W_Dlg_Edit(HWND hwnd, UINT message, WPARAM wParam, 
                              LPARAM lParam) {
    switch (message) {
    case WM_DROPFILES:
        {    
            wchar_t wbuffer[2048]={0};
            TCHAR buffer[2048]={0};
            HDROP hdrop = (HDROP) wParam;
            memset(buffer,0,2048*sizeof(TCHAR));
            
            if (isNT) {
                DragQueryFileW(hdrop,0,wbuffer,2048);
                SetWindowTextW(hwnd,wbuffer);
                DragFinish(hdrop);
                return 0;
            } else {
                DragQueryFile(hdrop,0,buffer,2048);
            }
            SetWindowText(hwnd,buffer);
            DragFinish(hdrop);
            return 0;
        }
    default:
        return CallWindowProc(oldEditWndProc,hwnd,message,wParam,lParam);
    }
}

// Message handler for disk config.
LRESULT CALLBACK A2W_Dlg_Disk(HWND hDlg, UINT message, WPARAM wParam, 
                              LPARAM lParam)
{
    typedef LONG(CALLBACK *SETWL)(HWND,int,LONG);

    SETWL SetWL=NULL;

    switch (message) {
    case WM_INITDIALOG:
          CenterDialog(hDlg);

          // Subclass the edit-control
          SetWL=SetWindowLong;
          if (isNT) {
              SetWL=SetWindowLongW;
          } 
          oldEditWndProc=(WNDPROC) SetWL(GetDlgItem(hDlg,IDC_EDIT_S5D1),
                                         GWL_WNDPROC,(LONG)A2W_Dlg_Edit);
          SetWL(GetDlgItem(hDlg,IDC_EDIT_S5D2),GWL_WNDPROC,
               (LONG)A2W_Dlg_Edit);
          SetWL(GetDlgItem(hDlg,IDC_EDIT_S6D1),GWL_WNDPROC,
               (LONG)A2W_Dlg_Edit);
          SetWL(GetDlgItem(hDlg,IDC_EDIT_S6D2),GWL_WNDPROC,
               (LONG)A2W_Dlg_Edit);
          SetWL(GetDlgItem(hDlg,IDC_EDIT_S7D1),GWL_WNDPROC,
               (LONG)A2W_Dlg_Edit);
          SetWL(GetDlgItem(hDlg,IDC_EDIT_S7D2),GWL_WNDPROC,
               (LONG)A2W_Dlg_Edit);

          // Clean entry field (init)
          SetWindowText(GetDlgItem(hDlg,IDC_EDIT_S5D1),_T(""));
          SetWindowText(GetDlgItem(hDlg,IDC_EDIT_S5D2),_T(""));
          SetWindowText(GetDlgItem(hDlg,IDC_EDIT_S6D1),_T(""));
          SetWindowText(GetDlgItem(hDlg,IDC_EDIT_S6D2),_T(""));
          SetWindowText(GetDlgItem(hDlg,IDC_EDIT_S7D1),_T(""));
          SetWindowText(GetDlgItem(hDlg,IDC_EDIT_S7D2),_T(""));
          read_disk_entry(hDlg);
          return TRUE;
    case WM_COMMAND:
       switch (LOWORD(wParam)) {
       case IDOK:
           if (write_disk_entry(hDlg)) {
               MessageBox(hDlg,
                   _T("Failed to save disk configuration.\n"
                     "Please check or restore from backup.\n"
                     "Aborting Operation.\nYou can quit by pressing Cancel."),
                   _T("Disk configuration save failure"),
                   MB_OK|MB_ICONEXCLAMATION);
               break;
           }
           EndDialog(hDlg, LOWORD(wParam));
           return TRUE;
       case IDCANCEL:
           EndDialog(hDlg, LOWORD(wParam));
           return FALSE;
       case IDC_BTN_S5D1:
       case IDC_BTN_S5D2:
       case IDC_BTN_S6D1:
       case IDC_BTN_S6D2:
       case IDC_BTN_S7D1:
       case IDC_BTN_S7D2:
           {
               
               TCHAR filename[2048]={0};
             
               if (isNT) {
                   WCHAR wfile[2048]={0}; 
                   OPENFILENAMEW opfn;
                   ZeroMemory(&opfn,sizeof(opfn));
                   opfn.lStructSize=sizeof(opfn);
                   opfn.hwndOwner=hDlg;
                   opfn.lpstrFilter=  L"2mg format (*.2mg)\0*.2mg\0"
                                      L"Prodos order format (*.po)\0*.po\0"
                                      L"Dos order format (*.dsk)\0*.dsk\0"
                                      L"All Files (*.*)\0*.*\0"
                                      L"\0\0";
                   opfn.lpstrFile=wfile;
                   opfn.nMaxFile=2048;
                   opfn.Flags=OFN_EXPLORER | OFN_FILEMUSTEXIST | 
                              OFN_HIDEREADONLY | OFN_NOCHANGEDIR ;
                   if (GetOpenFileNameW(&opfn)) {
                        SetWindowTextW(GetDlgItem(hDlg,
                                       LOWORD(wParam-1000)),
                                       wfile);
                   }    

               } else {
                   OPENFILENAME opfn;
                   ZeroMemory(&opfn,sizeof(opfn));
                   opfn.lStructSize=sizeof(opfn);
                   opfn.hwndOwner=hDlg;
                   opfn.lpstrFilter=_T("2mg format (*.2mg)\0*.2mg\0"
                                       "Prodos order format (*.po)\0*.po\0"
                                       "Dos order format (*.dsk)\0*.dsk\0"
                                       "All Files (*.*)\0*.*\0"
                                       "\0\0");
                   opfn.lpstrFile=filename;
                   opfn.nMaxFile=2048;
                   opfn.Flags=OFN_EXPLORER | OFN_FILEMUSTEXIST | 
                              OFN_HIDEREADONLY | OFN_NOCHANGEDIR ;
                   if (GetOpenFileName(&opfn)) {
                       SetWindowText(GetDlgItem(hDlg,LOWORD(wParam-1000)),
                                     filename);
                   }    
               }

           }
           break;
       }
       break;
    }
    return FALSE;
}

void A2W_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify) {
   switch (id) {
   case ID_HELP_ABOUT:
      win_pause=1;
      DialogBoxParam(GetModuleHandle(NULL), 
                     (LPCTSTR)IDD_ABOUT_DIALOG,
                     hwnd, 
                     (DLGPROC)A2W_Dlg_About_Dialog,0);
      win_pause=0;
      break;
   case ID_HELP_KEY:
      win_pause=1;
      DialogBoxParam(GetModuleHandle(NULL), 
                     (LPCTSTR)IDD_KEGS32_KEY,
                     hwnd, 
                     (DLGPROC)A2W_Dlg_About_Dialog,0);
      win_pause=0;
      break;
   case ID_FILE_SPEED:
      win_pause=1;
      DialogBoxParam(GetModuleHandle(NULL), 
                     (LPCTSTR)IDD_SPEEDDIALOG,
                     hwnd, 
                     (DLGPROC)A2W_Dlg_Speed_Dialog,0);
      win_pause=0;
      break;
   case ID_FILE_JOYSTICK:
       if (g_joystick_type == JOYSTICK_MOUSE) {
            joystick_init();
            if (g_joystick_type != JOYSTICK_MOUSE){
                CheckMenuItem(GetMenu(hwndMain),ID_FILE_JOYSTICK,
                              MF_CHECKED);
            };
       } else {
           g_joystick_type = JOYSTICK_MOUSE;
           CheckMenuItem(GetMenu(hwndMain),ID_FILE_JOYSTICK,
                         MF_UNCHECKED);
       }
       break;
   case ID_FILE_DISK:
      win_pause=1; 
      if (isNT) {
          DialogBoxParamW(GetModuleHandle(NULL), 
                          (LPCWSTR)IDD_DLG_DISKCONF,
                          hwnd, 
                          (DLGPROC)A2W_Dlg_Disk,0);
      } else {
          DialogBoxParam(GetModuleHandle(NULL), 
                         (LPCTSTR)IDD_DLG_DISKCONF,
                         hwnd, 
                         (DLGPROC)A2W_Dlg_Disk,0);
      }

      win_pause=0;
      break;
   case ID_FILE_EXIT:
      PostQuitMessage(0);
      break;
   case ID_FILE_DEBUGSTAT:
       win_stat_debug = ! win_stat_debug;
       {
           RECT rect,wrect;
           int adjx,adjy;
           GetClientRect(hwndMain,&rect);
           GetWindowRect(hwndMain,&wrect);
           adjx=(wrect.right-wrect.left)-(rect.right-rect.left);
           adjy=(wrect.bottom-wrect.top)-(rect.bottom-rect.top);
           SetWindowPos(hwndMain,HWND_NOTOPMOST,
                        win_rect_oldpos.left,win_rect_oldpos.top,
                        BASE_WINDOW_WIDTH+adjx,
                        STATUS_HEIGHT+adjy,
                        SWP_NOMOVE | SWP_SHOWWINDOW );
       }

       if (win_stat_debug) {
           CheckMenuItem(GetMenu(hwndMain),ID_FILE_DEBUGSTAT,MF_CHECKED);
       } else {
           CheckMenuItem(GetMenu(hwndMain),ID_FILE_DEBUGSTAT,MF_UNCHECKED);
       }
       break;
   case ID_FILE_SENDRESET:
      if (hwndHidden) {
          // Simulate key pressing to send reset
          SendMessage(hwndHidden,WM_KEYDOWN,0x11,0x1d0001L);
		  adb_physical_key_update(0x36, 0);
          SendMessage(hwndHidden,WM_KEYDOWN,0x3,0x1460001L);
	
          SendMessage(hwndHidden,WM_KEYUP,3,0xc1460001L);
		  SendMessage(hwndHidden,WM_KEYUP,0x11,0xc01d0001L); 
		  adb_physical_key_update(0x36, 1);
      }
      break;
   case ID_FILE_FULLSCREEN:
	  handle_vk_keys(VK_F11);
	  break;
   case ID_SPEED_1MHZ:
      g_limit_speed = 1;
      displaySpeedStatus();
      break;
   case ID_SPEED_2MHZ:
      g_limit_speed = 2;
      displaySpeedStatus();
      break;
   case ID_SPEED_FMHZ:
      g_limit_speed = 0;
      g_desired_pmhz=0.0;
      displaySpeedStatus();
      break;
   }
}

void A2W_OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags) {
    int motion=0;
    int centerx,centery;
    centerx=centery=0;
    win_centroid(&centerx,&centery);
    motion=update_mouse(x,y, 0, 0);
    if (motion && g_warp_pointer) {
        win_warp_cursor(hwndMain);
        update_mouse(-1,-1,-1,-1);
    }
}

void A2W_OnLButtonDown(HWND hwnd, BOOL fDClick, int x, int y, UINT keyFlags) {
    int motion=0;
    int centerx,centery;
    centerx=centery=0;
    win_centroid(&centerx,&centery);
    motion=update_mouse(x,y,1,1);
    if (motion && g_warp_pointer) {
        win_warp_cursor(hwndMain);
        update_mouse(-1,-1,-1,-1);
    }
}

void A2W_OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags) {
    int motion=0;
    int centerx,centery;
    centerx=centery=0;
    win_centroid(&centerx,&centery);
    motion=update_mouse(x,y,0,1);
    if (motion && g_warp_pointer) {
        win_warp_cursor(hwndMain);
        update_mouse(-1,-1,-1,-1);
    }
}

void A2W_OnRButtonDown(HWND hwnd, BOOL fDClick, int x, int y, UINT keyFlags) {
    g_limit_speed++;
    if(g_limit_speed > 2) {
        g_limit_speed = 0;
    }

    displaySpeedStatus();

}

void A2W_ActivateApp(HWND hwnd, BOOL activate,DWORD tid) {

	if (!activate) {
		if (g_warp_pointer) {
			win_toggle_mouse_cursor(hwndMain);
		}
	}
}

LRESULT CALLBACK A2WndProcHidden(HWND hwnd,UINT uMsg,WPARAM wParam,  
                                 LPARAM lParam) {
    /*
    if (uMsg == WM_KEYDOWN) {
        printf ("Down:%x %x\n",wParam,lParam);
    }
    if (uMsg == WM_KEYUP) {
        printf ("UP:%x %x\n",wParam,lParam);
    }
	*/
     
    switch (uMsg) 
    {
        HANDLE_MSG(hwnd,WM_MOUSEMOVE,A2W_OnMouseMove);
        HANDLE_MSG(hwnd,WM_KEYUP,A2W_OnKeyChar);
        HANDLE_MSG(hwnd,WM_KEYDOWN,A2W_OnKeyChar);
        HANDLE_MSG(hwnd,WM_SYSKEYUP,A2W_OnKeyChar);
        HANDLE_MSG(hwnd,WM_SYSKEYDOWN,A2W_OnKeyChar);
        HANDLE_MSG(hwnd,WM_LBUTTONDOWN,A2W_OnLButtonDown);
        HANDLE_MSG(hwnd,WM_LBUTTONUP,A2W_OnLButtonUp);
    } 
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam); 
}

LRESULT CALLBACK A2WndProc(HWND hwnd,UINT uMsg,WPARAM wParam,  
                           LPARAM lParam) {

    switch (uMsg) 
    { 
        case WM_NOTIFY:
        {
            LPTOOLTIPTEXT ptool;
            ptool=(LPTOOLTIPTEXT) lParam;
            if (ptool->hdr.code == TTN_NEEDTEXT) {
                PopulateToolTip(ptool);
                return 0;
            }
        } 
        case WM_KEYDOWN:
            handle_vk_keys(wParam);
        case WM_MOUSEMOVE:
        case WM_KEYUP:
        case WM_SYSKEYUP:
        case WM_SYSKEYDOWN:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP: 
        {
            if (hwndHidden != NULL) {
                PostMessage(hwndHidden,uMsg,wParam,lParam);
            }
            return 0;
        }
		
		HANDLE_MSG(hwnd,WM_ACTIVATEAPP,A2W_ActivateApp);
        //HANDLE_MSG(hwnd,WM_CREATE,A2W_OnCreate); 
        HANDLE_MSG(hwnd,WM_DESTROY,A2W_OnDestroy);
        HANDLE_MSG(hwnd,WM_ERASEBKGND,A2W_OnEraseBkgnd);
        HANDLE_MSG(hwnd,WM_RBUTTONDOWN,A2W_OnRButtonDown);
        HANDLE_MSG(hwnd,WM_COMMAND,A2W_OnCommand);
    }
  
    return DefWindowProc(hwnd, uMsg, wParam, lParam); 
}

#if defined(KEGS_DIRECTDRAW)
BOOL initDirectDraw()
{
    HRESULT ddrval;
    DDSURFACEDESC ddsd;

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
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY;

    ddrval = IDirectDraw_CreateSurface(lpDD,&ddsd, &primsurf, NULL );
    if (FAILED(ddrval))
    {
        printf ("Error creating primary surface = %ld\n",ddrval);
        IDirectDraw_Release(lpDD);
        return(FALSE);
    }
    
    ddrval = IDirectDraw_CreateClipper(lpDD,0, &lpClipper, NULL );
    if (FAILED(ddrval))
    {
        printf ("Error creating direct draw clipper = %ld\n",ddrval);
        IDirectDrawSurface_Release(primsurf);
        IDirectDraw_Release(lpDD);
        return(FALSE);
    }

    // setting it to our hwnd gives the clipper the coordinates from 
    // our window
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
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_VIDEOMEMORY;
    ddsd.dwWidth =  BASE_WINDOW_WIDTH;
    ddsd.dwHeight = BASE_MARGIN_TOP+A2_WINDOW_HEIGHT+BASE_MARGIN_BOTTOM;
    //ddsd.dwWidth =  GetSystemMetrics(SM_CXSCREEN);
    //ddsd.dwHeight = GetSystemMetrics(SM_CYSCREEN);
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
    unsigned int tid;
    int waitVar=0;
    WNDCLASS wc; 
    OSVERSIONINFO osVersion;

    // Check the windows version
    osVersion.dwOSVersionInfoSize=sizeof(OSVERSIONINFO);
    GetVersionEx(&osVersion);
    if (osVersion.dwPlatformId >=2 && osVersion.dwMajorVersion >=4) {
        isNT=1;
    }

    _beginthreadex(NULL,0,dev_video_init_ex,(void *)&waitVar,0,&tid);

    // Wait for the background thread to finish initializing
    while (!waitVar) {
        Sleep(1);
    }

    // Create a hidden window
    wc.style = 0; 
    wc.lpfnWndProc = (WNDPROC) A2WndProcHidden; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
    wc.hInstance =  GetModuleHandle(NULL);
    wc.hIcon = LoadIcon((HINSTANCE) NULL, IDI_APPLICATION); 
    wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW); 
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  NULL;
    wc.lpszClassName = _T("Kegs32 Hidden Window"); 
    if (!RegisterClass(&wc)) {
        printf ("Register window failed\n");
        return; 
    }
    hwndHidden = CreateWindow(
        _T("Kegs32 Hidden Window"), 
        _T("Kegs32 Hidden Window"), 
        WS_OVERLAPPED,
        CW_USEDEFAULT, CW_USEDEFAULT, 
        CW_USEDEFAULT, CW_USEDEFAULT, NULL,NULL,GetModuleHandle(NULL),NULL); 
   
    if (AttachThreadInput(GetCurrentThreadId(),tid,TRUE) == 0) {
        printf ("Unable to attach thread input\n");
    }

}

unsigned int __stdcall dev_video_init_ex(void *param) {
    int    i,j;
    int    keycode;
    int    tmp_array[0x80];
    int    *waitVar=(int *)param;
    MSG    msg;
    HACCEL hAccel;
    word32 *ptr;

    WNDCLASS wc; 
    LPBITMAPINFOHEADER pDib;
    TEXTMETRIC tm;  
    int iStatusWidths[] = {60, 100,200,300, -1};

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

    // Initialize Common Controls;
    InitCommonControls();

    // Create a window
    wc.style = 0; 
    wc.lpfnWndProc = (WNDPROC) A2WndProc; 
    wc.cbClsExtra = 0; 
    wc.cbWndExtra = 0; 
    wc.hInstance =  GetModuleHandle(NULL);
    wc.hIcon = LoadIcon((HINSTANCE) GetModuleHandle(NULL), 
               MAKEINTRESOURCE(IDC_KEGS32)); 
    wc.hCursor = LoadCursor((HINSTANCE) NULL, IDC_ARROW); 
    wc.hbrBackground = GetStockObject(WHITE_BRUSH); 
    wc.lpszMenuName =  MAKEINTRESOURCE(IDC_KEGS32);
    wc.lpszClassName = _T("Kegs32"); 

    printf ("Registering Window\n");
    if (!RegisterClass(&wc)) {
        printf ("Register window failed\n");
        return 0; 
    } 
 
    // Create the main window. 

	printf ("Creating Window\n");
    hwndMain = CreateWindow(
       _T("Kegs32"), 
       _T("Kegs32 - GS Emulator"), 
       WS_OVERLAPPEDWINDOW&(~(WS_MAXIMIZEBOX | WS_SIZEBOX)), 
       CW_USEDEFAULT,CW_USEDEFAULT, 
       CW_USEDEFAULT,CW_USEDEFAULT,NULL,NULL,GetModuleHandle(NULL),NULL);
      
    // If the main window cannot be created, terminate 
    // the application. 
 
    if (!hwndMain) {
        printf ("Window create failed\n");
        return 0;
    }

    // Loading Accelerators
    
    hAccel=LoadAccelerators(GetModuleHandle(NULL),
                            MAKEINTRESOURCE(IDR_ACCEL));

    // Create Toolbar	
    win_toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                  WS_CHILD | WS_VISIBLE |  CCS_ADJUSTABLE | TBSTYLE_TOOLTIPS,
                  0, 0, 0, 0,
                  hwndMain,(HMENU)IDC_KEGS32, GetModuleHandle(NULL), NULL);
    SendMessage(win_toolbar,TB_BUTTONSTRUCTSIZE,(WPARAM)sizeof(TBBUTTON),0);
  
    {
        TBADDBITMAP tbab;
        TBBUTTON tbb[5];
        HWND hwndToolTip;
        int i,j;
        int idCmd[]={ID_FILE_DISK,-1,
                     ID_SPEED_1MHZ,ID_SPEED_2MHZ,ID_SPEED_FMHZ};

        ZeroMemory(tbb, sizeof(tbb));

        for (i=0,j=0;i<sizeof(tbb)/sizeof(TBBUTTON);i++) {
            if (idCmd[i] <0) {
                tbb[i].fsStyle = TBSTYLE_SEP;
            } else {
                tbb[i].iBitmap = j;
                tbb[i].idCommand = idCmd[i];
                tbb[i].fsState = TBSTATE_ENABLED;
                tbb[i].fsStyle = TBSTYLE_BUTTON;
                j++;
            }
        }

        tbab.hInst = GetModuleHandle(NULL);
        tbab.nID = IDC_KEGS32;
        SendMessage(win_toolbar, TB_ADDBITMAP,j, (LPARAM)&tbab);
        SendMessage(win_toolbar, TB_ADDBUTTONS,i, (LPARAM)&tbb);
		GetWindowRect(win_toolbar,&win_toolHeight);

        // Activate the tooltip
        hwndToolTip = (HWND) SendMessage(win_toolbar,TB_GETTOOLTIPS,0L,0L);
        if (hwndToolTip) {
            SendMessage(hwndToolTip,TTM_ACTIVATE,TRUE,0L);
        }
    }
  
    // Create Status
    win_status  = CreateWindowEx(0, STATUSCLASSNAME, NULL,
                  WS_CHILD | WS_VISIBLE , 0, 0, 0, 0,
                  hwndMain, (HMENU)ID_STATUSBAR, GetModuleHandle(NULL), NULL);
    SendMessage(win_status, SB_SETPARTS, 5, (LPARAM)iStatusWidths);
    GetWindowRect(win_status,&win_statHeight);

    SendMessage(win_toolbar,TB_AUTOSIZE,0,0);
    SendMessage(win_status,WM_SIZE,0,0);

    #if defined(KEGS_DIRECTDRAW) 
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
    ximage_superhires=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,
                                       DIB_RGB_COLORS,
                                       (VOID **)&data_superhires,NULL,0);
    pDib->biWidth = EFF_BORDER_WIDTH;
    ximage_border_sides=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,
                                         DIB_RGB_COLORS,
                                         (VOID **)&data_border_sides,NULL,0);
    pDib->biWidth = (X_A2_WINDOW_WIDTH);
    pDib->biHeight = - (X_A2_WINDOW_HEIGHT - A2_WINDOW_HEIGHT + 2*8);
    ximage_border_special=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,
                                           DIB_RGB_COLORS,
                                           (VOID **)&data_border_special,
                                           NULL,0);
    pDib->biWidth    = 8;
    pDib->biHeight   = 8;
	pDib->biBitCount = 32;
    ximage_interlace=CreateDIBSection(hdcRef,(BITMAPINFO *)pDib,
                                      DIB_RGB_COLORS,
                                      (VOID **)&data_interlace,
                                      NULL,0);
    GlobalFree(pDib);

    // Initialize interlace pattern
	ptr = (word32 *) &(data_interlace[0]);

	for (j=0;j<8;j+=2) {
		for (i=0;i<8;i++) {
			ptr[i+j*8]=RGB(255,255,255);
		}
    }

	hbr=CreatePatternBrush(ximage_interlace);
    
    initWindow(hwndMain);
    ShowWindow(hwndMain, SW_SHOWDEFAULT); 
    UpdateWindow(hwndMain); 
    *waitVar=1;

    while (GetMessage(&msg,NULL,0,0)>0) {
        if (!(TranslateAccelerator(hwndMain,hAccel,&msg)))
            TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    }
    my_exit(0);
    return 0;
}

void update_colormap(int mode) {
    
}

void redraw_border(void) {

}

void check_input_events(void) {
    MSG msg;
   
    while (win_pause) {
        // Flush out all events
        if (PeekMessage(&msg,NULL,0,0,PM_NOREMOVE)) {
            GetMessage(&msg,NULL,0,0);
        }
        Sleep(1);
    }

    // Try to update joystick buttons 
    joystick_update_button(); 

    if (PeekMessage(&msg,hwndHidden,0,0,PM_NOREMOVE)) {
        do {
            if (GetMessage(&msg,hwndHidden,0,0)>0) {
                TranslateMessage(&msg); 
                DispatchMessage(&msg); 
            } else {
                my_exit(0);
            }
        } while (PeekMessage(&msg,hwndHidden,0,0,PM_NOREMOVE));
    }
}

void update_status_line(int line, const char *string) {
    char    *buf;
    const char *ptr;
    int    i;

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
    char *buf;
    TCHAR buffer[255]; 
    int height=dwchary;
    int centerx,centery;

    centerx=centery=0;
    win_centroid(&centerx,&centery);
    if (!g_fullscreen) {
        centery+=28;
    }
    if (win_stat_debug) {
        for(line = 0; line < MAX_STATUS_LINES; line++) {
            buf = &(g_status_buf[line][0]);
            TextOutA(hdcRef, 0+centerx, X_A2_WINDOW_HEIGHT+
                     height*line+centery,buf, strlen(buf));
        }
    }
	
    if (win_status !=NULL && !g_fullscreen) {
        SendMessage(win_status, SB_SETTEXT,0,(LPARAM)_T("KEGS 0.60"));
        _stprintf(buffer,_T("Vol:%d"),g_doc_vol);
        SendMessage(win_status, SB_SETTEXT,1,(LPARAM)buffer);
        buf=strstr(g_status_buf[0],"sim MHz:");
        
        if (buf) {
            memset(buffer,0,255*sizeof(TCHAR));
            if (sizeof(TCHAR) <2) {
                strncpy((char *)buffer,buf,strchr(buf+8,' ')-buf);
            } else {
                mbstowcs((wchar_t *)buffer,buf,strchr(buf+8,' ')-buf);
            }
            SendMessageA(win_status, SB_SETTEXT,2,(LPARAM) buffer);
        } else {
            SendMessageA(win_status,SB_SETTEXT,2,(LPARAM)_T("sim MHz:???"));
        }
		
        buf=strstr(g_status_buf[0],"Eff MHz:");
        if (buf) {
            memset(buffer,0,255*sizeof(TCHAR));
            if (sizeof(TCHAR) <2) {
                strncpy((char *)buffer,buf,strchr(buf+8,',')-buf);           
            } else {
                mbstowcs((wchar_t *)buffer,buf,strchr(buf+8,',')-buf);
            }
            SendMessageA(win_status, SB_SETTEXT,3,(LPARAM) buffer);
        } else {
            SendMessageA(win_status,SB_SETTEXT,3,(LPARAM)_T("Eff MHz:???"));
        }
        
        buf=strstr(g_status_buf[5],"fast");
        if (buf) {
            memset(buffer,0,255*sizeof(TCHAR));
            if (sizeof(TCHAR) <2) {
                strncpy((char *)buffer,g_status_buf[5],
                        buf-&(g_status_buf[5][0]));
            } else {
                mbstowcs((wchar_t *)buffer,g_status_buf[5],
                         buf-&(g_status_buf[5][0]));
            }
            SendMessageA(win_status, SB_SETTEXT,4,(LPARAM) buffer);
        } else {
           
        }
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
