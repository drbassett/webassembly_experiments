@echo off

setlocal

set projectDir=%1
if [%projectDir%] == [] (
	%projectDir% = minimal
) else (
	if not exist %projectDir% (
		echo Specified project directory does not exist: "%projectDir%"
		goto:eof
	)
)

set rootDir=%~dp0
set rootDir="%rootDir:~0,-1%"
set outDir=%rootDir%\out

set emccDebugFlags=-O0 -g --emrun
set emccReleaseFlags=-O3
set emccConfigFlags=%emccDebugFlags%
set emccFlags=-fno-exceptions -fno-rtti %emccConfigFlags%

if not exist %outDir% (mkdir %outDir%)
pushd %rootDir%/%projectDir%
call emcc.bat %emccFlags% -o %outDir%\main.js main.c ^
	&& copy index.html %outDir% > nul
popd

endlocal
