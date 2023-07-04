/*
	BUG TRACKER:
	-crashes wcap when fullscreened
*/
#define WIN32_LEAN_AND_MEAN
#undef UNICODE
int _fltused;
#include <windows.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <shellapi.h>
#include <shobjidl_core.h>
#include <shlguid.h>
#include "res.h"
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define COUNT(arr) (sizeof(arr)/sizeof(*arr))
#define FOR(var,count) for(i32 var = 0; var < (count); var++)
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
char toUpper(char c){
	return c>='a' ? c-32 : c;
}
int StringsEqualInsensitive(char *a, char *b){
	while (*a && *b && toUpper(*a)==toUpper(*b)){
		a++;
		b++;
	}
	return *a==*b;
}
i32 StringsEqual(u8 *a, u8 *b){
	while (*a && *b && *a==*b){
		a++;
		b++;
	}
	return *a==*b;
}
i32 StringsEqualWide(u16 *a, u16 *b){
	while (*a && *b && *a==*b){
		a++;
		b++;
	}
	return *a==*b;
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
u8 *loadFileString(u8 *path, u32 *size){
	HANDLE f = CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	*size = GetFileSize(f,0)+1;
	u8 *b = HeapAlloc(heap,0,*size);
	u32 bytesRead = 0;
	ReadFile(f,b,*size,&bytesRead,0);
	CloseHandle(f);
	b[*size-1] = 0;
	return b;
}
void intToAscii(i32 i, u8 *a){
	u8 *p = a;
	do {
		*p++ = '0'+i%10;
		i/=10;
	} while (i);
	*p = 0;
	p--;
	while (a < p){
		u8 c = *a;
		*a++ = *p;
		*p-- = c;
	}
}
i32 dpi;
i32 dpiScale(i32 val){
    return (i32)((float)val * dpi / 96);
}
HINSTANCE instance;
HWND gwnd,gedit;
HANDLE consoleOut;
HBRUSH background;
HFONT font;
void writeNum(i32 i){
	u8 a[256];
	intToAscii(i,a);
	WriteConsoleA(consoleOut,a,StringLength(a),0,0);
	WriteConsoleA(consoleOut,"\r\n",2,0,0);
}
MENUINFO menuinfo = {sizeof(MENUINFO),MIM_BACKGROUND};
i32 WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
		case WM_MEASUREITEM:
			MEASUREITEMSTRUCT *mip = lparam;
			mip->itemWidth = 30;
			mip->itemHeight = 0;
			return 1;
		case WM_DRAWITEM:
			DRAWITEMSTRUCT *dip = lparam;
			HPEN oldPen = SelectObject(dip->hDC,GetStockObject(DC_PEN));
			HBRUSH oldBrush = SelectObject(dip->hDC,GetStockObject(DC_BRUSH));
			SetBkMode(dip->hDC,TRANSPARENT);
			SetTextColor(dip->hDC,RGB(255,255,255));
			if (dip->itemState & ODS_HOTLIGHT){
				SetDCPenColor(dip->hDC,RGB(70,70,70));
				SetDCBrushColor(dip->hDC,RGB(70,70,70));
			} else {
				SetDCPenColor(dip->hDC,RGB(20,20,20));
				SetDCBrushColor(dip->hDC,RGB(20,20,20));
			}
			Rectangle(dip->hDC,dip->rcItem.left,dip->rcItem.top,dip->rcItem.right,dip->rcItem.bottom);
			DrawTextA(dip->hDC,dip->itemData,-1,&dip->rcItem,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			SelectObject(dip->hDC,oldPen);
			SelectObject(dip->hDC,oldBrush); 
			return 1;
		case WM_CREATE:
			HMENU Bar = CreateMenu();
			HMENU File = CreateMenu();
			HMENU Edit = CreateMenu();
			AppendMenuA(Bar,MF_OWNERDRAW,File,"File");
			AppendMenuA(Bar,MF_OWNERDRAW,Edit,"Edit");
			menuinfo.hbrBack = background;
			SetMenuInfo(Bar,&menuinfo);
			SetMenu(wnd,Bar);
			i32 t = 1;
			DwmSetWindowAttribute(wnd,20,&t,sizeof(t));
			gedit = CreateWindowExA(0,"EDIT",0,WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL,0,0,0,0,wnd,0,instance,0);
			SendMessageA(gedit,WM_SETFONT,font,0);
			u32 size;
			u8 *test = loadFileString("darkpad.c",&size);
            SendMessageA(gedit,WM_SETTEXT,0,test);//WM_SETTEXT expects CRLF
			free(test);
			break;
		case WM_SETFOCUS:
            SetFocus(gedit);
            return 0;
        case WM_SIZE:
            MoveWindow(gedit,0,0,LOWORD(lparam),HIWORD(lparam),1);
            return 0;
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_CTLCOLOREDIT:
			SetTextColor(wparam,0xcccccc);
			SetBkColor(wparam,RGB(20,20,20));
			return background;
	}
	return DefWindowProcA(wnd,msg,wparam,lparam);
}
WNDCLASSA wc = {0,WindowProc,0,0,0,0,0,0,0,"DarkPad"};
HTHEME(*OpenNcThemeData)(HWND wnd, LPCWSTR classList);
HTHEME customOpenThemeData(HWND wnd, LPCWSTR classList){
	if (StringsEqualWide(classList,L"ScrollBar")){
		wnd = 0;
		classList = L"Explorer::ScrollBar";
	}
	return OpenNcThemeData(wnd,classList);
}
void WinMainCRTStartup(){
	instance = GetModuleHandleA(0);
	heap = GetProcessHeap();
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	HMODULE uxtheme = LoadLibraryExW(L"uxtheme.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);
	OpenNcThemeData = GetProcAddress(uxtheme,MAKEINTRESOURCEA(49));
	((i32 (*)(i32))GetProcAddress(uxtheme,MAKEINTRESOURCEA(135)))(1);
	((void (*)())GetProcAddress(uxtheme, MAKEINTRESOURCEA(104)))();
	u64 comctl = LoadLibraryExW(L"comctl32.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);
	PIMAGE_DELAYLOAD_DESCRIPTOR imports = comctl+((PIMAGE_NT_HEADERS)(comctl+((PIMAGE_DOS_HEADER)comctl)->e_lfanew))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;
	while (!StringsEqualInsensitive(comctl+imports->DllNameRVA,"uxtheme.dll")) imports++;
	PIMAGE_THUNK_DATA impName = comctl+imports->ImportNameTableRVA;
	PIMAGE_THUNK_DATA impAddr = comctl+imports->ImportAddressTableRVA;
	while (!(IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal)==49)){
		impName++;
		impAddr++;
	}
	DWORD oldProtect;
	VirtualProtect(impAddr,sizeof(IMAGE_THUNK_DATA),PAGE_READWRITE,&oldProtect);
	impAddr->u1.Function = customOpenThemeData;
	VirtualProtect(impAddr,sizeof(IMAGE_THUNK_DATA),oldProtect,&oldProtect);

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