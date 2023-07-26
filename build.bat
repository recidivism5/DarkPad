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
del darkpad.obj
del res.res
rc uninstallerres.rc
cl /nologo /O1 /w /Gz /GS- uninstaller.c /link /nodefaultlib /subsystem:windows kernel32.lib shell32.lib gdi32.lib user32.lib ole32.lib uuid.lib dwmapi.lib comdlg32.lib uxtheme.lib advapi32.lib msvcrt.lib uninstallerres.res
del uninstallerres.res
del uninstaller.obj
rc installerres.rc
cl /nologo /O1 /w /Gz /GS- installer.c /link /nodefaultlib /subsystem:windows kernel32.lib shell32.lib gdi32.lib user32.lib ole32.lib uuid.lib dwmapi.lib comdlg32.lib uxtheme.lib advapi32.lib msvcrt.lib installerres.res
del installer.obj
del installerres.res
del uninstaller.exe
del darkpad.exe
del icon.ico
