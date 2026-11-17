@echo off
setlocal
rem Usage: sign_release.bat swiftray-<version>.zip
rem Run on the code sign machine. Unzips the release, signs every .exe/.dll that does not
rem already carry a valid Authenticode signature (keeps Microsoft's vcruntime signatures),
rem and writes swiftray-<version>-signed.zip next to the input. Upload that to S3.
rem Windows 11 Smart App Control blocks unsigned dlls with "Bad Image" 0xc0e90002.

if "%~1"=="" (
  echo usage: %~nx0 swiftray-^<version^>.zip
  exit /b 1
)

rem same setup as sign.bat on the code sign machine; "token password.txt" sits next to this script
set PATH=C:\Program Files (x86)\Windows Kits\10\bin\x64;%PATH%
if not exist "%~dp0token password.txt" (
  echo missing "%~dp0token password.txt"
  exit /b 1
)
set /p Password=<"%~dp0token password.txt"

set WORK=%~dpn1-signed
set OUT=%~dpn1-signed.zip
rmdir /s /q "%WORK%" 2>nul
mkdir "%WORK%"
tar -xf "%~1" -C "%WORK%" --exclude __MACOSX || exit /b 1

set /a signed=0
set /a skipped=0
for /r "%WORK%" %%f in (*.exe *.dll) do call :sign "%%f" || exit /b 1
echo signed %signed% files, skipped %skipped% already-signed files

del "%OUT%" 2>nul
tar -a -cf "%OUT%" -C "%WORK%" . || exit /b 1
rmdir /s /q "%WORK%"
echo wrote %OUT%
exit /b 0

:sign
signtool verify /pa /q %1 >nul 2>&1
if not errorlevel 1 (
  set /a skipped+=1
  exit /b 0
)
echo [sign] %~nx1
C:\ScSignTool.exe -pin %Password% "sign" "/a" "/t" "http://timestamp.digicert.com" "/fd" "SHA256" "/d" "Swiftray" "/du" "https://github.com/flux3dp/swiftray" %1 || exit /b 1
set /a signed+=1
exit /b 0
