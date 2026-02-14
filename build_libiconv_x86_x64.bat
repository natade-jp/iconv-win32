@echo off

REM ========= 設定 =========
set VCPKG_DIR=%~dp0vcpkg

REM ========= vcpkg が無ければ取得 =========
if not exist %VCPKG_DIR% (
    echo [INFO] Cloning vcpkg...
    git clone https://github.com/microsoft/vcpkg %VCPKG_DIR%
)

cd /d %VCPKG_DIR%

REM ========= vcpkg 初期化 =========
if not exist vcpkg.exe (
    echo [INFO] Bootstrapping vcpkg...
    call bootstrap-vcpkg.bat
)

REM ========= Visual Studio と統合（1回だけ必要） =========
echo [INFO] Integrating with Visual Studio...
vcpkg integrate install

REM ========= libiconv x86 DLL ビルド =========
echo [INFO] Installing libiconv for x86 (DLL)...
vcpkg install libiconv:x86-windows

echo.
echo ========================================
echo  完了
echo  DLL の場所:
echo  %VCPKG_DIR%\installed\x86-windows\bin
echo ========================================
pause
