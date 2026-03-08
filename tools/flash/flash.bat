@echo off
REM ============================================================
REM Mood_1.0 STM32F407 Flash Script (J-Link)
REM ============================================================
setlocal

set SCRIPT_DIR=%~dp0
for %%I in ("%SCRIPT_DIR%..\..") do set ROOT_DIR=%%~fI

set JLINK_EXE=D:\software\jlink\JLink_V916a\JLink.exe

echo.
echo ========================================
echo   Mood_1.0 J-Link Flash Tool
echo ========================================
echo.

REM 检查J-Link
if not exist "%JLINK_EXE%" (
    echo [ERROR] JLink.exe not found at: %JLINK_EXE%
    echo Please update JLINK_EXE path in this script.
    pause
    exit /b 1
)

REM 检查固件文件
if not exist "%ROOT_DIR%\Output\Mood_1.0.bin" (
    echo [ERROR] %ROOT_DIR%\Output\Mood_1.0.bin not found!
    echo Please build the project first: Ctrl+Shift+B
    pause
    exit /b 1
)

echo [INFO] Firmware: %ROOT_DIR%\Output\Mood_1.0.bin
for %%A in ("%ROOT_DIR%\Output\Mood_1.0.bin") do echo [INFO] Size: %%~zA bytes
echo.
echo [INFO] Starting J-Link...
echo.

pushd "%ROOT_DIR%"
"%JLINK_EXE%" -device STM32F407ZG -if SWD -speed 4000 -autoconnect 1 -CommandFile "%ROOT_DIR%\tools\flash\jlink_flash.jlink"
set FLASH_EXIT=%ERRORLEVEL%
popd

if not "%FLASH_EXIT%"=="0" (
    echo.
    echo [ERROR] Flash failed!
    pause
    exit /b %FLASH_EXIT%
)

echo.
echo ========================================
echo   Flash completed successfully!
echo ========================================
echo.
pause
endlocal
