@echo off
if not exist out (mkdir out || goto:eof)
call emcc.bat -fno-exceptions --emrun -o out\main.js main.c ^
	&& copy index.html out > nul
