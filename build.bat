@ECHO OFF

SET "msvcpath=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133"
SET "sdkpath=C:\Program Files (x86)\Windows Kits\10"
SET vctoolspath="%msvcpath%\bin\Hostx64\x64"
SET vslibspath="%msvcpath%\lib\x64"
SET systemlibspath="%sdkpath%\Lib\10.0.19041.0\um\x64"
SET crtpath="%sdkpath%\Lib\10.0.19041.0\ucrt\x64"
SET rctoolpath="%sdkpath%\bin\10.0.19041.0\x64"

clang src\main.cpp -c -std=c++1z -g -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic
%rctoolpath%\rc.exe /fo resource.res src\resource.rc
%vctoolspath%\LINK main.o resource.res kernel32.lib user32.lib gdi32.lib opengl32.lib ucrt.lib vcruntime.lib msvcrt.lib /OUT:tetris_ai.exe /DEBUG /LIBPATH:%systemlibspath% /LIBPATH:%vslibspath% /LIBPATH:%crtpath% /SUBSYSTEM:WINDOWS /INCREMENTAL:NO
