@echo off
setlocal enabledelayedexpansion

REM ========= ê›íË =========
set "VCPKG_DIR=%~dp0vcpkg"

REM ========= vcpkg Ç™ñ≥ÇØÇÍÇŒéÊìæ =========
if not exist "%VCPKG_DIR%\" (
    echo [INFO] Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg "%VCPKG_DIR%"
    if errorlevel 1 (
        echo [ERROR] git clone failed.
        pause
        exit /b 1
    )
)

cd /d "%VCPKG_DIR%"

REM ========= vcpkg èâä˙âª =========
if not exist "vcpkg.exe" (
    echo [INFO] Bootstrapping vcpkg...
    call "bootstrap-vcpkg.bat"
    if errorlevel 1 (
        echo [ERROR] bootstrap-vcpkg.bat failed.
        pause
        exit /b 1
    )
)

REM ========= Visual Studio Ç∆ìùçáÅièââÒÇæÇØÇ≈OKÅj =========
if not exist "%LOCALAPPDATA%\vcpkg\vcpkg.user.props" (
    echo [INFO] Integrating with Visual Studio...
    vcpkg integrate install
    if errorlevel 1 (
        echo [ERROR] vcpkg integrate install failed.
        pause
        exit /b 1
    )
) else (
    echo [INFO] vcpkg already integrated. (skip)
)

REM ========= libiconv x86/x64 DLL =========
echo [INFO] Installing libiconv for x86 (DLL)...
vcpkg install libiconv:x86-windows
if errorlevel 1 (
    echo [ERROR] vcpkg install libiconv:x86-windows failed.
    pause
    exit /b 1
)

echo [INFO] Installing libiconv for x64 (DLL)...
vcpkg install libiconv:x64-windows
if errorlevel 1 (
    echo [ERROR] vcpkg install libiconv:x64-windows failed.
    pause
    exit /b 1
)

echo.
echo ========================================
echo  äÆóπ
echo.
echo  x86 DLL:
echo  %VCPKG_DIR%\installed\x86-windows\bin
echo.
echo  x64 DLL:
echo  %VCPKG_DIR%\installed\x64-windows\bin
echo ========================================
pause
endlocal
