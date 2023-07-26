@echo off
if not defined DevEnvDir (
    (call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat") || (call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat")
)
cl /nologo /O1 /w iconwriter.c
iconwriter icon16.png icon32.png icon48.png icon256.png icon.ico
del iconwriter.obj
del iconwriter.exe
rc res.rc
cl /nologo /O1 /w /Gz /GS- darkpad.c /link /nodefaultlib /subsystem:windows kernel32.lib shell32.lib gdi32.lib user32.lib ole32.lib uuid.lib dwmapi.lib comdlg32.lib uxtheme.lib advapi32.lib comctl32.lib msvcrt.lib res.res
mt.exe -manifest darkpad.exe.manifest -outputresource:darkpad.exe;1
del darkpad.obj
del res.res
cl /nologo /O1 /w /Gz /GS- uninstaller.c /link /nodefaultlib /subsystem:windows kernel32.lib shell32.lib gdi32.lib user32.lib ole32.lib uuid.lib dwmapi.lib comdlg32.lib uxtheme.lib advapi32.lib msvcrt.lib
mt.exe -manifest uninstaller.exe.manifest -outputresource:uninstaller.exe;1
del uninstaller.obj
rc installerres.rc
cl /nologo /O1 /w /Gz /GS- installer.c /link /nodefaultlib /subsystem:windows kernel32.lib shell32.lib gdi32.lib user32.lib ole32.lib uuid.lib dwmapi.lib comdlg32.lib uxtheme.lib advapi32.lib msvcrt.lib installerres.res
mt.exe -manifest installer.exe.manifest -outputresource:installer.exe;1
del installer.obj
del installerres.res
del uninstaller.exe
del darkpad.exe
del icon.ico
