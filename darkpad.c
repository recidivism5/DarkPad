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
HFONT font,menufont;
void writeNum(i32 i){
	u8 a[256];
	intToAscii(i,a);
	WriteConsoleA(consoleOut,a,StringLength(a),0,0);
	WriteConsoleA(consoleOut,"\r\n",2,0,0);
}
MENUINFO menuinfo = {sizeof(MENUINFO),MIM_BACKGROUND};
RECT MapRectFromClientToWndCoords(HWND hwnd, RECT r){
    MapWindowPoints(hwnd,0,&r,2);
    RECT s;
    GetWindowRect(hwnd,&s);
    OffsetRect(&r,-s.left,-s.top);
    return r;
}
RECT GetNonclientMenuBorderRect(HWND hwnd){
    RECT r;
    GetClientRect(hwnd,&r);
    r = MapRectFromClientToWndCoords(hwnd,r);
    int y = r.top - 1;
    return (RECT){
        r.left,
        y,
        r.right,
        y+1
    };
}
enum{
	AID_NEW=169,
	AID_OPEN,
	AID_SAVE,
	AID_SAVE_AS,
	AID_UNDO,
	AID_CUT,
	AID_COPY,
	AID_PASTE,
	AID_SELECT_ALL,
	AID_FIND,
	AID_WORD_WRAP,
	AID_FONT,
	AID_ZOOM_IN,
	AID_ZOOM_OUT,
};
i32 WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
		case WM_NCPAINT:
			LRESULT result = DefWindowProcA(wnd,WM_NCPAINT,wparam,lparam);
		case WM_SETFOCUS: case WM_KILLFOCUS:{
			MENUBARINFO mbi = {sizeof(mbi)};
			if (!GetMenuBarInfo(wnd,OBJID_MENU,0,&mbi)) return;
			HDC hdc = GetWindowDC(wnd);
			RECT r = GetNonclientMenuBorderRect(wnd);
			HBRUSH brush = CreateSolidBrush(RGB(40,40,40));
			FillRect(hdc,&r,brush);
			DeleteObject(brush);
			ReleaseDC(wnd,hdc);
			return result;
		}
		case WM_MEASUREITEM:{
			MEASUREITEMSTRUCT *mip = lparam;
			HDC hdc = GetDC(wnd);
			SelectObject(hdc,menufont);
			RECT r;
			DrawTextA(hdc,mip->itemData,-1,&r,DT_LEFT|DT_SINGLELINE|DT_CALCRECT);
			ReleaseDC(wnd,hdc);
			mip->itemWidth = r.right-r.left;
			mip->itemHeight = 0;
			return 1;
		}
		case WM_DRAWITEM:
			DRAWITEMSTRUCT *dip = lparam;
			HPEN oldPen = SelectObject(dip->hDC,GetStockObject(DC_PEN));
			HBRUSH oldBrush = SelectObject(dip->hDC,GetStockObject(DC_BRUSH));
			SetBkMode(dip->hDC,TRANSPARENT);
			SetTextColor(dip->hDC,RGB(255,255,255));
			if (dip->itemState & (ODS_HOTLIGHT|ODS_SELECTED)){
				SetDCPenColor(dip->hDC,RGB(70,70,70));
				SetDCBrushColor(dip->hDC,RGB(70,70,70));
			} else {
				SetDCPenColor(dip->hDC,RGB(40,40,40));
				SetDCBrushColor(dip->hDC,RGB(40,40,40));
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
			HMENU Format = CreateMenu();
			AppendMenuA(File,MF_STRING,AID_NEW,"New\tCtrl+N");
			AppendMenuA(File,MF_STRING,AID_OPEN,"Open\tCtrl+O");
			AppendMenuA(File,MF_STRING,AID_SAVE,"Save\tCtrl+S");
			AppendMenuA(File,MF_STRING,AID_SAVE_AS,"Save As\tCtrl+Shift+S");
			AppendMenuA(Edit,MF_STRING,AID_UNDO,"Undo\tCtrl+Z");
			AppendMenuA(Edit,MF_STRING,AID_CUT,"Cut\tCtrl+X");
			AppendMenuA(Edit,MF_STRING,AID_COPY,"Copy\tCtrl+C");
			AppendMenuA(Edit,MF_STRING,AID_PASTE,"Paste\tCtrl+V");
			AppendMenuA(Edit,MF_STRING,AID_SELECT_ALL,"Select All\tCtrl+A");
			AppendMenuA(Edit,MF_STRING,AID_FIND,"Find\tCtrl+F");
			AppendMenuA(Format,MF_STRING|MF_CHECKED,AID_WORD_WRAP,"Word Wrap\tAlt+Z");
			AppendMenuA(Format,MF_STRING,AID_FONT,"Font");
			AppendMenuA(Bar,MF_OWNERDRAW|MF_POPUP,File,"File");
			AppendMenuA(Bar,MF_OWNERDRAW|MF_POPUP,Edit,"Edit");
			AppendMenuA(Bar,MF_OWNERDRAW|MF_POPUP,Format,"Format");
			menuinfo.hbrBack = CreateSolidBrush(RGB(40,40,40));
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
		case WM_COMMAND:
			switch(LOWORD(wparam)){ 
                case AID_OPEN:
					IFileDialog *pfd;
					IShellItem *psi;
					PWSTR path = 0;
					if (SUCCEEDED(CoCreateInstance(&CLSID_FileOpenDialog,0,CLSCTX_INPROC_SERVER,&IID_IFileOpenDialog,&pfd))){
						pfd->lpVtbl->Show(pfd,wnd);
						if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd,&psi))){
							if (SUCCEEDED(psi->lpVtbl->GetDisplayName(psi,SIGDN_FILESYSPATH,&path))){
								/*WCHAR *p = path;
								FOR(i,MAX_PATH){
									if (!path[i]) break;
									gpath[i] = path[i];
								}
								loadFile(gpath);*/
								CoTaskMemFree(path);
							}
							psi->lpVtbl->Release(psi);
						}
						pfd->lpVtbl->Release(pfd);
					}
                default:
                	break;
            }
            return 0;
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

	NONCLIENTMETRICSA ncm = {sizeof(NONCLIENTMETRICSA)};
	SystemParametersInfoA(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICSA),&ncm,0);
	menufont = CreateFontIndirectA(&ncm.lfMenuFont);
	background = CreateSolidBrush(RGB(20,20,20));
	font = CreateFontA(-12,0,0,0,FW_DONTCARE,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_DONTCARE,"Consolas");
	wc.hIcon = LoadIconA(instance,MAKEINTRESOURCEA(RID_ICON));
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