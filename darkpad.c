#define WIN32_LEAN_AND_MEAN
int _fltused;
#include <windows.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <shellapi.h>
#include <shobjidl_core.h>
#include <shlguid.h>
#include <commdlg.h>
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
u16 *CopyString(u16 *dst, u16 *src){
	while (*src) *dst++ = *src++;
	*dst = 0;
	return dst;
}
i64 StringLength(u16 *s){
	i64 n = 0;
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
i32 dpi;
i32 dpiScale(i32 val){
    return (i32)((float)val * dpi / 96);
}
HINSTANCE instance;
HWND gwnd,gedit;
HANDLE consoleOut;
HBRUSH bBackground,bMenuBackground,bOutline;
HFONT font,menufont;
u16 gpath[MAX_PATH+4];
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
	AID_NEW_WINDOW,
	AID_OPEN,
	AID_SAVE,
	AID_SAVE_AS,
	AID_UNDO,
	AID_CUT,
	AID_COPY,
	AID_PASTE,
	AID_DELETE,
	AID_SELECT_ALL,
	AID_FIND,
	AID_REPLACE,
	AID_WORD_WRAP,
	AID_FONT,
	AID_ZOOM_IN,
	AID_ZOOM_OUT,
	AID_RESET_ZOOM,
	AID_STATUS_BAR,
};
ACCEL accels[]={
    FCONTROL|FVIRTKEY,'N',AID_NEW,
    FCONTROL|FVIRTKEY,'O',AID_OPEN,
    FCONTROL|FVIRTKEY,'S',AID_SAVE,
    FCONTROL|FSHIFT|FVIRTKEY,'S',AID_SAVE_AS,
    FCONTROL|FVIRTKEY,'F',AID_FIND,
    //FCONTROL|FVIRTKEY,'G',AID_GO_TO_LINE,
    FCONTROL|FVIRTKEY,'A',AID_SELECT_ALL,
    //FCONTROL|FVIRTKEY,'T',AID_INSERT_TIMESTAMP,
    FALT|FVIRTKEY,'Z',AID_WORD_WRAP,
};
i32 starred;
void updateTitle(){
	u16 title[MAX_PATH+32];
	CopyString(CopyString(title,starred ? L"DarkPad - *" : L"DarkPad - "),*gpath ? gpath : L"Untitled");
	SetWindowTextW(gwnd,title);
}
void loadFile(u16 *path){
	HANDLE hfile = CreateFileW(path,GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (hfile == INVALID_HANDLE_VALUE){
		MessageBoxW(gwnd,L"File not found.",L"Error",0);
		return;
	}
	u32 size = GetFileSize(hfile,0);
	u8 *file = HeapAlloc(heap,0,size);
	ReadFile(hfile,file,size,0,0);
	CloseHandle(hfile);
	u32 total = 4, used = 0;
	u8 *str = HeapAlloc(heap,0,total);
	u8 *f = file;
	while (f < file+size){
		if (used+2 > total){
			while (used+2 > total) total *= 2;
			str = HeapReAlloc(heap,0,str,total);
		}
		if (*f=='\r' || *f=='\n'){
			str[used++] = '\r';
			str[used++] = '\n';
		} else str[used++] = *f;
		f += 1 + (*f=='\r');
	}
	if (used==total){
		total++;
		str = HeapReAlloc(heap,0,str,total);
	}
	str[used++] = 0;
	i32 wlen = MultiByteToWideChar(CP_UTF8,MB_PRECOMPOSED,str,used,0,0);
	u16 *w = HeapAlloc(heap,0,wlen*sizeof(u16));
	MultiByteToWideChar(CP_UTF8,MB_PRECOMPOSED,str,used,w,wlen);
	SendMessageW(gedit,WM_SETTEXT,0,w);
	HeapFree(heap,0,file);
	HeapFree(heap,0,str);
	HeapFree(heap,0,w);
	CopyString(gpath,path);
	starred = 0;
	updateTitle();
}
void saveFile(u16 *path){
	i64 len = SendMessageW(gedit,WM_GETTEXTLENGTH,0,0);
	u16 *buf = HeapAlloc(heap,0,len*sizeof(u16));
	len = SendMessageW(gedit,WM_GETTEXT,len,buf);
	i32 mlen = WideCharToMultiByte(CP_UTF8,0,buf,len,0,0,0,0);
	u8 *m = HeapAlloc(heap,0,mlen);
	WideCharToMultiByte(CP_UTF8,0,buf,len,m,mlen,0,0);
	HANDLE hfile = CreateFileW(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	WriteFile(hfile,m,mlen,0,0);
	CloseHandle(hfile);
	HeapFree(heap,0,buf);
	HeapFree(heap,0,m);
	CopyString(gpath,path);
	starred = 0;
	updateTitle();
}
i64 FindProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
		case WM_INITDIALOG:
			i32 t = 1;
			DwmSetWindowAttribute(wnd,20,&t,sizeof(t));
			return 0;
		case WM_CTLCOLORDLG: case WM_CTLCOLORSTATIC:
			SetBkMode(wparam,TRANSPARENT);
			SetTextColor(wparam,RGB(255,255,255));
			return bBackground;
		case WM_CTLCOLOREDIT:
			SetTextColor(wparam,0xffffff);
			SetBkColor(wparam,RGB(20,20,20));
			return bBackground;
	}
	return 0;
}
i64 WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	if (0){
		FINDREPLACEW *fr = lparam;
		if (fr->Flags & FR_FINDNEXT){
			WriteConsoleW(consoleOut,L"FUCK",4,0,0);
			i64 len = SendMessageW(gedit,WM_GETTEXTLENGTH,0,0) + 1;
			u16 *buf = HeapAlloc(heap,0,len*sizeof(u16));
			SendMessageW(gedit,WM_GETTEXT,len,buf);
			DWORD starti,endi;
			SendMessageW(gedit,EM_GETSEL,&starti,&endi);
			/*
				1: go along checking for first char.
				2: once found, check end char. if not equal go to 1.
				3: check the chars in between.
			*/
			/*i64 whatlen = StringLength(fr->lpstrFindWhat);
			u16 *start = buf+starti;
			u16 *s = start;
			u16 *end = buf+endi+1;
			i32 searching = 1;
			while (searching){
				while (s!=end && *s!=fr->lpstrFindWhat[0]){
					if (s==start) break; //not found
					s++;
				}
				if (s==end) s = buf;
				else {
					if (end-s >= whatlen && s[whatlen-1]==fr->lpstrFindWhat[whatlen-1]){
						for (i64 i = 1; i < whatlen; i++){
							if (s[i]!=fr->lpstrFindWhat[i]) goto KEK;
						}
						//ExitProcess(1);
						//SendMessageW(gedit,EM_SETSEL,s-buf,s+whatlen-buf);
						SendMessageW(gedit,EM_SETSEL,0,-1);
						searching = 0;
						KEK:;
					}
					s++;
				}
			}*/
			SendMessageW(gedit,EM_SETSEL,0,-1);
			HeapFree(heap,0,buf);
		}
		return 0;
	} else switch (msg){
		case WM_NCPAINT:
			LRESULT result = DefWindowProcW(wnd,WM_NCPAINT,wparam,lparam);
		case WM_SETFOCUS: SetFocus(gedit); case WM_KILLFOCUS:{
			MENUBARINFO mbi = {sizeof(mbi)};
			if (!GetMenuBarInfo(wnd,OBJID_MENU,0,&mbi)) return;
			RECT r, wr;
			GetClientRect(wnd,&r);
			MapWindowPoints(wnd,0,&r,2);
			GetWindowRect(wnd,&wr);
			OffsetRect(&r,-wr.left,-wr.top);
			r.top--;
			r.bottom = r.top+1;
			HDC hdc = GetWindowDC(wnd);
			FillRect(hdc,&r,bMenuBackground);
			ReleaseDC(wnd,hdc);
			return result;
		}
		case WM_MEASUREITEM:{
			MEASUREITEMSTRUCT *mip = lparam;
			HDC hdc = GetDC(wnd);
			SelectObject(hdc,menufont);
			RECT r;
			DrawTextW(hdc,mip->itemData,-1,&r,DT_LEFT|DT_SINGLELINE|DT_CALCRECT);
			ReleaseDC(wnd,hdc);
			mip->itemWidth = r.right-r.left;
			mip->itemHeight = 0;
			return 1;
		}
		case WM_DRAWITEM:{
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
			DrawTextW(dip->hDC,dip->itemData,-1,&dip->rcItem,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
			SelectObject(dip->hDC,oldPen);
			SelectObject(dip->hDC,oldBrush); 
			return 1;
		}
		case WM_CREATE:{
			gwnd = wnd;
			updateTitle();
			HMENU Bar = CreateMenu();
			HMENU File = CreateMenu();
			HMENU Edit = CreateMenu();
			HMENU Format = CreateMenu();
			HMENU View = CreateMenu();
			AppendMenuW(File,MF_STRING,AID_NEW,L"New\tCtrl+N");
			AppendMenuW(File,MF_STRING,AID_NEW_WINDOW,L"New Window\tCtrl+Shift+N");
			AppendMenuW(File,MF_STRING,AID_OPEN,L"Open\tCtrl+O");
			AppendMenuW(File,MF_STRING,AID_SAVE,L"Save\tCtrl+S");
			AppendMenuW(File,MF_STRING,AID_SAVE_AS,L"Save As\tCtrl+Shift+S");
			AppendMenuW(Edit,MF_STRING,AID_UNDO,L"Undo\tCtrl+Z");
			AppendMenuW(Edit,MF_STRING,AID_CUT,L"Cut\tCtrl+X");
			AppendMenuW(Edit,MF_STRING,AID_COPY,L"Copy\tCtrl+C");
			AppendMenuW(Edit,MF_STRING,AID_PASTE,L"Paste\tCtrl+V");
			AppendMenuW(Edit,MF_STRING,AID_DELETE,L"Delete\tDel");
			AppendMenuW(Edit,MF_STRING,AID_SELECT_ALL,L"Select All\tCtrl+A");
			AppendMenuW(Edit,MF_SEPARATOR,0,0);
			AppendMenuW(Edit,MF_STRING,AID_FIND,L"Find And Replace\tCtrl+F");
			AppendMenuW(Edit,MF_STRING,AID_REPLACE,L"Replace\tCtrl+R");
			AppendMenuW(Format,MF_STRING,AID_WORD_WRAP,L"Word Wrap\tAlt+Z");
			AppendMenuW(Format,MF_STRING,AID_FONT,L"Font");
			AppendMenuW(View,MF_STRING,AID_ZOOM_IN,L"Zoom In\tCtrl+Plus");
			AppendMenuW(View,MF_STRING,AID_ZOOM_OUT,L"Zoom Out\tCtrl+Minus");
			AppendMenuW(View,MF_STRING,AID_RESET_ZOOM,L"Reset Zoom\tCtrl+0");
			AppendMenuW(View,MF_SEPARATOR,0,0);
			AppendMenuW(View,MF_STRING,AID_STATUS_BAR,L"Status Bar");
			AppendMenuW(Bar,MF_OWNERDRAW|MF_POPUP,File,L"File");
			AppendMenuW(Bar,MF_OWNERDRAW|MF_POPUP,Edit,L"Edit");
			AppendMenuW(Bar,MF_OWNERDRAW|MF_POPUP,Format,L"Format");
			AppendMenuW(Bar,MF_OWNERDRAW|MF_POPUP,View,L"View");
			MENUINFO menuinfo = {0};
			menuinfo.cbSize = sizeof(MENUINFO);
			menuinfo.fMask = MIM_BACKGROUND;
			menuinfo.hbrBack = bMenuBackground;
			SetMenuInfo(Bar,&menuinfo);
			SetMenu(wnd,Bar);
			i32 t = 1;
			DwmSetWindowAttribute(wnd,20,&t,sizeof(t));
			gedit = CreateWindowExW(0,L"EDIT",0,WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL|ES_NOHIDESEL,0,0,0,0,wnd,0,instance,0);
			SendMessageW(gedit,WM_SETFONT,font,0);
			SendMessageW(gedit,EM_SETLIMITTEXT,0x80000000,0);
			break;
		}
        case WM_SIZE:{
            MoveWindow(gedit,0,0,LOWORD(lparam),HIWORD(lparam)-22,1);
			RECT r = {0,HIWORD(lparam)-22,LOWORD(lparam),HIWORD(lparam)};
			InvalidateRect(wnd,&r,0);
            return 0;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_CTLCOLOREDIT:
			SetTextColor(wparam,0xcccccc);
			SetBkColor(wparam,RGB(20,20,20));
			return bBackground;
		case WM_PAINT:{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(wnd,&ps);
			SelectObject(hdc,menufont);
			SetBkMode(hdc,TRANSPARENT);
			SetTextColor(hdc,RGB(255,255,255));
			RECT r;
			GetClientRect(wnd,&r);
			r.top = r.bottom-22;
			FillRect(hdc,&r,bMenuBackground);
			EndPaint(wnd,&ps);
			return 0;
		}
		case WM_COMMAND:
			if (!starred && HIWORD(wparam)==EN_CHANGE){
				starred = 1;
				updateTitle();
			} else switch(LOWORD(wparam)){
				case AID_NEW:{
					u16 zero = 0;
					SendMessageW(gedit,WM_SETTEXT,0,&zero);
					gpath[0] = 0;
					starred = 0;
					updateTitle();
					break;
				}
                case AID_OPEN:{
					IFileDialog *pfd;
					IShellItem *psi;
					PWSTR path = 0;
					if (SUCCEEDED(CoCreateInstance(&CLSID_FileOpenDialog,0,CLSCTX_INPROC_SERVER,&IID_IFileOpenDialog,&pfd))){
						pfd->lpVtbl->Show(pfd,wnd);
						if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd,&psi))){
							if (SUCCEEDED(psi->lpVtbl->GetDisplayName(psi,SIGDN_FILESYSPATH,&path))){
								loadFile(path);
								CoTaskMemFree(path);
							}
							psi->lpVtbl->Release(psi);
						}
						pfd->lpVtbl->Release(pfd);
					}
					break;
				}
				case AID_SAVE:{
					if (*gpath && starred){
						saveFile(gpath);
						break;
					}
				}
				case AID_SAVE_AS:{
					IFileDialog *pfd;
					IShellItem *psi;
					PWSTR path = 0;
					if (SUCCEEDED(CoCreateInstance(&CLSID_FileSaveDialog,0,CLSCTX_INPROC_SERVER,&IID_IFileSaveDialog,&pfd))){
						pfd->lpVtbl->Show(pfd,wnd);
						if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd,&psi))){
							if (SUCCEEDED(psi->lpVtbl->GetDisplayName(psi,SIGDN_FILESYSPATH,&path))){
								saveFile(path);
								CoTaskMemFree(path);
							}
							psi->lpVtbl->Release(psi);
						}
						pfd->lpVtbl->Release(pfd);
					}
					break;
				}
				case AID_FONT:{
					LOGFONTW lf;
					CHOOSEFONTW cf = {sizeof(CHOOSEFONTW),wnd,0,&lf,0,0,0,0,0,0,0,0,0,0,0,0};
					if (!ChooseFontW(&cf)) break;
					DeleteObject(font);
					font = CreateFontIndirectW(&lf);
					SendMessageW(gedit,WM_SETFONT,font,0);
					HANDLE hfile = CreateFileW(L"font",GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
					if (hfile==INVALID_HANDLE_VALUE){
						wchar_t buf[256];
						FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,0,GetLastError(),MAKELANGID(LANG_NEUTRAL,SUBLANG_DEFAULT),buf,(sizeof(buf)/sizeof(wchar_t)),0);
						MessageBoxW(wnd,buf,L"Cannot open font cache file.",0);
						break;
					}
					WriteFile(hfile,&lf,sizeof(lf),0,0);
					CloseHandle(hfile);
					break;
				}
				case AID_FIND:{
					DialogBoxParamW(0,MAKEINTRESOURCEW(RID_FIND),wnd,FindProc,0);
					break;
				}
				case AID_ZOOM_IN:{
					SendMessageW(gedit,EM_SETSEL,0,-1);
					break;
				}
                default:
                	break;
            }
            return 0;
	}
	return DefWindowProcW(wnd,msg,wparam,lparam);
}
WNDCLASSW wc = {0,WindowProc,0,0,0,0,0,0,0,L"DarkPad"};
HTHEME(*OpenNcThemeData)(HWND wnd, LPCWSTR classList);
HTHEME customOpenThemeData(HWND wnd, LPCWSTR classList){
	if (StringsEqualWide(classList,L"ScrollBar")){
		wnd = 0;
		classList = L"Explorer::ScrollBar";
	}
	return OpenNcThemeData(wnd,classList);
}
void WinMainCRTStartup(){
	instance = GetModuleHandleW(0);
	heap = GetProcessHeap();
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
#if 1
	AllocConsole();
	consoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	HMODULE uxtheme = LoadLibraryExW(L"uxtheme.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);
	OpenNcThemeData = GetProcAddress(uxtheme,MAKEINTRESOURCEA(49));
	((i32 (*)(i32))GetProcAddress(uxtheme,MAKEINTRESOURCEA(135)))(1);
	((void (*)())GetProcAddress(uxtheme,MAKEINTRESOURCEA(104)))();
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

	NONCLIENTMETRICSW ncm = {sizeof(NONCLIENTMETRICSW)};
	SystemParametersInfoW(SPI_GETNONCLIENTMETRICS,sizeof(NONCLIENTMETRICSW),&ncm,0);
	menufont = CreateFontIndirectW(&ncm.lfMenuFont);
	bBackground = CreateSolidBrush(RGB(20,20,20));
	bMenuBackground = CreateSolidBrush(RGB(40,40,40));
	HANDLE hfile = CreateFileW(L"font",GENERIC_READ,FILE_SHARE_READ,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (hfile!=INVALID_HANDLE_VALUE){
		LOGFONTW lf;
		ReadFile(hfile,&lf,sizeof(lf),0,0);
		CloseHandle(hfile);
		font = CreateFontIndirectW(&lf);
	} else font = CreateFontW(-12,0,0,0,FW_DONTCARE,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_DONTCARE,L"Consolas");
	wc.hIcon = LoadIconW(instance,MAKEINTRESOURCEA(RID_ICON));
	wc.hbrBackground = bBackground;
	RegisterClassW(&wc);
	RECT wr = {0,0,800,600};
	AdjustWindowRect(&wr,WS_OVERLAPPEDWINDOW,FALSE);
	i32 wndWidth = wr.right-wr.left;
	i32 wndHeight = wr.bottom-wr.top;
	CreateWindowExW(WS_EX_APPWINDOW,wc.lpszClassName,wc.lpszClassName,WS_OVERLAPPEDWINDOW|WS_VISIBLE,GetSystemMetrics(SM_CXSCREEN)/2-wndWidth/2,GetSystemMetrics(SM_CYSCREEN)/2-wndHeight/2,wndWidth,wndHeight,0,0,instance,0);
	i32 argc;
	u16 **argv = CommandLineToArgvW(GetCommandLineW(),&argc);
	if (argc==2) loadFile(argv[1]);
	MSG msg;
	HACCEL haccel = CreateAcceleratorTableW(accels,COUNT(accels));
	while (GetMessageW(&msg,0,0,0)){
		if (!TranslateAcceleratorW(gwnd,haccel,&msg)){
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}
	ExitProcess(msg.wParam);
}