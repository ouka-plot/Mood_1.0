@echo off
REM ============================================================
REM Mood_1.0 STM32F407 Build Script (Docker)
REM ============================================================

setlocal enabledelayedexpansion

set PROJECT_DIR=%cd%
set DOCKER_IMAGE=mood-compiler:latest

echo.
echo ========================================
echo   Mood_1.0 Build System
echo ========================================
echo.

REM 检查Docker
docker version >nul 2>&1
if errorlevel 1 (
    echo [ERROR] Docker not found or not running!
    echo Please start Docker Desktop.
    pause
    exit /b 1
)

REM 检查Docker镜像
docker image inspect %DOCKER_IMAGE% >nul 2>&1
if errorlevel 1 (
    echo [INFO] Docker image not found, building...
    docker build -f Dockerfile.optimized -t %DOCKER_IMAGE% .
    if errorlevel 1 (
        echo [ERROR] Failed to build Docker image!
        pause
        exit /b 1
    )
)

REM 解析参数
if "%1"=="" goto build_all
if "%1"=="clean" goto clean
if "%1"=="rebuild" goto rebuild
if "%1"=="debug" goto debug
if "%1"=="flash" goto flash
if "%1"=="help" goto help
goto invalid

:build_all
echo [INFO] Building project...
docker run --rm -v "%PROJECT_DIR%:/workspace" -w /workspace %DOCKER_IMAGE% make all
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
goto show_output

:clean
echo [INFO] Cleaning...
docker run --rm -v "%PROJECT_DIR%:/workspace" -w /workspace %DOCKER_IMAGE% make clean
echo [DONE] Clean complete!
exit /b 0

:rebuild
echo [INFO] Clean rebuild...
docker run --rm -v "%PROJECT_DIR%:/workspace" -w /workspace %DOCKER_IMAGE% make clean all
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
goto show_output

:debug
echo [INFO] Building debug version...
docker run --rm -v "%PROJECT_DIR%:/workspace" -w /workspace %DOCKER_IMAGE% make debug
if errorlevel 1 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
goto show_output

:flash
echo [INFO] Flashing via J-Link...
call flash.bat
exit /b 0

:help
echo Usage: build.bat [command]
echo.
echo Commands:
echo   (none)   Build project
echo   clean    Clean build artifacts
echo   rebuild  Clean and rebuild
echo   debug    Build with debug symbols
echo   flash    Flash to device via J-Link
echo   help     Show this message
echo.
exit /b 0

:invalid
echo [ERROR] Unknown command: %1
echo Run 'build.bat help' for usage.
exit /b 1

:show_output
echo.
echo ========================================
echo   Build Complete!
echo ========================================
if exist "Output\Mood_1.0.elf" (
    echo.
    echo Output files:
    dir /b Output\Mood_1.0.* 2>nul
)
echo.
pause
exit /b 0
