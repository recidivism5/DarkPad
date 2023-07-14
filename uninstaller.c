/*
- put darkpad.exe and uninstall.exe in C:\Users\destr\AppData\Local\darkpad (https://stackoverflow.com/questions/2489613/how-to-get-system-folder-pathc-windows-c-program-files-in-windows-using-c)
- store in HKEY_CURRENT_USER\SOFTWARE\darkpad:
    - word wrap = 1
    - font = consolas 12 pt
    - match case = 0
    - match whole word = 0
- link uninstall.exe in HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall (https://learn.microsoft.com/en-us/windows/win32/msi/configuring-add-remove-programs-with-windows-installer)
- put shortcut in %appdata%\Microsoft\Windows\Start Menu\Programs (https://gist.github.com/abel0b/0b740648d6370e3e77fd70a816a34523) or maybe (https://learn.microsoft.com/en-us/windows/win32/shell/how-to-add-shortcuts-to-the-start-menu)
*/
#define WIN32_LEAN_AND_MEAN
int _fltused;
#include <windows.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vssym32.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <shobjidl_core.h>
#include <shlguid.h>
#include <commdlg.h>
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
u16 *EndOf(u16 *s){
	while (*s) s++;
	return s;
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
#define CONSOLE 1
#if CONSOLE
HANDLE consoleOut;
#define print(str) WriteConsoleW(consoleOut,str,StringLength(str),0,0); WriteConsoleW(consoleOut,L"\r\n",2,0,0);
#else
#define print(str)
#endif
HANDLE heap;
HINSTANCE instance;
u16 path[MAX_PATH];
void WinMainCRTStartup(){
	instance = GetModuleHandleW(0);
	heap = GetProcessHeap();
	CoInitialize(0);
#if CONSOLE
	AllocConsole();
	consoleOut = GetStdHandle(STD_OUTPUT_HANDLE);
#endif

	u16 *prgs;
    SHGetKnownFolderPath(&FOLDERID_Programs,0,0,&prgs);
	CopyString(CopyString(path,prgs),L"\\DarkPad.lnk");
	DeleteFileW(path);

	RegDeleteKeyW(HKEY_CURRENT_USER,L"software\\darkpad");
	RegDeleteKeyW(HKEY_CURRENT_USER,L"software\\microsoft\\windows\\currentversion\\uninstall\\darkpad");

	CopyString(path,L"/c cd .. & rmdir /s /q ");
	SHGetSpecialFolderPathW(0,EndOf(path),CSIDL_LOCAL_APPDATA,0);
	CopyString(CopyString(EndOf(path),L"\\darkpad"),L" >> NUL");
	u16 cmdPath[MAX_PATH];
	GetEnvironmentVariableW(L"ComSpec",cmdPath,MAX_PATH);
    ShellExecuteW(0,0,cmdPath,path,0,SW_HIDE);

	CoUninitialize();
	ExitProcess(0);
}