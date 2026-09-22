@echo off
rem Fetch the prebuilt ONNX Runtime (win-x64) and the MobileSAM models used by
rem src\segment. Idempotent. See cmake\onnxruntime.cmake.
setlocal
cd /d "%~dp0\.."
set ORT_VER=1.30.0
set ORT_PKG=onnxruntime-win-x64-%ORT_VER%
set ORT_DIR=third_party\onnxruntime\windows

if not exist "%ORT_DIR%\include\onnxruntime_cxx_api.h" (
  echo Downloading %ORT_PKG%
  if not exist third_party\onnxruntime mkdir third_party\onnxruntime
  curl.exe -fL -o "%TEMP%\%ORT_PKG%.zip" "https://github.com/microsoft/onnxruntime/releases/download/v%ORT_VER%/%ORT_PKG%.zip" || exit /b 1
  tar -xf "%TEMP%\%ORT_PKG%.zip" -C third_party\onnxruntime || exit /b 1
  if exist "%ORT_DIR%" rmdir /s /q "%ORT_DIR%"
  move "third_party\onnxruntime\%ORT_PKG%" "%ORT_DIR%" >nul
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
