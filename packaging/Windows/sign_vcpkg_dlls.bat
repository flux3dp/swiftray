@echo off
setlocal
rem Usage: sign_vcpkg_dlls.bat [vcpkg_installed\x64-windows]
rem Run on the packaging machine. Sends every unsigned release dll under vcpkg_installed to the
rem code sign server (same HTTP endpoint the Beam Studio CD uses) and replaces it with the signed
rem copy. The build copies these dlls next to Swiftray.exe, so signed sources give a signed
rem package. Re-run after vcpkg reinstalls or upgrades packages: that restores unsigned dlls.
rem Windows 11 Smart App Control blocks unsigned dlls with "Bad Image" 0xc0e90002.
rem
rem Needs two environment variables (never commit them). In PowerShell use $env:SIGN_SERVER = "..."
rem because PowerShell's `set` makes a shell variable that this script cannot see.
rem   SIGN_SERVER      e.g. http://192.168.1.20:8000/
rem   SIGN_CREDENTIAL  the value of the Credential header

set ROOT=%~1
if "%ROOT%"=="" set ROOT=%~dp0..\..\vcpkg_installed\x64-windows
if not exist "%ROOT%" (
  echo not found: "%ROOT%"
  exit /b 1
)
if "%SIGN_SERVER%"=="" (
  echo SIGN_SERVER is not set. cmd: set SIGN_SERVER=http://ip:port/   PowerShell: $env:SIGN_SERVER = "http://ip:port/"
  exit /b 1
)
if "%SIGN_CREDENTIAL%"=="" (
  echo SIGN_CREDENTIAL is not set. cmd: set SIGN_CREDENTIAL=...   PowerShell: $env:SIGN_CREDENTIAL = "..."
  exit /b 1
)

rem signtool lives in bin\x64 on some SDK installs and in bin\<version>\x64 on others; take the newest
set "PATH=%ProgramFiles(x86)%\Windows Kits\10\bin\x64;%PATH%"
for /d %%d in ("%ProgramFiles(x86)%\Windows Kits\10\bin\10.*") do if exist "%%d\x64\signtool.exe" set "SIGNTOOL_DIR=%%d\x64"
if defined SIGNTOOL_DIR set "PATH=%SIGNTOOL_DIR%;%PATH%"
where signtool >nul 2>&1 || (
  echo signtool not found under "%ProgramFiles(x86)%\Windows Kits\10\bin", install the Windows SDK
  exit /b 1
)

set /a signed=0
set /a skipped=0
rem debug\ and tools\ never ship with Swiftray
for /r "%ROOT%" %%f in (*.dll) do (
  echo %%f | findstr /i /c:"\debug\" /c:"\tools\" >nul || call :sign "%%f" || exit /b 1
)
echo signed %signed% files, skipped %skipped% already-signed files
exit /b 0

:sign
signtool verify /pa /q %1 >nul 2>&1
if not errorlevel 1 (
  set /a skipped+=1
  exit /b 0
)
echo [sign] %~nx1
rem write to a temp file first: on a server error curl would otherwise overwrite the dll
curl.exe --fail --silent --show-error --retry 3 -X POST -H "Content-Type: application/octet-stream" -H "Credential: %SIGN_CREDENTIAL%" -H "File-Name: %~nx1" --data-binary "@%~1" -o "%~1.signed" "%SIGN_SERVER%" || goto :fail
signtool verify /pa /q "%~1.signed" >nul 2>&1 || (
  echo server returned an unsigned file for %~nx1
  goto :fail
)
move /y "%~1.signed" %1 >nul || goto :fail
set /a signed+=1
exit /b 0

:fail
del "%~1.signed" 2>nul
exit /b 1
