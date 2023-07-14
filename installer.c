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
#include "installerres.h"
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
#define CONSOLE 0
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

	SHGetSpecialFolderPathW(0,path,CSIDL_LOCAL_APPDATA,0);
	CopyString(EndOf(path),L"\\darkpad");
	CreateDirectoryW(path,0);

	CopyString(EndOf(path),L"\\darkpad.exe");
	HANDLE hfile = CreateFileW(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	HRSRC res = FindResourceW(instance,MAKEINTRESOURCEW(RID_DARKPAD),RT_RCDATA);
	u8 *data = LockResource(LoadResource(0,res));
	u32 size = SizeofResource(0,res);
	WriteFile(hfile,data,size,0,0);
	CloseHandle(hfile);

	IShellLinkW* psl;
	CoCreateInstance(&CLSID_ShellLink,0,CLSCTX_INPROC_SERVER,&IID_IShellLinkW,&psl);
	psl->lpVtbl->SetPath(psl,path);
	psl->lpVtbl->SetDescription(psl,L"DarkPad");
	IPersistFile* ppf;
	psl->lpVtbl->QueryInterface(psl,&IID_IPersistFile,&ppf);
	u16 *prgs;
	SHGetKnownFolderPath(&FOLDERID_Programs,0,0,&prgs);
	CopyString(CopyString(path,prgs),L"\\DarkPad.lnk");
	ppf->lpVtbl->Save(ppf,path,1);
    ppf->lpVtbl->Release(ppf);
	psl->lpVtbl->Release(psl);
	CoTaskMemFree(prgs);

	SHGetSpecialFolderPathW(0,path,CSIDL_LOCAL_APPDATA,0);
	CopyString(EndOf(path),L"\\darkpad\\uninstaller.exe");
	hfile = CreateFileW(path,GENERIC_WRITE,0,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
	res = FindResourceW(instance,MAKEINTRESOURCEW(RID_UNINSTALLER),RT_RCDATA);
	data = LockResource(LoadResource(0,res));
	size = SizeofResource(0,res);
	WriteFile(hfile,data,size,0,0);
	CloseHandle(hfile);

	HKEY key;
	RegCreateKeyW(HKEY_CURRENT_USER,L"software\\darkpad",&key);
	u32 d = 1;
	RegSetValueExW(key,L"wordwrap",0,REG_DWORD,&d,sizeof(d));
	d = 0;
	RegSetValueExW(key,L"matchcase",0,REG_DWORD,&d,sizeof(d));
	RegSetValueExW(key,L"matchwholeword",0,REG_DWORD,&d,sizeof(d));
	res = FindResourceW(instance,MAKEINTRESOURCEW(RID_FONT),RT_RCDATA);
	data = LockResource(LoadResource(0,res));
	size = SizeofResource(0,res);
	RegSetValueExW(key,L"font",0,REG_BINARY,data,size);
	RegCloseKey(key);

	RegCreateKeyW(HKEY_CURRENT_USER,L"software\\microsoft\\windows\\currentversion\\uninstall\\darkpad",&key);
	RegSetValueExW(key,L"DisplayName",0,REG_SZ,L"DarkPad",sizeof(L"DarkPad"));
	RegSetValueExW(key,L"DisplayVersion",0,REG_SZ,L"1",sizeof(L"1"));
	RegSetValueExW(key,L"Publisher",0,REG_SZ,L"Dark Software",sizeof(L"Dark Software"));
	RegSetValueExW(key,L"UninstallString",0,REG_SZ,path,StringLength(path)*2+2);
	SHGetSpecialFolderPathW(0,path,CSIDL_LOCAL_APPDATA,0);
	CopyString(EndOf(path),L"\\darkpad");
	RegSetValueExW(key,L"InstallLocation",0,REG_SZ,path,StringLength(path)*2+2);
	CopyString(EndOf(path),L"\\darkpad.exe");
	RegSetValueExW(key,L"DisplayIcon",0,REG_SZ,path,StringLength(path)*2+2);
	d = 20;
	RegSetValueExW(key,L"EstimatedSize",0,REG_DWORD,&d,sizeof(d));
	RegCloseKey(key);

#if CONSOLE
	while(1) Sleep(5);
#endif

	CoUninitialize();
	ExitProcess(0);
}