@echo off
setlocal enabledelayedexpansion

set "ROOT=%~dp0"
set "NKF_DIR=%ROOT%nkf"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat"

echo.
echo ========================================
echo build_nkf.bat (debug)
echo ========================================

echo [DBG] ROOT=%ROOT%
echo [DBG] NKF_DIR=%NKF_DIR%
echo [DBG] VCVARS=%VCVARS%

if not exist "%VCVARS%" (
  echo [ERROR] vcvarsall.bat not found:
  echo   "%VCVARS%"
  exit /b 1
)

if not exist "%NKF_DIR%\" (
  echo [INFO] Cloning nkf...
  git clone https://github.com/nurse/nkf "%NKF_DIR%"
  echo [DBG] git clone errorlevel=!errorlevel!
  if errorlevel 1 exit /b 1
) else (
  echo [INFO] nkf already exists. (skip clone)
)

for %%A in (x86 x64) do (
  echo.
  echo ----------------------------------------
  echo [INFO] Build for %%A
  echo ----------------------------------------

  echo [DBG] call vcvarsall %%A
  call "%VCVARS%" %%A
  echo [DBG] vcvarsall errorlevel=!errorlevel!
  if errorlevel 1 exit /b 1

  echo [DBG] pushd "%NKF_DIR%"
  pushd "%NKF_DIR%"
  echo [DBG] now at: %CD%

  echo [DBG] nmake clean
  nmake /nologo /f nkf.mak clean
  echo [DBG] nmake clean errorlevel=!errorlevel!
  if errorlevel 1 (
    popd
    exit /b 1
  )

  echo [DBG] nmake nkf32.dll
  nmake /nologo /f nkf.mak nkf32.dll
  echo [DBG] nmake build errorlevel=!errorlevel!
  if errorlevel 1 (
    popd
    exit /b 1
  )

  echo [DBG] after build dir nkf32.*
  dir /b nkf32.* 2>nul

  if errorlevel 1 exit /b 1

  echo [DBG] stash start for %%A
  if /i "%%A"=="x86" (
    del /q "nkf32_x86.dll" 2>nul
    move /y "nkf32.dll" "nkf32_x86.dll"
    echo [DBG] move dll errorlevel=!errorlevel!
    if exist "nkf32.lib" (
      del /q "nkf32_x86.lib" 2>nul
      move /y "nkf32.lib" "nkf32_x86.lib"
      echo [DBG] move lib errorlevel=!errorlevel!
    )
  ) else (
    del /q "nkf32_x64.dll" 2>nul
    move /y "nkf32.dll" "nkf32_x64.dll"
    echo [DBG] move dll errorlevel=!errorlevel!
    if exist "nkf32.lib" (
      del /q "nkf32_x64.lib" 2>nul
      move /y "nkf32.lib" "nkf32_x64.lib"
      echo [DBG] move lib errorlevel=!errorlevel!
    )
  )

  echo [INFO] Stashed:
  if /i "%%A"=="x86" (
    echo   %NKF_DIR%\nkf32_x86.dll
    if exist "nkf32_x86.lib" echo   %NKF_DIR%\nkf32_x86.lib
  ) else (
    echo   %NKF_DIR%\nkf32_x64.dll
    if exist "nkf32_x64.lib" echo   %NKF_DIR%\nkf32_x64.lib
  )

  popd
  echo [DBG] popd done
)

echo.
echo ========================================
echo DONE
echo %NKF_DIR%
echo ========================================
pause
endlocal
