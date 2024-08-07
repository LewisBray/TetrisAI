@ECHO OFF

SET "msvc_path=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.38.33130"
SET "sdk_path=C:\Program Files (x86)\Windows Kits\10"
SET vc_tools_path="%msvc_path%\bin\Hostx64\x64"
SET system_libs_path="%sdk_path%\Lib\10.0.22621.0\um\x64"
SET rc_tool_path="%sdk_path%\bin\10.0.22621.0\x64"

SET "common_compiler_flags=-c -g"
REM SET "clang_cpp_compiler_flags=-std=c++1z -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic"
SET "clang_cpp_compiler_flags=-std=c++1z"
SET "common_linker_flags=/NODEFAULTLIB /DEBUG /SUBSYSTEM:WINDOWS /INCREMENTAL:NO"

clang src\tetris_ai.cpp %common_compiler_flags% %clang_cpp_compiler_flags%
clang src\crt.c src\dllmain_crt_win32.c %common_compiler_flags%
%vc_tools_path%\LINK tetris_ai.o crt.o dllmain_crt_win32.o %common_linker_flags% /DLL /EXPORT:initialise_game /EXPORT:update_game /EXPORT:render_game /OUT:tetris_ai.dll

clang src\tetris_ai_win32.cpp %common_compiler_flags% %clang_cpp_compiler_flags%
clang src\crt.c %common_compiler_flags%
%rc_tool_path%\rc.exe /fo resource.res src\resource.rc
%vc_tools_path%\LINK tetris_ai_win32.o crt.o resource.res kernel32.lib user32.lib gdi32.lib opengl32.lib %common_linker_flags% /STACK:0x100000,0x100000 /LIBPATH:%system_libs_path% /OUT:tetris_ai_win32.exe
