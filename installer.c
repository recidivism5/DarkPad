/*
- put darkpad.exe and uninstall.exe in C:\Users\destr\AppData\Local\darkpad (https://stackoverflow.com/questions/2489613/how-to-get-system-folder-pathc-windows-c-program-files-in-windows-using-c)
- store in HKEY_CURRENT_USER\SOFTWARE\darkpad:
    - word wrap = 1
    - font = consolas 12 pt
    - match case = 0
    - match whole word = 0
- link uninstall.exe in HKEY_CURRENT_USER\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall (https://learn.microsoft.com/en-us/windows/win32/msi/configuring-add-remove-programs-with-windows-installer) (https://learn.microsoft.com/en-us/windows/win32/msi/uninstall-registry-key)
- put shortcut in %appdata%\Microsoft\Windows\Start Menu\Programs (https://gist.github.com/abel0b/0b740648d6370e3e77fd70a816a34523) or maybe (https://learn.microsoft.com/en-us/windows/win32/shell/how-to-add-shortcuts-to-the-start-menu)
*/

#include "base.h"
#include "extensions.h"
#include "installerres.h"

HINSTANCE instance;
u16 path[MAX_PATH];
void WinMainCRTStartup(){
	instance = GetModuleHandleW(0);
	CoInitialize(0);

	SHGetSpecialFolderPathW(0,path,CSIDL_LOCAL_APPDATA,0);
	wcscat(path,L"\\darkpad");
	CreateDirectoryW(path,0);

	wcscat(path,L"\\darkpad.exe");
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
	_snwprintf(path,COUNT(path),L"%s%s",prgs,L"\\DarkPad.lnk");
	ppf->lpVtbl->Save(ppf,path,1);
    ppf->lpVtbl->Release(ppf);
	psl->lpVtbl->Release(psl);
	CoTaskMemFree(prgs);

	SHGetSpecialFolderPathW(0,path,CSIDL_LOCAL_APPDATA,0);
	wcscat(path,L"\\darkpad\\uninstaller.exe");
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
	RegSetValueExW(key,L"lineending",0,REG_DWORD,&d,sizeof(d));
	d = 0;
	RegSetValueExW(key,L"matchcase",0,REG_DWORD,&d,sizeof(d));
	res = FindResourceW(instance,MAKEINTRESOURCEW(RID_FONT),RT_RCDATA);
	data = LockResource(LoadResource(0,res));
	size = SizeofResource(0,res);
	RegSetValueExW(key,L"font",0,REG_BINARY,data,size);
	RegCloseKey(key);

	RegCreateKeyW(HKEY_CURRENT_USER,L"software\\microsoft\\windows\\currentversion\\uninstall\\darkpad",&key);
	RegSetValueExW(key,L"DisplayName",0,REG_SZ,L"DarkPad",sizeof(L"DarkPad"));
	RegSetValueExW(key,L"DisplayVersion",0,REG_SZ,L"1",sizeof(L"1"));
	RegSetValueExW(key,L"Publisher",0,REG_SZ,L"Dark Software",sizeof(L"Dark Software"));
	RegSetValueExW(key,L"UninstallString",0,REG_SZ,path,wcslen(path)*2+2);
	SHGetSpecialFolderPathW(0,path,CSIDL_LOCAL_APPDATA,0);
	wcscat(path,L"\\darkpad");
	RegSetValueExW(key,L"InstallLocation",0,REG_SZ,path,wcslen(path)*2+2);
	wcscat(path,L"\\darkpad.exe");
	RegSetValueExW(key,L"DisplayIcon",0,REG_SZ,path,wcslen(path)*2+2);
	d = 20;
	RegSetValueExW(key,L"EstimatedSize",0,REG_DWORD,&d,sizeof(d));
	RegCloseKey(key);

	for (i32 i = 0; i < COUNT(extensions); i++){
		_snwprintf(path,COUNT(path),L"software\\microsoft\\windows\\currentversion\\explorer\\fileexts\\.%s\\openwithlist",extensions[i]);
		if (ERROR_SUCCESS == RegOpenKeyW(HKEY_CURRENT_USER,path,&key)){
			u16 c[2] = L"a";
			DWORD type;
			DWORD size;
			while (1){
				size = sizeof(path);
				if (ERROR_SUCCESS != RegQueryValueExW(key,c,0,&type,path,&size)) break;
				if (type==REG_SZ && size){
					if ((size==12*2 || size==11*2) && !_wcsnicmp(path,L"darkpad.exe",size/2)) goto NOCREATE;
				} else {
					_snwprintf(path,COUNT(path),L".%s Explorer FileExts registry key is corrupted.",extensions[i]);
					MessageBoxW(0,path,L"Error",0);
					goto NEXT;
				}
				c[0]++;
				if (c[0]>'z') goto NEXT;
			}
			RegSetValueExW(key,c,0,REG_SZ,L"darkpad.exe",sizeof(L"darkpad.exe"));
			NOCREATE:
			size = sizeof(path);
			i32 count;
			if (ERROR_SUCCESS == RegQueryValueExW(key,L"mrulist",0,&type,path,&size)) count = size/2;
			else count = 0;
			for (i32 j = 0; j < count; j++){
				if (path[j]==c[0]) goto NEXT;
			}
			if (count && !path[count-1]){
				path[count-1] = c[0];
				path[count] = 0;
				count += 1;
			} else {
				path[count] = c[0];
				path[count+1] = 0;
				count += 2;
			}
			RegSetValueExW(key,L"mrulist",0,REG_SZ,path,count*2);
			NEXT:
			RegCloseKey(key);
		}
	}

	CoUninitialize();
	ExitProcess(0);
}