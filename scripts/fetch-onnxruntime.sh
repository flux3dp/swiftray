#!/bin/sh
# Fetch the prebuilt ONNX Runtime for this Mac and the MobileSAM models used by
# src/segment. Idempotent; re-run after `git clean`. See cmake/onnxruntime.cmake.
set -e
cd "$(dirname "$0")/.."

if [ "$(uname -m)" = "arm64" ]; then
  ORT_VER=1.30.0; ORT_PLATFORM=macos_arm64; ORT_PKG=onnxruntime-osx-arm64-$ORT_VER
else
  # Microsoft stopped shipping macOS x86_64 binaries after 1.23.2; only the C API is used, any release works.
  ORT_VER=1.23.2; ORT_PLATFORM=macos; ORT_PKG=onnxruntime-osx-x86_64-$ORT_VER
fi
ORT_DIR=third_party/onnxruntime/$ORT_PLATFORM

if [ ! -f "$ORT_DIR/include/onnxruntime_cxx_api.h" ]; then
  echo "Downloading $ORT_PKG"
  mkdir -p third_party/onnxruntime
  curl -fL -o /tmp/$ORT_PKG.tgz "https://github.com/microsoft/onnxruntime/releases/download/v$ORT_VER/$ORT_PKG.tgz"
  tar -xzf /tmp/$ORT_PKG.tgz -C third_party/onnxruntime
  rm -rf "$ORT_DIR" && mv "third_party/onnxruntime/$ORT_PKG" "$ORT_DIR"
  xattr -dr com.apple.quarantine "$ORT_DIR" 2>/dev/null || true
fi

MODELS_URL=https://huggingface.co/nrl-ai/samexporter-onnx-models/resolve/main/mobile_sam
mkdir -p resources/models
for m in mobile_sam.encoder.onnx mobile_sam.decoder.onnx; do
  if [ ! -f "resources/models/$m" ]; then
    if [ -f "../mini-sam/models/$m" ]; then
      cp "../mini-sam/models/$m" resources/models/
    else
      echo "Downloading $m"
      curl -fL -o "resources/models/$m.part" "$MODELS_URL/$m"
      mv "resources/models/$m.part" "resources/models/$m"
    fi
  fi
done
echo "OK: $ORT_DIR, resources/models"
