@echo off
setlocal enabledelayedexpansion

REM === 基準ディレクトリ（このbatの場所）===
set ROOT=%~dp0

REM === vcpkg の成果物（両方）===
set VCPKG_X86=%ROOT%vcpkg\installed\x86-windows
set VCPKG_X64=%ROOT%vcpkg\installed\x64-windows

REM === 出力先 ===
set OUTDIR=%ROOT%dist

echo.
echo ========================================
echo Creating distribution layout (x86 + x64)...
echo ========================================

REM === dist を作り直す ===
if exist "%OUTDIR%" (
    echo Removing existing dist directory...
    rmdir /s /q "%OUTDIR%"
)

REM === フォルダ作成 ===
mkdir "%OUTDIR%\x86\bin"
mkdir "%OUTDIR%\x86\lib"
mkdir "%OUTDIR%\x64\bin"
mkdir "%OUTDIR%\x64\lib"
mkdir "%OUTDIR%\include"
mkdir "%OUTDIR%\LICENSES"

REM === x86 DLL/.lib チェック&コピー ===
if not exist "%VCPKG_X86%\bin\iconv-2.dll" (
    echo [ERROR] x86 iconv-2.dll not found: %VCPKG_X86%\bin\iconv-2.dll
    pause
    exit /b 1
)
if not exist "%VCPKG_X86%\lib\iconv.lib" (
    echo [ERROR] x86 iconv.lib not found: %VCPKG_X86%\lib\iconv.lib
    pause
    exit /b 1
)
copy /y "%VCPKG_X86%\bin\iconv-2.dll" "%OUTDIR%\x86\bin\" >nul
copy /y "%VCPKG_X86%\lib\iconv.lib" "%OUTDIR%\x86\lib\" >nul

REM === x64 DLL/.lib チェック&コピー ===
if not exist "%VCPKG_X64%\bin\iconv-2.dll" (
    echo [ERROR] x64 iconv-2.dll not found: %VCPKG_X64%\bin\iconv-2.dll
    pause
    exit /b 1
)
if not exist "%VCPKG_X64%\lib\iconv.lib" (
    echo [ERROR] x64 iconv.lib not found: %VCPKG_X64%\lib\iconv.lib
    pause
    exit /b 1
)
copy /y "%VCPKG_X64%\bin\iconv-2.dll" "%OUTDIR%\x64\bin\" >nul
copy /y "%VCPKG_X64%\lib\iconv.lib" "%OUTDIR%\x64\lib\" >nul

REM === ヘッダ（共通）===
if not exist "%VCPKG_X86%\include\iconv.h" (
    echo [ERROR] iconv.h not found: %VCPKG_X86%\include\iconv.h
    pause
    exit /b 1
)
copy /y "%VCPKG_X86%\include\iconv.h" "%OUTDIR%\include\" >nul

REM === ライセンス（共通）===
if not exist "%VCPKG_X86%\share\libiconv\copyright" (
    echo [ERROR] license file not found: %VCPKG_X86%\share\libiconv\copyright
    pause
    exit /b 1
)
copy /y "%VCPKG_X86%\share\libiconv\copyright" "%OUTDIR%\LICENSES\libiconv-LICENSE.txt" >nul

echo.
echo ========================================
echo DONE
echo Output directory:
echo %OUTDIR%
echo.
echo x86:
echo   %OUTDIR%\x86\bin\iconv-2.dll
echo   %OUTDIR%\x86\lib\iconv.lib
echo x64:
echo   %OUTDIR%\x64\bin\iconv-2.dll
echo   %OUTDIR%\x64\lib\iconv.lib
echo headers:
echo   %OUTDIR%\include\iconv.h
echo licenses:
echo   %OUTDIR%\LICENSES\libiconv-LICENSE.txt
echo ========================================

pause
endlocal
