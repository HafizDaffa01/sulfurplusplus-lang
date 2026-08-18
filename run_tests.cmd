@echo off
setlocal enabledelayedexpansion

set "PATH=%~dp0build;C:\msys64\mingw64\bin;%PATH%"
set "COMBUST=%~dp0build\combust.exe"

echo ===================================================
echo   Sulfur++ Automated Test Suite
echo ===================================================
echo.

set PASSED=0
set FAILED=0

for %%F in ("%~dp0tests\*.sfpp") do (
    echo [RUNNING] %%~nxF...
    "%COMBUST%" "%%F" > "%TEMP%\sulfur_test_out.txt" 2>&1
    if !ERRORLEVEL! EQU 0 (
        echo [PASS] %%~nxF
        set /a PASSED+=1
    ) else (
        echo [FAIL] %%~nxF
        type "%TEMP%\sulfur_test_out.txt"
        set /a FAILED+=1
    )
    echo ---------------------------------------------------
)

echo.
echo Test Summary:
echo   Passed: %PASSED%
echo   Failed: %FAILED%
echo ===================================================

if %FAILED% NEQ 0 exit /b 1
exit /b 0
