@echo off
setlocal enabledelayedexpansion

set "FLAGS=/permissive- /GS /GL /Gy /Gm- /W3 /WX- /O2 /Oi /sdl /Gd /MD /arch:AVX2 /EHsc /Zc:inline /fp:precise /Zc:forScope /nologo /D ""NDEBUG"" /D ""_CRT_SECURE_NO_WARNINGS"" /D ""_CONSOLE"""
set "FILES=main.c cross_platform_time.c mmap_utils.c thread_utils.c hash_table.c source_data.c processor.c"

if exist publish (
    rmdir /s /q publish
)
mkdir publish

cl %FLAGS% %FILES% /Fe:publish\app.exe
del *.obj

endlocal
