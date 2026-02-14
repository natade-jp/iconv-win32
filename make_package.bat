@echo off
setlocal

REM === 基準ディレクトリ（このbatの場所）===
set ROOT=%~dp0

REM === vcpkg の成果物 ===
set VCPKG_INSTALLED=%ROOT%vcpkg\installed\x86-windows

REM === 出力先 ===
set OUTDIR=%ROOT%dist

echo.
echo ========================================
echo Creating distribution layout...
echo ========================================

REM === dist を作り直す ===
if exist "%OUTDIR%" (
    echo Removing existing dist directory...
    rmdir /s /q "%OUTDIR%"
)

REM === フォルダ作成 ===
mkdir "%OUTDIR%\bin"
mkdir "%OUTDIR%\include"
mkdir "%OUTDIR%\lib"
mkdir "%OUTDIR%\LICENSES"

REM === DLL ===
if not exist "%VCPKG_INSTALLED%\bin\iconv-2.dll" (
    echo [ERROR] iconv-2.dll not found.
    pause
    exit /b 1
)
copy "%VCPKG_INSTALLED%\bin\iconv-2.dll" "%OUTDIR%\bin\" >nul

REM === ヘッダ ===
copy "%VCPKG_INSTALLED%\include\iconv.h" "%OUTDIR%\include\" >nul

REM === .lib ===
copy "%VCPKG_INSTALLED%\lib\iconv.lib" "%OUTDIR%\lib\" >nul

REM === ライセンス ===
copy "%VCPKG_INSTALLED%\share\libiconv\copyright" "%OUTDIR%\LICENSES\libiconv-LICENSE.txt" >nul

echo.
echo ========================================
echo DONE
echo Output directory:
echo %OUTDIR%
echo ========================================

pause
endlocal
