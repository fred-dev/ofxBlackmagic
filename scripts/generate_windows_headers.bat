@echo off
rem Generates the DeckLink API header for Windows from the SDK's IDL files.
rem On Windows the DeckLink SDK only ships .idl files; Microsoft's MIDL compiler
rem turns them into DeckLinkAPI.h and DeckLinkAPI_i.c. Run this ONCE (and again
rem after updating the SDK in libs/DeckLink/Win/idl), then commit the two files.
rem
rem Run it from a "x64 Native Tools Command Prompt for VS" (it has midl.exe on the PATH).

setlocal
set ROOT=%~dp0..\libs\DeckLink\Win
set TMPDIR=%ROOT%\midl_tmp

where midl >nul 2>nul
if errorlevel 1 (
	echo midl.exe not found: run this from the "x64 Native Tools Command Prompt for VS".
	exit /b 1
)

if not exist "%TMPDIR%" mkdir "%TMPDIR%"
pushd "%ROOT%\idl"
midl /nologo /env x64 /out "%TMPDIR%" /h DeckLinkAPI.h /iid DeckLinkAPI_i.c DeckLinkAPI.idl
if errorlevel 1 (
	popd
	echo MIDL failed.
	exit /b 1
)
popd

copy /y "%TMPDIR%\DeckLinkAPI.h" "%ROOT%\include\DeckLinkAPI.h" >nul
copy /y "%TMPDIR%\DeckLinkAPI_i.c" "%ROOT%\src\DeckLinkAPI_i.c" >nul
rmdir /s /q "%TMPDIR%"
echo Done: libs\DeckLink\Win\include\DeckLinkAPI.h and libs\DeckLink\Win\src\DeckLinkAPI_i.c
endlocal
