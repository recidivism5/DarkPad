/*
	No more OOP.
	We want full dpi compliance. I guess. Fuck.
	No variable width fonts.
	Every operation is a string, applied to a set of positions.
	First we're gonna try expanded tabs.
	Should we be using ExcludeRect for the menus or child windows?
	ComCtl Edit Control.
	If notepad.exe uses Edit Control, how the fuck does it handle files larger than 65535 lines?
	I just thought of some shit:
		We know the font used in the editor, so we can get the lineheight. We also can get firstvisible line.
		So just firgure out the scroll bar from those.
	Okay now this shit is getting crazy.
	
	Darkpad Mini:
	Just one theme, dark with no titlebar image.
	On Win11 we can use DwmSetWindowAttribute to set the titlebar color: https://stackoverflow.com/questions/39261826/change-the-color-of-the-title-bar-caption-of-a-win32-application
	On Win10 we can draw the titlebar ourselves using Direct2D, maybe look into the way Visual Studio Code emulates the button hover effect.
	The editor will be an edit control whose scrollbar we have turned dark using https://github.com/notepad-plus-plus/notepad-plus-plus/tree/master/PowerEditor/src/DarkMode

	New plan:


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
i32 __stdcall WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam){
	switch (msg){
		case WM_CREATE:
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
/*template <typename T>
constexpr T DataDirectoryFromModuleBase(void *moduleBase, size_t entryID)
{
	auto dosHdr = reinterpret_cast<PIMAGE_DOS_HEADER>(moduleBase);
	auto ntHdr = RVA2VA<PIMAGE_NT_HEADERS>(moduleBase, dosHdr->e_lfanew);
	auto dataDir = ntHdr->OptionalHeader.DataDirectory;
	return RVA2VA<T>(moduleBase, dataDir[entryID].VirtualAddress);
}
PIMAGE_THUNK_DATA FindDelayLoadThunkInModule(void *moduleBase, const char *dllName, uint16_t ordinal)
{
	auto imports = DataDirectoryFromModuleBase<PIMAGE_DELAYLOAD_DESCRIPTOR>(moduleBase, IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT);
	for (; imports->DllNameRVA; ++imports)
	{
		if (_stricmp(RVA2VA<LPCSTR>(moduleBase, imports->DllNameRVA), dllName) != 0)
			continue;

		auto impName = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportNameTableRVA);
		auto impAddr = RVA2VA<PIMAGE_THUNK_DATA>(moduleBase, imports->ImportAddressTableRVA);
		return FindAddressByOrdinal(moduleBase, impName, impAddr, ordinal);
	}
	return nullptr;
}
PIMAGE_THUNK_DATA FindAddressByOrdinal(void *moduleBase, PIMAGE_THUNK_DATA impName, PIMAGE_THUNK_DATA impAddr, uint16_t ordinal)
{
	for (; impName->u1.Ordinal; ++impName, ++impAddr)
	{
		if (IMAGE_SNAP_BY_ORDINAL(impName->u1.Ordinal) && IMAGE_ORDINAL(impName->u1.Ordinal) == ordinal)
			return impAddr;
	}
	return nullptr;
}
*/
HTHEME(__stdcall *OpenNcThemeData)(HWND wnd, LPCWSTR classList);
HTHEME customOpenThemeData(HWND wnd, LPCWSTR classList){
	if (StringsEqualWide(classList,L"ScrollBar")){
		wnd = 0;
		classList = L"Explorer::ScrollBar";
	}
	return OpenNcThemeData(wnd,classList);
}
void __stdcall WinMainCRTStartup(){
	instance = GetModuleHandleA(0);
	heap = GetProcessHeap();
	AllocConsole();
	consoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	HMODULE uxtheme = LoadLibraryExW(L"uxtheme.dll",0,LOAD_LIBRARY_SEARCH_SYSTEM32);
	OpenNcThemeData = GetProcAddress(uxtheme,MAKEINTRESOURCEA(49));
	((i32 (__stdcall *)(i32))GetProcAddress(uxtheme,MAKEINTRESOURCEA(135)))(1);
	((void (__stdcall *)())GetProcAddress(uxtheme, MAKEINTRESOURCEA(104)))();
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
	wc.hbrBackground = background;
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