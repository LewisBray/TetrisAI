@ECHO OFF

SET "msvcpath=C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.29.30133"
SET "sdkpath=C:\Program Files (x86)\Windows Kits\10"
SET vctoolspath="%msvcpath%\bin\Hostx64\x64"
SET vslibspath="%msvcpath%\lib\x64"
SET systemlibspath="%sdkpath%\Lib\10.0.19041.0\um\x64"
SET crtpath="%sdkpath%\Lib\10.0.19041.0\ucrt\x64"
SET rctoolpath="%sdkpath%\bin\10.0.19041.0\x64"

SET "clang_compiler_flags=-c -std=c++1z -g -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic"

clang src\tetris_ai.cpp %clang_compiler_flags%
%vctoolspath%\LINK tetris_ai.o ucrt.lib vcruntime.lib msvcrt.lib /DLL /EXPORT:initialise_game /EXPORT:update_game /EXPORT:render_game /OUT:tetris_ai.dll /DEBUG /LIBPATH:%systemlibspath% /LIBPATH:%vslibspath% /LIBPATH:%crtpath% /SUBSYSTEM:WINDOWS /INCREMENTAL:NO

clang src\tetris_ai_win32.cpp %clang_compiler_flags%
%rctoolpath%\rc.exe /fo resource.res src\resource.rc
%vctoolspath%\LINK tetris_ai_win32.o resource.res kernel32.lib user32.lib gdi32.lib opengl32.lib ucrt.lib vcruntime.lib msvcrt.lib /OUT:tetris_ai_win32.exe /DEBUG /LIBPATH:%systemlibspath% /LIBPATH:%vslibspath% /LIBPATH:%crtpath% /SUBSYSTEM:WINDOWS /INCREMENTAL:NO
