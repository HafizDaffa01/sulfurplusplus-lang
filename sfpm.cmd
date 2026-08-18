@echo off
setlocal
set "PATH=%~dp0build;C:\msys64\mingw64\bin;%PATH%"
"%~dp0build\sfpm.exe" %*
endlocal
