@echo off
if not exist out (mkdir out)
pushd out
set libs=libcmt.lib kernel32.lib libvcruntime.lib libucrt.lib ws2_32.lib
set clArgs=/nologo /WX /Zi /Od /Fehttp-server /Fdhttp-server ../tools/http_server.c /link /INCREMENTAL:NO /NODEFAULTLIB /VERBOSE:UNUSEDLIBS %libs%
cl.exe %clArgs% && http-server.exe
popd
