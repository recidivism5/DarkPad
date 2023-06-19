/*
	No more OOP.
    We want full dpi compliance. I guess. Fuck.
	No variable width fonts.
	Every operation is a string, applied to a set of positions.
	First we're gonna try expanded tabs.
*/
#define WIN32_LEAN_AND_MEAN
#undef UNICODE
int _fltused;
#include <windows.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <shobjidl_core.h>
#include <Shlguid.h>
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
void *memset(void *dest, int c, size_t size){
	u8 *b = dest;
	while (size--) *b++ = c;
	return dest;
}
void *memcpy(void *dest, const void *src, size_t size){
	u8 *d = dest, *s = src;
	while (size--) *d++ = *s++;
	return dest;
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
	while (*src){
		*dst++ = *src++;
	}
	*dst = 0;
	return dst;
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
void centerRectInRect(RECT *inner, RECT *outer){
	i32 iw = inner->right - inner->left;
	i32 ih = inner->bottom - inner->top;
	i32 ow = outer->right - outer->left;
	i32 oh = outer->bottom - outer->top;
	inner->left = outer->left + (ow-iw)/2;
	inner->top = outer->top + (oh-ih)/2;
	inner->right = inner->left + iw;
	inner->bottom = inner->top + ih;
}
RECT insetRect(RECT r){
	r.left+=4;r.right-=4;r.top+=4;r.bottom-=4;
	return r;
}
HWND gwnd;
void ShittyItoa(u8 *t, i32 n){
	int i = 0;
	do {
		t[i] = '0'+n%10;
		n/=10;
		i++;
	} while (n);
	t[i]=0;
}
void ShittyPrintNum(i32 n){
	u8 t[64];
	ShittyItoa(t,n);
	MessageBoxA(gwnd,t,"Number",MB_OK);
}
RECT rTopbar,rTitlebar,rClient,rBody,rText,rBottombar;
union{
    struct{HBRUSH
        titlebar,bodyBackground,systemBackground[2],closeHovered,menuBackground[2],bottombarBackground;
    };
    HBRUSH arr[8];
}brushes;
union{
	struct{HPEN
		unfocused,focused;
	};
	HPEN arr[2];
}pens;
typedef struct{
    u8 *name;
	u32 titlebarIconId;
	u32 brushColors[8],penColors[2],text,menuText[2];
}Theme;
#define GREY RGB(127,127,127)
#define WHITE RGB(255,255,255)
Theme themes[]={
    {"Candy Cane",RID_CANDYCANE,0,RGB(20,20,20),RGB(20,20,20),RGB(40,40,40),RGB(0xcc,0,0),RGB(0,100,0),RGB(0,120,0),RGB(40,40,40),GREY,WHITE,WHITE,GREY,WHITE},
	{"White On Black",0,WHITE,0,0,RGB(20,20,20),RGB(20,20,20),0,RGB(20,20,20),RGB(20,20,20),GREY,WHITE,WHITE,GREY,WHITE},
};
Theme *theme;
void setTheme(Theme *t){//this is unfortunately the most efficient way of doing solid brushes/pens in win32. super cringe.
    if (t->titlebarIconId){
		if (brushes.titlebar) DeleteObject(brushes.titlebar);
		HICON icon = (HICON)LoadImageA(GetModuleHandle(0),MAKEINTRESOURCEA(t->titlebarIconId),IMAGE_ICON,0,0,LR_LOADTRANSPARENT);
		ICONINFO info;
		GetIconInfo(icon,&info);
		brushes.titlebar = CreatePatternBrush(info.hbmColor);
		DeleteObject(info.hbmColor);
		DeleteObject(info.hbmMask);
		DestroyIcon(icon);
	}
	for (int i = t->titlebarIconId ? 1 : 0; i < COUNT(brushes.arr); i++){
        if (brushes.arr[i]) DeleteObject(brushes.arr[i]);
        brushes.arr[i] = CreateSolidBrush(t->brushColors[i]);
    }
	FOR(i,COUNT(pens.arr)){
		if (pens.arr[i]) DeleteObject(pens.arr[i]);
		pens.arr[i] = CreatePen(PS_SOLID,1,t->penColors[i]);
	}
	theme = t;
	InvalidateRect(gwnd,0,0);
}
HFONT systemFont;
HCURSOR cArrow,cIBeam;
enum{
    ACC_NEW=42069,
    ACC_OPEN,
    ACC_SAVE,
    ACC_SAVE_AS,
    ACC_UNDO,
    ACC_REDO,
    ACC_CUT,
    ACC_COPY,
    ACC_PASTE,
    ACC_FIND,
    ACC_GO_TO_LINE,
    ACC_SELECT_ALL,
    ACC_INSERT_TIMESTAMP,
    ACC_WORD_WRAP,
	ACC_LAST,
};
ACCEL accels[]={
    FCONTROL|FVIRTKEY,'N',ACC_NEW,
    FCONTROL|FVIRTKEY,'O',ACC_OPEN,
    FCONTROL|FVIRTKEY,'S',ACC_SAVE,
    FCONTROL|FSHIFT|FVIRTKEY,'S',ACC_SAVE_AS,
    FCONTROL|FVIRTKEY,'Z',ACC_UNDO,
    FCONTROL|FSHIFT|FVIRTKEY,'Z',ACC_REDO,
    FCONTROL|FVIRTKEY,'X',ACC_CUT,
    FCONTROL|FVIRTKEY,'C',ACC_COPY,
    FCONTROL|FVIRTKEY,'V',ACC_PASTE,
    FCONTROL|FVIRTKEY,'F',ACC_FIND,
    FCONTROL|FVIRTKEY,'G',ACC_GO_TO_LINE,
    FCONTROL|FVIRTKEY,'A',ACC_SELECT_ALL,
    FCONTROL|FVIRTKEY,'T',ACC_INSERT_TIMESTAMP,
    FALT|FVIRTKEY,'Z',ACC_WORD_WRAP,
};
typedef struct Button{
    RECT rect;
    u8 *name;
    u16 shortcut;
    void (*func)(void);
}Button;
typedef struct Menu{
    Button button;
	Button *childButtons;
    i32 childButtonCount;
}Menu;
Button *hoveredButton;
Menu *menu = 0;
void close(){
    PostMessageA(gwnd,WM_CLOSE,0,0);
}
void maximize(){
    ShowWindow(gwnd,windowMaximized(gwnd) ? SW_NORMAL : SW_MAXIMIZE);
}
void minimize(){
    ShowWindow(gwnd,SW_MINIMIZE);
}
typedef struct DAHead{
	u32 total,used;
}DAHead;
void daInsertBytes(u8 **arr, i32 offset, u8 *b, i32 len){
	if (*arr){
		DAHead *h = *arr - sizeof(DAHead);
		if (h->total < h->used+len){
			while (h->total < h->used+len) h->total *= 2;
			h = realloc(h,sizeof(DAHead) + h->total);
			*arr = h+1;
		}
		MoveMem(*arr + offset + len,*arr + offset,h->used-offset);
		memcpy(*arr + offset,b,len);
		h->used += len;
	} else {
		i32 total = 1;
		while (total < offset+len) total *= 2;
		DAHead *h = malloc(sizeof(DAHead)+total);
		*arr = h+1;
		memcpy(*arr + offset,b,len);
		h->total = total;
		h->used = offset+len;
	}
}
void daRemoveBytes(u8 **arr, i32 offset, i32 len){
	DAHead *h = *arr - sizeof(DAHead);
	MoveMem(*arr+offset,*arr+offset+len,h->used-offset-len);
	h->used -= len;
}
typedef struct File{
	u8 path[MAX_PATH+4];
	u32 total,used;
	u8 **lines;
}File;
void insertLine(File *d, i32 offset, u8 *b, i32 len){
	if (d->used == d->total){
		d->total *= 2;
		d->lines = realloc(d->lines,d->total*sizeof(*d->lines));
		memset(d->lines+d->used,0,(d->total-d->used)*sizeof(*d->lines));
	}
	MoveMem(d->lines+offset+1,d->lines+offset,(d->used-offset)*sizeof(*d->lines));
	d->used++;
	d->lines[offset] = 0;
	if (len) daInsertBytes(d->lines+offset,0,b,len);
}
void removeLine(File *d, i32 offset){
	MoveMem(d->lines+offset,d->lines+offset+1,(d->used-offset)*sizeof(*d->lines));
	d->used--;
}
File file;
void loadFile(u8 *path){
	HANDLE hfile = CreateFileA(path,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	u32 size = GetFileSize(hfile,NULL);
	u8 *buf = malloc(size);
	u32 bytesRead = 0;
	ReadFile(hfile,buf,size,&bytesRead,NULL);
	CloseHandle(hfile);
	u8 *s = buf, *p = buf, *e = buf+size;
	while (s < e){
		if (p==e || p[0]=='\r' || p[0]=='\n'){
			insertLine(&file,file.used,s,p-s);
			if (p+1 < e && p[0]=='\r' && p[1]=='\n') p+=2;
			else p++;
			s = p;
		} else p++;
	}
	free(buf);
}
void open(){
	IFileDialog *pfd;
	IShellItem *psi;
	PWSTR path = NULL;
	if (SUCCEEDED(CoCreateInstance(&CLSID_FileOpenDialog,NULL,CLSCTX_INPROC_SERVER,&IID_IFileOpenDialog,&pfd))){
		pfd->lpVtbl->Show(pfd,gwnd);
		if (SUCCEEDED(pfd->lpVtbl->GetResult(pfd,&psi))){
			if (SUCCEEDED(psi->lpVtbl->GetDisplayName(psi,SIGDN_FILESYSPATH,&path))){
				FOR(i,MAX_PATH){
					if (!path[i]) break;
					file.path[i] = path[i];
				}
				loadFile(file.path);
				CoTaskMemFree(path);
			}
			psi->lpVtbl->Release(psi);
		}
		pfd->lpVtbl->Release(pfd);
	}
	menu = 0;
	hoveredButton = 0;
	InvalidateRect(gwnd,0,0);
}
Button fileButtons[]={
	{{0},"New",ACC_NEW,0},
	{{0},"Open...",ACC_OPEN,open},
	{{0},"Save",ACC_SAVE,0},
	{{0},"Save As...",ACC_SAVE_AS,0},
};
Button editButtons[]={
	{{0},"Undo",ACC_UNDO,0},
	{{0},"Redo",ACC_REDO,0},
	{{0},"Cut",ACC_CUT,0},
	{{0},"Copy",ACC_COPY,0},
	{{0},"Paste",ACC_PASTE,0},
	{{0},"Find",ACC_FIND,0},
	{{0},"Go To Line",ACC_GO_TO_LINE,0},
	{{0},"Select All",ACC_SELECT_ALL,0},
	{{0},"Insert Timestamp",ACC_INSERT_TIMESTAMP,0},
};
Button viewButtons[]={
	{{0},"Word Wrap",ACC_WORD_WRAP,0},
	{{0},"Font Size",0,0},
	{{0},"Tab Width",0,0},
};
Button themeButtons[COUNT(themes)];
void setThemeFromButton(){
	setTheme(themes+(hoveredButton-themeButtons));
}
Menu *toggledMenu;
void toggleMenu(){
	Menu *m = hoveredButton;//Not UB: https://stackoverflow.com/questions/11057174/are-there-any-guarantees-about-c-struct-order
	RECT r = m->childButtons[0].rect;
	r.bottom = m->childButtons[m->childButtonCount-1].rect.bottom;
	InvalidateRect(gwnd,&r,0);
	if (toggledMenu != m){
		if (toggledMenu){
			r = toggledMenu->childButtons[0].rect;
			r.bottom = toggledMenu->childButtons[toggledMenu->childButtonCount-1].rect.bottom;
			InvalidateRect(gwnd,&r,0);
		}
		toggledMenu = m;
	} else toggledMenu = 0;
}
Menu menus[]={
    {{{0},"File",0,toggleMenu},fileButtons,COUNT(fileButtons)},
    {{{0},"Edit",0,toggleMenu},editButtons,COUNT(editButtons)},
    {{{0},"View",0,toggleMenu},viewButtons,COUNT(viewButtons)},
    {{{0},"Theme",0,toggleMenu},themeButtons,COUNT(themeButtons)},
};
Button buttons[]={
    {{0},0,0,close},
    {{0},0,0,maximize},
    {{0},0,0,minimize},
};
i32 windowMaximized(HWND wnd){
	WINDOWPLACEMENT p = {0};
	p.length = sizeof(WINDOWPLACEMENT);
	if (GetWindowPlacement(wnd,&p)) return p.showCmd == SW_SHOWMAXIMIZED;
	return 0;
}
void prepHDC(HDC hdc){
	i32 focus = GetFocus() ? 1 : 0;
	SelectObject(hdc,pens.arr[focus]);
	SelectObject(hdc,systemFont);
	SetBkMode(hdc,TRANSPARENT);
	SetTextColor(hdc,theme->menuText[focus]);
}
i32 getStringWidth(HDC hdc, u8 *s){
	RECT r;
	DrawTextA(hdc,s,-1,&r,DT_CALCRECT|DT_NOPREFIX|DT_SINGLELINE);
	return r.right-r.left;
}
void getAccString(u8 *s, ACCEL *acc){
	if (acc->fVirt & FCONTROL) s = CopyString(s,"Ctrl+");
	if (acc->fVirt & FSHIFT) s = CopyString(s,"Shift+");
	if (acc->fVirt & FALT) s = CopyString(s,"Alt+");
	*s++ = acc->key;
	*s = 0;
}
void fixRects(HDC hdc){
	HTHEME ht = OpenThemeData(gwnd,L"WINDOW");
	SIZE size = {0};
	GetThemePartSize(ht,0,WP_CAPTION,CS_ACTIVE,0,TS_TRUE,&size);
	CloseThemeData(ht);
	GetClientRect(gwnd,&rClient);
	rTopbar = rClient;
	rTopbar.bottom = dpiScale(size.cy)+2;
	rBottombar = rClient;
	rBottombar.top = rBottombar.bottom - dpiScale(20);
	rBody.left = rClient.left;
	rBody.right = rClient.right;
	rBody.top = rTopbar.bottom;
	rBody.bottom = rBottombar.top;
	rText = rBody;
	rText.left += 3;
	rText.right -= 3;
	rText.top += 3;
	i32 buttonWidth = dpiScale(47);
	i32 buttonHeight = rTopbar.bottom;
	i32 x = rTopbar.right;
	for (Button *b = buttons; b < buttons+COUNT(buttons); b++){
		b->rect = rTopbar;
		b->rect.right = x;
		x -= buttonWidth;
		b->rect.left = x;
	}
	rTitlebar = rTopbar;
	rTitlebar.right = buttons[2].rect.left;
	i32 menuPad = dpiScale(4);
	x = menuPad;
	buttonHeight = buttonHeight - 2*menuPad;
	for (Menu *m = menus; m < menus+COUNT(menus); m++){
		m->button.rect = rTitlebar;
		m->button.rect.top += menuPad;
		m->button.rect.bottom -= menuPad;
		m->button.rect.left = x;
		x += getStringWidth(hdc,m->button.name) + 3*menuPad;
		m->button.rect.right = x-menuPad;

		i32 y = rTitlebar.bottom;
		i32 width = 0;
		i32 shortcutEncountered = 0;
		for (Button *b = m->childButtons; b < m->childButtons+m->childButtonCount; b++){
			i32 w = getStringWidth(hdc,b->name);
			if (b->shortcut){
				shortcutEncountered = 1;
				u8 buf[32];
				ACCEL *acc = accels + b->shortcut-ACC_NEW;
				getAccString(buf,acc);
				w += getStringWidth(hdc,buf);
			}
			if (width < w) width = w;
		}
		width += (2+2*shortcutEncountered)*menuPad;
		for (Button *b = m->childButtons; b < m->childButtons+m->childButtonCount; b++){
			b->rect.left = m->button.rect.left;
			b->rect.right = b->rect.left + width;
			b->rect.top = y;
			y += buttonHeight;
			b->rect.bottom = y;
		}
	}
}
i32 testButton(Button *b, POINT pt){
	if (PtInRect(&b->rect,pt)){
		if (hoveredButton != b){
			if (hoveredButton) InvalidateRect(gwnd,&hoveredButton->rect,0);
			InvalidateRect(gwnd,&b->rect,0);
			hoveredButton = b;
		}
		return 1;
	}
	return 0;
}
i32 __stdcall WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
		case WM_NCCALCSIZE:{
			if (!wparam) break;
            dpi = GetDpiForWindow(wnd);
			i32 fx = GetSystemMetricsForDpi(SM_CXFRAME,dpi);
			i32 fy = GetSystemMetricsForDpi(SM_CYFRAME,dpi);
			i32 pad = GetSystemMetricsForDpi(SM_CXPADDEDBORDER,dpi);
			RECT *rr = ((NCCALCSIZE_PARAMS *)lparam)->rgrc;
			rr->right -= fx + pad;
			rr->left += fx + pad;
			rr->bottom -= fy + pad;
			if (windowMaximized(wnd)) rr->top += pad;
			return 0;
		}
		case WM_CREATE:{
			RECT wr;
			GetWindowRect(wnd,&wr);
			SetWindowPos(wnd,0,wr.left,wr.top,wr.right-wr.left,wr.bottom-wr.top,SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE);//this prevents the single white frame when the window first appears
			break;
		}
		case WM_ACTIVATE:{
			InvalidateRect(wnd,&rTopbar,0);
			return DefWindowProc(wnd,msg,wparam,lparam);
		}
		case WM_NCHITTEST:{
			LRESULT hit = DefWindowProc(wnd,msg,wparam,lparam);
			switch (hit){
				case HTNOWHERE:
				case HTRIGHT:
				case HTLEFT:
				case HTTOPLEFT:
				case HTTOP:
				case HTTOPRIGHT:
				case HTBOTTOMRIGHT:
				case HTBOTTOM:
				case HTBOTTOMLEFT:
					return hit;
			}
			i32 fy = GetSystemMetricsForDpi(SM_CYFRAME,dpi);
			i32 pad = GetSystemMetricsForDpi(SM_CXPADDEDBORDER,dpi);
			POINT cursor = {LOWORD(lparam),HIWORD(lparam)};
			ScreenToClient(wnd,&cursor);
			if (cursor.y >= 0 && cursor.y < fy + pad) return HTTOP;
			if (cursor.y < rTitlebar.bottom) return HTCAPTION;
			return HTCLIENT;
		}
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		case WM_PAINT:{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(wnd,&ps);
			prepHDC(hdc);
			fixRects(hdc);

			FillRect(hdc,&buttons[0].rect,hoveredButton == buttons ? brushes.closeHovered : brushes.systemBackground[0]);
			RECT r = {0,0,10,10};
			centerRectInRect(&r,&buttons[0].rect);
			MoveToEx(hdc,r.left,r.top,NULL);
			LineTo(hdc,r.right + 1,r.bottom + 1);
			MoveToEx(hdc,r.left,r.bottom,NULL);
			LineTo(hdc,r.right + 1,r.top - 1);

			FillRect(hdc,&buttons[1].rect,brushes.systemBackground[hoveredButton == buttons+1]);
			SelectObject(hdc,GetStockObject(HOLLOW_BRUSH));
			r = (RECT){0,0,10,10};
			centerRectInRect(&r,&buttons[1].rect);
			if (windowMaximized(gwnd)){
				Rectangle(hdc,r.left+2,r.top-2,r.right+2,r.bottom-2);
				FillRect(hdc,&r,brushes.systemBackground[hoveredButton == buttons+1]);
			}
			Rectangle(hdc,r.left,r.top,r.right,r.bottom);

			FillRect(hdc,&buttons[2].rect,brushes.systemBackground[hoveredButton == buttons+2]);
			r = (RECT){0,0,10,1};
			centerRectInRect(&r,&buttons[2].rect);
			MoveToEx(hdc,r.left,r.top,NULL);
			LineTo(hdc,r.right,r.top);
			
			for(Menu *m = menus; m < menus+COUNT(menus); m++){
				Button *b = &m->button;
				FillRect(hdc,&b->rect,brushes.menuBackground[hoveredButton == b]);
				DrawTextA(hdc,b->name,-1,&b->rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER);
				EXCLUDE_RECT(hdc,b->rect);
			}
			if (toggledMenu){
				for(Button *b = toggledMenu->childButtons; b < toggledMenu->childButtons+toggledMenu->childButtonCount; b++){
					FillRect(hdc,&b->rect,brushes.menuBackground[hoveredButton == b]);
					r = b->rect;
					i32 menuPad = dpiScale(4);
					r.left += menuPad;
					DrawTextA(hdc,b->name,-1,&r,DT_SINGLELINE|DT_LEFT|DT_VCENTER);
					if (b->shortcut){
						r = b->rect;
						r.right -= menuPad;
						u8 buf[32];
						ACCEL *acc = accels + b->shortcut-ACC_NEW;
						getAccString(buf,acc);
						DrawTextA(hdc,buf,-1,&r,DT_SINGLELINE|DT_RIGHT|DT_VCENTER);
					}
				}
				HBRUSH bmt = CreateSolidBrush(theme->menuText[GetFocus() ? 1 : 0]);
				r = toggledMenu->childButtons[0].rect;
				r.bottom = toggledMenu->childButtons[toggledMenu->childButtonCount-1].rect.bottom;
				FrameRect(hdc,&r,bmt);
				DeleteObject(bmt);
				EXCLUDE_RECT(hdc,r);
			}

			FillRect(hdc,&rTitlebar,brushes.titlebar);

			FillRect(hdc,&rBody,brushes.bodyBackground);
			FillRect(hdc,&rBottombar,brushes.bottombarBackground);

			EndPaint(wnd,&ps);
			return 0;
		}
		case WM_NCMOUSEMOVE:case WM_MOUSEMOVE:{
			POINT cursor;
			GetCursorPos(&cursor);
			ScreenToClient(wnd,&cursor);
			for (Button *b = buttons; b < buttons+COUNT(buttons); b++) if (testButton(b,cursor)) goto NEXT;
			for (Menu *m = menus; m < menus+COUNT(menus); m++){
				if (testButton(&m->button,cursor)) goto NEXT;
				if (toggledMenu == m) for (Button *b = m->childButtons; b < m->childButtons+m->childButtonCount; b++) if (testButton(b,cursor)) goto NEXT;
			}
			if (hoveredButton){
				InvalidateRect(wnd,&hoveredButton->rect,0);
				hoveredButton = 0;
			}
			NEXT:
			if (hoveredButton) SetCursor(cArrow);
			else if (msg == WM_MOUSEMOVE) SetCursor(PtInRect(&rText,cursor) ? cIBeam : cArrow);
			TRACKMOUSEEVENT tme = {sizeof(tme),msg == WM_NCMOUSEMOVE ? TME_LEAVE|TME_NONCLIENT : TME_LEAVE,wnd,0};
			TrackMouseEvent(&tme);
			return 0;
		}
		case WM_NCMOUSELEAVE:case WM_MOUSELEAVE:
			if (hoveredButton){
				InvalidateRect(wnd,&hoveredButton->rect,0);
				hoveredButton = 0;
			}
			return 0;
		case WM_NCLBUTTONDOWN:case WM_LBUTTONDOWN:case WM_NCLBUTTONDBLCLK:
			if (hoveredButton && hoveredButton->func){
				hoveredButton->func();
				return 0;
			}
			break;
        case WM_COMMAND:{
			switch (LOWORD(wparam)){
				case ACC_NEW:
					break;
				case ACC_OPEN:
					open();
					break;
			}
			return 0;
		}
	}
	return DefWindowProc(wnd,msg,wparam,lparam);
}
WNDCLASSA wc = {CS_HREDRAW|CS_VREDRAW,WindowProc,0,0,0,0,0,0,0,"DarkPad"};
void __stdcall WinMainCRTStartup(){
	HINSTANCE instance = GetModuleHandleA(0);
	u8 *cmdline = GetCommandLineA();
	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
	heap = GetProcessHeap();
	setTheme(themes);
	FOR(i,COUNT(themeButtons)){
		themeButtons[i].name = themes[i].name;
		themeButtons[i].func = setThemeFromButton;
	}
	systemFont = CreateFontA(-12,0,0,0,FW_DONTCARE,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,FF_DONTCARE,"Segoe UI");
	cArrow = LoadCursorA(0,IDC_ARROW);
	cIBeam = LoadCursorA(0,IDC_IBEAM);
	HACCEL haccel = CreateAcceleratorTableA(accels,COUNT(accels));
	wc.hInstance = instance;
	wc.hIcon = LoadIconA(instance,MAKEINTRESOURCEA(RID_ICON));
	wc.hbrBackground = CreateSolidBrush(RGB(0,0,0));
	RegisterClassA(&wc);
	RECT wr = {0,0,800,600};
	AdjustWindowRect(&wr,WS_OVERLAPPEDWINDOW,FALSE);
	i32 wndWidth = wr.right-wr.left;
	i32 wndHeight = wr.bottom-wr.top;
	gwnd = CreateWindowExA(WS_EX_APPWINDOW,wc.lpszClassName,wc.lpszClassName,WS_OVERLAPPEDWINDOW,GetSystemMetrics(SM_CXSCREEN)/2-wndWidth/2,GetSystemMetrics(SM_CYSCREEN)/2-wndHeight/2,wndWidth,wndHeight,0,0,wc.hInstance,0);
	ShowWindow(gwnd,SW_SHOWDEFAULT);
	HDC hdc = GetDC(gwnd);
	prepHDC(hdc);
	fixRects(hdc);
	ReleaseDC(gwnd,hdc);
	if (cmdline && cmdline[0]) loadFile(cmdline);
	else {
		file.total = 1;
		file.used = 1;
		file.lines = calloc(file.total,sizeof(*file.lines));
	}
	MSG msg;
	while (GetMessageA(&msg,0,0,0)){
		if (!TranslateAcceleratorA(gwnd,haccel,&msg)){
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
	}
	ExitProcess(msg.wParam);
}