@echo off
if not defined DevEnvDir (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"
)
rm darkpad.exe
rc res.rc
cl /nologo /O1 /w /Gz /GS- darkpad.c /Fe"darkpad.exe" /link /nodefaultlib /subsystem:windows kernel32.lib shell32.lib gdi32.lib user32.lib ole32.lib uuid.lib dwmapi.lib comdlg32.lib res.res
mt.exe -manifest darkpad.exe.manifest -outputresource:darkpad.exe;1
rm darkpad.obj
darkpad.exe