#define _NO_CRT_STDIO_INLINE
#define WIN32_LEAN_AND_MEAN
int _fltused;
#include <stdio.h>
#include <string.h>
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
HBRUSH bBackground,bMenuBackground,bOutline;
HFONT font,menufont;
u16 gpath[MAX_PATH+4];
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
	AID_COLORS,
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
	_snwprintf(title,COUNT(title),L"%s%s - DarkPad",starred ? L"*" : L"",*gpath ? gpath : L"Untitled");
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
	wcscpy(gpath,path);
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
	wcscpy(gpath,path);
	starred = 0;
	updateTitle();
}
i32 controlStyler(HWND wnd, LPARAM lparam){
	u16 className[32];
	GetClassNameW(wnd,className,COUNT(className));
	if (!wcscmp(className,L"Button")) (GetWindowLongPtrW(wnd,GWL_STYLE) & BS_TYPEMASK) == BS_AUTOCHECKBOX ? SetWindowTheme(wnd,L"fake",L"fake") : SetWindowTheme(wnd,L"DarkMode_Explorer",0); //npp subclasses autocheckbox and draws it manually, but here we just set its theme to a nonexistent theme, reverting it to windows 2000 style which for some reason responds to SetTextColor. Looks decent enough for my purposes.
	return 1;
}
i64 FindProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
		case WM_INITDIALOG:{
			i32 t = 1;
			DwmSetWindowAttribute(wnd,20,&t,sizeof(t));
			EnumChildWindows(wnd,controlStyler,0);

			HKEY key;
			RegOpenKeyW(HKEY_CURRENT_USER,L"software\\darkpad",&key);
			DWORD v,vsize = sizeof(v);
			RegQueryValueExW(key,L"matchcase",0,0,&v,&vsize);
			SendMessageW(GetDlgItem(wnd,RID_FIND_CASE),BM_SETCHECK,v ? BST_CHECKED : BST_UNCHECKED,0);
			RegCloseKey(key);
			return 0;
		}
		case WM_CTLCOLORDLG: case WM_CTLCOLORSTATIC: case WM_CTLCOLORBTN:
			SetBkMode(wparam,TRANSPARENT);
			SetTextColor(wparam,RGB(255,255,255));
			return bBackground;
		case WM_CTLCOLOREDIT:
			SetTextColor(wparam,0xffffff);
			SetBkColor(wparam,RGB(20,20,20));
			return bBackground;
		case WM_COMMAND:{
			switch (LOWORD(wparam)){
				case RID_FIND_WHAT:
					break;
				case RID_FIND_WITH:
					break;
				case RID_FIND_CASE:{
					HKEY key;
					RegOpenKeyW(HKEY_CURRENT_USER,L"software\\darkpad",&key);
					DWORD v = SendMessageW(lparam,BM_GETCHECK,0,0)==BST_CHECKED ? 1 : 0;
					RegSetValueExW(key,L"matchcase",0,REG_DWORD,&v,sizeof(v));
					RegCloseKey(key);
					break;
				}
				case RID_FIND_UP: case RID_FIND_DOWN: case RID_REPLACE: case RID_REPLACE_ALL:{
					HWND hwhat = GetDlgItem(wnd,RID_FIND_WHAT);
					i64 whatlen = SendMessageW(hwhat,WM_GETTEXTLENGTH,0,0) + 1;
					if (whatlen == 1) break;
					u16 *what = HeapAlloc(heap,0,whatlen*sizeof(u16));
					whatlen = SendMessageW(hwhat,WM_GETTEXT,whatlen,what);

					HWND hwith = GetDlgItem(wnd,RID_FIND_WITH);
					i64 withlen = SendMessageW(hwith,WM_GETTEXTLENGTH,0,0) + 1;
					u16 *with = HeapAlloc(heap,0,withlen*sizeof(u16));
					withlen = SendMessageW(hwith,WM_GETTEXT,withlen,with);

					i64 total = SendMessageW(gedit,WM_GETTEXTLENGTH,0,0) + 1;
					u16 *text = HeapAlloc(heap,0,total*sizeof(u16));
					i64 textlen = SendMessageW(gedit,WM_GETTEXT,total,text);

					i32 (*cmp)(u16 *s1, u16 *s2, size_t maxCount) = SendMessageW(GetDlgItem(wnd,RID_FIND_CASE),BM_GETCHECK,0,0)==BST_CHECKED ? wcsncmp : _wcsnicmp;

					switch (LOWORD(wparam)){
						case RID_FIND_UP:{
							DWORD starti,endi;
							SendMessageW(gedit,EM_GETSEL,&starti,&endi);
							u16 *start = text+starti,
							*s = start,
							*pe = text+whatlen-1;
							do {
								if (!cmp(s,what+whatlen-1,1) && !cmp(s-whatlen+1,what,1) && !cmp(s-whatlen+2,what+1,MAX(0,whatlen-2))){
									SendMessageW(gedit,EM_SETSEL,s-whatlen+1-text,s+1-text);
									break;
								}
								s--;
								if (s < pe) s = text+textlen-1;
							} while (s != start);
							break;
						}
						case RID_FIND_DOWN:{
							DWORD starti,endi;
							SendMessageW(gedit,EM_GETSEL,&starti,&endi);
							u16 *start = text+starti,
							*s = start,
							*pe = text+textlen-whatlen;
							do {
								if ((s!=start || endi-starti!=whatlen) && !cmp(s,what,1) && !cmp(s+whatlen-1,what+whatlen-1,1) && !cmp(s+1,what+1,MAX(0,whatlen-2))){
									SendMessageW(gedit,EM_SETSEL,s-text,s-text+whatlen);
									break;
								}
								s++;
								if (s > pe) s = text;
							} while (s != start);
							break;
						}
						case RID_REPLACE:{
							DWORD starti,endi;
							SendMessageW(gedit,EM_GETSEL,&starti,&endi);
							u16 *start = text+starti,
							*s = start,
							*pe = text+textlen-whatlen;
							i64 dif = 0;
							do {
								if (!cmp(s,what,1) && !cmp(s+whatlen-1,what+whatlen-1,1) && !cmp(s+1,what+1,MAX(0,whatlen-2))){
									if (s==start){
										SendMessageW(gedit,EM_REPLACESEL,1,with);
										s += withlen;
										dif = withlen-whatlen;
									} else {
										SendMessageW(gedit,EM_SETSEL,s-text+dif,s-text+dif+whatlen);
										break;
									}
								}
								s++;
								if (s > pe) s = text;
							} while (s != start);
							break;
						}
						case RID_REPLACE_ALL:{
							u16 *s = text,
							*pe = text+textlen-whatlen;
							i32 replace = 0;
							do {
								START:
								if (!cmp(s,what,1) && !cmp(s+whatlen-1,what+whatlen-1,1) && !cmp(s+1,what+1,MAX(0,whatlen-2))){
									if (textlen+withlen-whatlen > total-1){
										while (textlen+withlen-whatlen > total-1) total += 4096;
										i64 d = s-text;
										text = HeapReAlloc(heap,0,text,total*sizeof(u16));
										s = text+d;
									}
									memmove(s+withlen,s+whatlen,(text+textlen-(s+whatlen))*sizeof(u16));
									memcpy(s,with,withlen*sizeof(u16));
									textlen += withlen-whatlen;
									pe = text+textlen-whatlen;
									s += withlen;
									replace = 1;
									if (s > pe) break;
									goto START;
								}
								s++;
								if (s > pe) s = text;
							} while (s != text);
							if (replace){
								text[textlen] = 0;
								SendMessageW(gedit,WM_SETTEXT,0,text);
							}
							break;
						}
					}

					HeapFree(heap,0,what);
					HeapFree(heap,0,with);
					HeapFree(heap,0,text);
					break;
				}
				case IDCANCEL: 
                    EndDialog(wnd,wparam); 
                    return TRUE;
			}
		}
	}
	return 0;
}
HMENU Format;
i64 WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
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
			Format = CreateMenu();
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
			HKEY key;
			RegOpenKeyW(HKEY_CURRENT_USER,L"software\\darkpad",&key);
			DWORD wordwrap,vsize = sizeof(wordwrap);
			RegQueryValueExW(key,L"wordwrap",0,0,&wordwrap,&vsize);
			RegCloseKey(key);
			AppendMenuW(Format,MF_STRING|(wordwrap ? MF_CHECKED : 0),AID_WORD_WRAP,L"Word Wrap\tAlt+Z");
			AppendMenuW(Format,MF_STRING,AID_FONT,L"Font");
			AppendMenuW(Format,MF_STRING,AID_COLORS,L"Colors");
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
			gedit = CreateWindowExW(0,L"EDIT",0,WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL|ES_NOHIDESEL|(wordwrap ? 0 : (WS_HSCROLL|ES_AUTOHSCROLL)),0,0,0,0,wnd,0,instance,0);
			SendMessageW(gedit,WM_SETFONT,font,0);
			SendMessageW(gedit,EM_SETLIMITTEXT,UINT_MAX,0);
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
					HKEY key;
					RegOpenKeyW(HKEY_CURRENT_USER,L"software\\darkpad",&key);
					RegSetValueExW(key,L"font",0,REG_BINARY,&lf,sizeof(lf));
					RegCloseKey(key);
					break;
				}
				case AID_FIND:{
					ShowWindow(CreateDialogParamW(0,MAKEINTRESOURCEW(RID_FIND),wnd,FindProc,0),SW_SHOW);
					break;
				}
				case AID_WORD_WRAP:{
					MENUITEMINFOW mii = {0};
					mii.cbSize = sizeof(MENUITEMINFOW);
					mii.fMask = MIIM_STATE;
					GetMenuItemInfoW(Format,AID_WORD_WRAP,0,&mii);
					HKEY key;
					RegOpenKeyW(HKEY_CURRENT_USER,L"software\\darkpad",&key);
					DWORD v;
					if (mii.fState == MFS_CHECKED){
						mii.fState = MFS_UNCHECKED;
						v = 0;
					} else {
						mii.fState = MFS_CHECKED;
						v = 1;
					}
					HWND newedit = CreateWindowExW(0,L"EDIT",0,WS_CHILD|WS_VISIBLE|WS_VSCROLL|ES_LEFT|ES_MULTILINE|ES_AUTOVSCROLL|ES_NOHIDESEL|(v ? 0 : (WS_HSCROLL|ES_AUTOHSCROLL)),0,0,0,0,wnd,0,instance,0);
					SendMessageW(newedit,WM_SETFONT,font,0);
					SendMessageW(newedit,EM_SETLIMITTEXT,UINT_MAX,0);
					HLOCAL eh = SendMessageW(gedit,EM_GETHANDLE,0,0);
					SendMessageW(newedit,WM_SETTEXT,0,LocalLock(eh));
					LocalUnlock(eh);
					DestroyWindow(gedit);
					gedit = newedit;
					RECT r;
					GetClientRect(wnd,&r);
					MoveWindow(gedit,0,0,r.right,r.bottom-22,1);
					RegSetValueExW(key,L"wordwrap",0,REG_DWORD,&v,sizeof(v));
					RegCloseKey(key);
					SetMenuItemInfoW(Format,AID_WORD_WRAP,0,&mii);
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
	if (!wcscmp(classList,L"ScrollBar")){
		wnd = 0;
		classList = L"Explorer::ScrollBar";
	}
	return OpenNcThemeData(wnd,classList);
}
void mainCRTStartup(){
	instance = GetModuleHandleW(0);
	heap = GetProcessHeap();
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	HMODULE uxtheme = LoadLibraryExW(L"uxtheme.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);
	OpenNcThemeData = GetProcAddress(uxtheme,MAKEINTRESOURCEW(49));
	((i32 (*)(i32))GetProcAddress(uxtheme,MAKEINTRESOURCEW(135)))(1);
	((void (*)())GetProcAddress(uxtheme,MAKEINTRESOURCEW(104)))();
	u64 comctl = LoadLibraryExW(L"comctl32.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);
	PIMAGE_DELAYLOAD_DESCRIPTOR imports = comctl+((PIMAGE_NT_HEADERS)(comctl+((PIMAGE_DOS_HEADER)comctl)->e_lfanew))->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT].VirtualAddress;
	while (_stricmp(comctl+imports->DllNameRVA,"uxtheme.dll")) imports++;
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

	LOGFONTW lf;
	HKEY key;
	RegOpenKeyW(HKEY_CURRENT_USER,L"software\\darkpad",&key);
	DWORD vsize = sizeof(lf);
	RegQueryValueExW(key,L"font",0,0,&lf,&vsize);
	RegCloseKey(key);
	font = CreateFontIndirectW(&lf);

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