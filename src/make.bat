@echo off
setlocal enabledelayedexpansion

:: 1. Configuration
set ROM_NAME=game
set SOURCE_FILE=main.c

:: 2. Hardcoded devkitPro Paths (Standard Windows installation paths)
set DEVKITPRO=C:\devkitPro
set DEVKITARM=C:\devkitPro\devkitARM

:: Set up paths to the compiler tools
set CC="%DEVKITARM%\bin\arm-none-eabi-gcc"
set OBJCOPY="%DEVKITARM%\bin\arm-none-eabi-objcopy"
set GBAFIX="%DEVKITPRO%\tools\bin\gbafix"

:: 2. Verify devkitARM environment variables
if "%DEVKITARM%"=="" (
    echo Error: DEVKITARM environment variable is not set.
    echo Please ensure devkitPro is installed correctly.
    pause
    exit /b 1
)

:: Set up paths to the compiler tools
set CC="%DEVKITARM%\bin\arm-none-eabi-gcc"
set OBJCOPY="%DEVKITARM%\bin\arm-none-eabi-objcopy"
set GBAFIX="%DEVKITPRO%\tools\bin\gbafix"

:: 3. Check if source file exists
if not exist %SOURCE_FILE% (
    echo Error: %SOURCE_FILE% not found in this directory.
    pause
    exit /b 1
)

echo Starting GBA build pipeline for %ROM_NAME%.gba...
echo ----------------------------------------------------

:: Step A: Compile source file to object file
echo [1/4] Compiling %SOURCE_FILE%...
%CC% -c %SOURCE_FILE% -mthumb -mthumb-interwork -mcpu=arm7tdmi -O3 -o %ROM_NAME%.o
if %errorlevel% neq 0 goto :error

:: Step B: Link object file into ELF binary
echo [2/4] Linking binary...
%CC% %ROM_NAME%.o -mthumb -mthumb-interwork -specs=gba.specs -o %ROM_NAME%.elf
if %errorlevel% neq 0 goto :error

:: Step C: Strip ELF to raw binary
echo [3/4] Extracting raw GBA ROM...
%OBJCOPY% -O binary %ROM_NAME%.elf %ROM_NAME%.gba
if %errorlevel% neq 0 goto :error

:: Step D: Fix Cartridge Header Checksum
echo [4/4] Fixing GBA header checksum...
if exist %GBAFIX% (
    %GBAFIX% %ROM_NAME%.gba
) else (
    echo Warning: gbafix tool not found at %GBAFIX%. ROM may not boot on real hardware/strict emulators.
)

:: 4. Clean up temporary files
echo ----------------------------------------------------
echo Cleaning up intermediate build files...
if exist %ROM_NAME%.o del %ROM_NAME%.o
if exist %ROM_NAME%.elf del %ROM_NAME%.elf

echo Build Successful! Generated %ROM_NAME%.gba
exit /b 1

:error
echo ----------------------------------------------------
echo Build FAILED during compilation pipeline.
pause
exit /b 1