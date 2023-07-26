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

#include "base.h"
#include "extensions.h"

u16 *EndOf(u16 *s){
	while (*s) s++;
	return s;
}
HINSTANCE instance;
u16 path[MAX_PATH];
void WinMainCRTStartup(){
	instance = GetModuleHandleW(0);
	CoInitialize(0);

	u16 *prgs;
    SHGetKnownFolderPath(&FOLDERID_Programs,0,0,&prgs);
	_snwprintf(path,COUNT(path),L"%s%s",prgs,L"\\DarkPad.lnk");
	DeleteFileW(path);

	RegDeleteKeyW(HKEY_CURRENT_USER,L"software\\darkpad");
	RegDeleteKeyW(HKEY_CURRENT_USER,L"software\\microsoft\\windows\\currentversion\\uninstall\\darkpad");

	HKEY key;
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
					if ((size==12*2 || size==11*2) && !_wcsnicmp(path,L"darkpad.exe",size/2)){
						RegDeleteValueW(key,c);
						size = sizeof(path);
						i32 count;
						if (ERROR_SUCCESS == RegQueryValueExW(key,L"mrulist",0,&type,path,&size)){
							count = size/2;
							for (i32 j = 0; j < count; j++){
								if (path[j]==c[0]){
									memmove(path+j,path+j+1,(count-1-j)*sizeof(u16));
									count--;
									if (path[count-1]){
										path[count] = 0;
										count++;
									}
									RegSetValueExW(key,L"mrulist",0,REG_SZ,path,count*2);
								}
							}
						}
					}
				} else {
					_snwprintf(path,COUNT(path),L".%s Explorer FileExts registry key is corrupted.",extensions[i]);
					MessageBoxW(0,path,L"Error",0);
					break;
				}
				c[0]++;
				if (c[0]>'z') break;
			}
			RegCloseKey(key);
		}
	}
	
	wcscpy(path,L"/c cd .. & rmdir /s /q ");
	SHGetSpecialFolderPathW(0,EndOf(path),CSIDL_LOCAL_APPDATA,0);
	wcscat(path,L"\\darkpad >> NUL");
	u16 cmdPath[MAX_PATH];
	GetEnvironmentVariableW(L"ComSpec",cmdPath,MAX_PATH);
    ShellExecuteW(0,0,cmdPath,path,0,SW_HIDE);

	CoUninitialize();
	ExitProcess(0);
}