@echo off
rem Fetch the prebuilt ONNX Runtime (win-x64) and the MobileSAM models used by
rem src\segment. Idempotent. See cmake\onnxruntime.cmake.
setlocal
cd /d "%~dp0\.."
set ORT_VER=1.30.0
set ORT_DIR=third_party\onnxruntime\windows
set ORT_NUPKG=%TEMP%\microsoft.ml.onnxruntime.%ORT_VER%.nupkg

rem GitHub no longer serves the microsoft/onnxruntime release zips (404), so use the official
rem NuGet package and lay it out like the old zip: include\ and lib\ (onnxruntime.dll/.lib).
rem lib\ is moved into place before include\, so the header check below implies a complete download.
if not exist "%ORT_DIR%\include\onnxruntime_cxx_api.h" (
  echo Downloading ONNX Runtime %ORT_VER% win-x64 from NuGet
  if exist "%ORT_DIR%" rmdir /s /q "%ORT_DIR%"
  mkdir "%ORT_DIR%" || exit /b 1
  curl.exe -fL -o "%ORT_NUPKG%" "https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/%ORT_VER%/microsoft.ml.onnxruntime.%ORT_VER%.nupkg" || exit /b 1
  tar -xf "%ORT_NUPKG%" -C "%ORT_DIR%" build/native/include runtimes/win-x64/native || exit /b 1
  del "%ORT_NUPKG%"
  move "%ORT_DIR%\runtimes\win-x64\native" "%ORT_DIR%\lib" >nul || exit /b 1
  move "%ORT_DIR%\build\native\include" "%ORT_DIR%\include" >nul || exit /b 1
  rmdir /s /q "%ORT_DIR%\build" "%ORT_DIR%\runtimes"
)

set MODELS_URL=https://huggingface.co/nrl-ai/samexporter-onnx-models/resolve/main/mobile_sam
if not exist resources\models mkdir resources\models
for %%m in (mobile_sam.encoder.onnx mobile_sam.decoder.onnx) do (
  if not exist "resources\models\%%m" (
    if exist "..\mini-sam\models\%%m" (
      copy "..\mini-sam\models\%%m" resources\models\ >nul
    ) else (
      echo Downloading %%m
      curl.exe -fL -o "resources\models\%%m.part" "%MODELS_URL%/%%m" || exit /b 1
      move /y "resources\models\%%m.part" "resources\models\%%m" >nul || exit /b 1
    )
  )
)
echo OK: %ORT_DIR%, resources\models
