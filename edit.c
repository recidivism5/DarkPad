/*
	No more OOP.
	We want full dpi compliance. I guess. Fuck.
	No variable width fonts.
	Every operation is a string, applied to a set of positions.
	First we're gonna try expanded tabs.
	Should we be using ExcludeRect for the menus or child windows?
	ComCtl Edit Control.

	Responsiveness:
	-WS_EX_COMPOSITED makes the window more responsive, but only if you don't resize it.
	-WS_EX_LAYERED plus SetLayeredWindowAttributes(gwnd,1,255,LWA_COLORKEY) makes window more responsive.
		Interestingly, LWA_ALPHA does not give responsiveness.
		The alpha value is ignored but might as well give 255 for good luck.
		The color key must be something other than 0, otherwise the border hit detection will be all fucked up, I guess 'cause there's black pixels on it.
		Whatever color key you choose, that's a banned color for your whole app.
	-Both still require using BufferedPaint to eliminate flickering.

	BUG TRACKER:
	-crashes wcap when fullscreened
*/
#define WIN32_LEAN_AND_MEAN
#undef UNICODE
int _fltused;
#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <shellapi.h>
#include <shobjidl_core.h>
#include <shlguid.h>
#include "res.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COUNT(arr) (sizeof(arr)/sizeof(*arr))
#define FOR(var,count) for(i32 var = 0; var < (count); var++)
#define EXCLUDE_RECT(hdc, r) (ExcludeClipRect((hdc),(r).left,(r).top,(r).right,(r).bottom))
void *memset(u8 *dst, int c, size_t size){
	while (size--) *dst++ = c;
	return dst;
}
void *memcpy(u8 *dst, u8 *src, size_t size){
	while (size--) *dst++ = *src++;
	return dst;
}
i32 memcmp(u8 *s1, u8 *s2, i32 len){
	for (int i = 0; i < len; i++){
		if (s1[i]!=s2[i]) return 1;
	}
	return 0;
}
void MoveMem(u8 *dst, u8 *src, size_t size){
	if (dst > src){
		dst += size-1;
		src += size-1;
		while (size--){
			*dst-- = *src--;
		}
	} else memcpy(dst,src,size);
}
u8 *CopyString(u8 *dst, u8 *src){
	while (*src) *dst++ = *src++;
	*dst = 0;
	return dst;
}
i32 StringLength(u8 *s){
	i32 n = 0;
	while (*s){
		n++;
		s++;
	}
	return n;
}
HANDLE heap;
#define calloc(count,size) HeapAlloc(heap,HEAP_ZERO_MEMORY,(count)*(size))
#define free(ptr) HeapFree(heap,0,(ptr))
#define abs(x) ((x)<0 ? -(x) : (x))
#define malloc(size) HeapAlloc(heap,0,(size))
#define realloc(ptr,size) HeapReAlloc(heap,0,(ptr),(size))
u8 *loadFile(u8 *path, u32 *size){
	HANDLE f = CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	*size = GetFileSize(f,0);
	u8 *b = HeapAlloc(heap,0,*size);
	u32 bytesRead = 0;
	ReadFile(f,b,*size,&bytesRead,0);
	CloseHandle(f);
	return b;
}
i32 dpi;
i32 dpiScale(i32 val){
    return (i32)((float)val * dpi / 96);
}
u8 testString[] = "Lorem ipsum dolor sit amet, consectetur "
				  "adipisicing elit, sed do eiusmod tempor "
				  "incididunt ut labore et dolore magna "
				  "aliqua. Ut enim ad minim veniam, quis "
				  "nostrud exercitation ullamco laboris nisi "
				  "ut aliquip ex ea commodo consequat. Duis "
				  "aute irure dolor in reprehenderit in "
				  "voluptate velit esse cillum dolore eu "
				  "fugiat nulla pariatur. Excepteur sint "
				  "occaecat cupidatat non proident, sunt "
				  "in culpa qui officia deserunt mollit "
				  "anim id est laborum.";
HINSTANCE instance;
HWND gwnd,gedit;
HBRUSH background;
HFONT font;
i32 __stdcall WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
		case WM_CREATE:{
			gedit = CreateWindowEx(0,"EDIT",0,WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL,0,0,0,0,wnd,0,instance,0);
			SendMessage(gedit,WM_SETFONT,font,0);
            SendMessage(gedit,WM_SETTEXT,0,testString);
			break;
		}
		case WM_SETFOCUS:
            SetFocus(gedit);
            return 0;
        case WM_SIZE:
            MoveWindow(gedit,0,0,LOWORD(lparam),HIWORD(lparam),1);
            return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_CTLCOLOREDIT:{
			HDC hdc = wparam;
			SetTextColor(hdc,RGB(255,255,255));
			SetBkColor(hdc,RGB(20,20,20));
			return background;
		}
	}
	return DefWindowProcA(wnd,msg,wparam,lparam);
}
WNDCLASSA wc = {CS_HREDRAW|CS_VREDRAW,WindowProc,0,0,0,0,0,0,0,"DarkPad"};
void __stdcall WinMainCRTStartup(){
	instance = GetModuleHandleA(0);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	heap = GetProcessHeap();
	background = CreateSolidBrush(RGB(20,20,20));
	font = CreateFontA(-12,0,0,0,FW_DONTCARE,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_DONTCARE,"Consolas");
	RegisterClassA(&wc);
	RECT wr = {0,0,800,600};
	AdjustWindowRect(&wr,WS_OVERLAPPEDWINDOW,FALSE);
	i32 wndWidth = wr.right-wr.left;
	i32 wndHeight = wr.bottom-wr.top;
	gwnd = CreateWindowExA(WS_EX_APPWINDOW,wc.lpszClassName,wc.lpszClassName,WS_OVERLAPPEDWINDOW|WS_VISIBLE,GetSystemMetrics(SM_CXSCREEN)/2-wndWidth/2,GetSystemMetrics(SM_CYSCREEN)/2-wndHeight/2,wndWidth,wndHeight,0,0,instance,0);
	MSG msg;
	while (GetMessageA(&msg,0,0,0)){
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
	}
	ExitProcess(msg.wParam);
}