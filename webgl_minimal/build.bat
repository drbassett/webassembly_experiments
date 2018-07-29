@echo off
if exist out (rmdir /q /s out)
mkdir out || goto:eof
call emcc.bat -fno-exceptions --emrun -o out\main.js main.c ^
	&& copy index.html out > nul ^
	&& emrun --no_browser out\index.html
