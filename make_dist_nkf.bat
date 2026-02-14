@echo off
setlocal

REM === 基準ディレクトリ（このbatの場所）===
set "ROOT=%~dp0"

REM === nkf の成果物がある場所 ===
set "NKF_DIR=%ROOT%nkf"

REM === 出力先 ===
set "OUTDIR=%ROOT%dist\nkf"

echo(
echo ========================================
echo Creating nkf distribution layout
echo ========================================
echo(ROOT    = "%ROOT%"
echo(NKF_DIR = "%NKF_DIR%"
echo(OUTDIR  = "%OUTDIR%"
echo(

REM === dist\nkf を作り直す ===
if exist "%OUTDIR%" (
    echo(Removing existing "%OUTDIR%" ...
    rmdir /s /q "%OUTDIR%"
    if errorlevel 1 (
        echo([ERROR] Failed to remove "%OUTDIR%"
        pause
        exit /b 1
    )
)

REM === フォルダ作成 ===
mkdir "%OUTDIR%" >nul 2>nul
mkdir "%OUTDIR%\x86\bin" >nul 2>nul
mkdir "%OUTDIR%\x86\lib" >nul 2>nul
mkdir "%OUTDIR%\x64\bin" >nul 2>nul
mkdir "%OUTDIR%\x64\lib" >nul 2>nul
mkdir "%OUTDIR%\include" >nul 2>nul
mkdir "%OUTDIR%\LICENSES" >nul 2>nul

REM === x86 DLL ===
if not exist "%NKF_DIR%\nkf32_x86.dll" (
    echo([ERROR] x86 DLL not found:
    echo(  "%NKF_DIR%\nkf32_x86.dll"
    echo(  Run build_nkf first.
    pause
    exit /b 1
)
copy /y "%NKF_DIR%\nkf32_x86.dll" "%OUTDIR%\x86\bin\nkf32.dll" >nul

REM === x64 DLL ===
if not exist "%NKF_DIR%\nkf32_x64.dll" (
    echo([ERROR] x64 DLL not found:
    echo(  "%NKF_DIR%\nkf32_x64.dll"
    echo(  Run build_nkf first.
    pause
    exit /b 1
)
copy /y "%NKF_DIR%\nkf32_x64.dll" "%OUTDIR%\x64\bin\nkf32.dll" >nul

REM === import lib（存在すれば）===
if exist "%NKF_DIR%\nkf32_x86.lib" (
    copy /y "%NKF_DIR%\nkf32_x86.lib" "%OUTDIR%\x86\lib\nkf32.lib" >nul
)

if exist "%NKF_DIR%\nkf32_x64.lib" (
    copy /y "%NKF_DIR%\nkf32_x64.lib" "%OUTDIR%\x64\lib\nkf32.lib" >nul
)

REM === ヘッダ ===
if not exist "%NKF_DIR%\nkf32.h" (
    echo([ERROR] nkf32.h not found:
    echo(  "%NKF_DIR%\nkf32.h"
    pause
    exit /b 1
)
copy /y "%NKF_DIR%\nkf32.h" "%OUTDIR%\include\" >nul

if exist "%NKF_DIR%\nkf.h" (
    copy /y "%NKF_DIR%\nkf.h" "%OUTDIR%\include\" >nul
)

REM === ライセンス探索 ===
set "LICENSE_FOUND=0"
for %%F in (LICENSE LICENSE.txt LICENSE.md COPYING COPYING.txt COPYRIGHT COPYRIGHT.txt) do (
    if exist "%NKF_DIR%\%%F" (
        copy /y "%NKF_DIR%\%%F" "%OUTDIR%\LICENSES\nkf-%%F" >nul
        set "LICENSE_FOUND=1"
    )
)

if "%LICENSE_FOUND%"=="0" (
    echo([WARN] License file not found in "%NKF_DIR%"
    echo(      Please copy it manually into:
    echo(      "%OUTDIR%\LICENSES\"
)

echo(
echo ========================================
echo DONE
echo(
echo(x86:
echo(  %OUTDIR%\x86\bin\nkf32.dll
if exist "%OUTDIR%\x86\lib\nkf32.lib" echo(  %OUTDIR%\x86\lib\nkf32.lib
echo(
echo(x64:
echo(  %OUTDIR%\x64\bin\nkf32.dll
if exist "%OUTDIR%\x64\lib\nkf32.lib" echo(  %OUTDIR%\x64\lib\nkf32.lib
echo(
echo(headers:
echo(  %OUTDIR%\include\nkf32.h
if exist "%OUTDIR%\include\nkf.h" echo(  %OUTDIR%\include\nkf.h
echo(
echo(licenses:
echo(  %OUTDIR%\LICENSES\
echo ========================================

pause
endlocal
