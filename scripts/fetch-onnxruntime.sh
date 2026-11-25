#!/bin/sh
# Fetch the prebuilt ONNX Runtime for this Mac and the MobileSAM models used by
# src/segment. Idempotent; re-run after `git clean`. See cmake/onnxruntime.cmake.
set -e
cd "$(dirname "$0")/.."

# 1.19.2 is the newest release whose macOS dylibs load on macOS 13 (minos 11.0). 1.23.2 needs
# 13.3+ libc++ symbols and 1.30.0 needs macOS 14 (std::pmr), so dyld rejects them on older systems.
# Only the C API on CPU is used, so the older release is enough.
ORT_VER=1.19.2
if [ "$(uname -m)" = "arm64" ]; then
  ORT_PLATFORM=macos_arm64; ORT_PKG=onnxruntime-osx-arm64-$ORT_VER
else
  ORT_PLATFORM=macos; ORT_PKG=onnxruntime-osx-x86_64-$ORT_VER
fi
ORT_DIR=third_party/onnxruntime/$ORT_PLATFORM

# keyed on the versioned dylib so bumping ORT_VER replaces an older download
if [ ! -f "$ORT_DIR/lib/libonnxruntime.$ORT_VER.dylib" ]; then
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
