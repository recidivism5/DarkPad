@echo off
if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)
rc res.rc
cl /nologo /O1 /w /Gz /GS- darkpad.c /Fe"darkpad.exe" /link /nodefaultlib /subsystem:windows kernel32.lib shell32.lib gdi32.lib user32.lib uxtheme.lib ole32.lib uuid.lib res.res
rm darkpad.obj
darkpad
