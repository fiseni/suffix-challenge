@echo off
setlocal enabledelayedexpansion

set "FLAGS=/permissive- /GS /GL /Gy /Gm- /W3 /WX- /O2 /Oi /sdl /Gd /MD /arch:AVX2 /EHsc /Zc:inline /fp:precise /Zc:forScope /nologo /D ""NDEBUG"" /D ""_CRT_SECURE_NO_WARNINGS"" /D ""_CONSOLE"""
set "FILES=main.c common.c cross_platform_time.c thread_utils.c hash_table_sizet.c hash_table_sizetlist.c source_data.c processor.c"

if exist publish (
    rmdir /s /q publish
)
mkdir publish

cl %FLAGS% %FILES% /Fe:publish\v1.exe
del *.obj

endlocal
