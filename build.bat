@echo off
setlocal enabledelayedexpansion
REM ============================================================
REM   Space Invaders - one-command build script (MinGW / MSYS2)
REM
REM   Compiles main.cpp with raylib and produces SpaceInvaders.exe
REM   in this same folder. Requires:
REM     - MSYS2 ucrt64 toolchain (g++)
REM     - raylib installed at C:\raylib (include + lib)
REM ============================================================

set "GXX=C:\msys64\ucrt64\bin\g++.exe"
set "RAYLIB_DIR=C:\raylib"
set "OUTPUT=SpaceInvaders.exe"

if not exist "%GXX%" goto :no_compiler
if not exist "%RAYLIB_DIR%\include\raylib.h" goto :no_raylib

echo Building Space Invaders...

"%GXX%" main.cpp -o "%OUTPUT%" -I "%RAYLIB_DIR%\include" -L "%RAYLIB_DIR%\lib" -lraylib -lopengl32 -lgdi32 -lwinmm -static-libgcc -static-libstdc++ -std=c++17 -O2

if not errorlevel 1 goto :ok

echo.
echo [FAILED] Build failed. Review the errors above.
exit /b 1

:ok
echo.
echo [OK] Build complete: %OUTPUT%
echo Run the game with:   %OUTPUT%
exit /b 0

:no_compiler
echo [ERROR] MinGW g++ not found at: %GXX%
echo         Install MSYS2 (ucrt64 toolchain), then re-run this script.
exit /b 1

:no_raylib
echo [ERROR] raylib not found at: %RAYLIB_DIR%\include\raylib.h
exit /b 1