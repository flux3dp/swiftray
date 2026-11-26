#!/bin/sh
# Fetch the prebuilt ONNX Runtime for this Mac and the MobileSAM models used by
# src/segment. Idempotent; re-run after `git clean`. See cmake/onnxruntime.cmake.
set -e
cd "$(dirname "$0")/.."

# 1.19.2 is the newest release whose macOS dylibs load on macOS 13 (minos 11.0). 1.23.2 needs
# 13.3+ libc++ symbols and 1.30.0 needs macOS 14 (std::pmr), so dyld rejects them on older systems.
# Only the C API on CPU is used, so the older release is enough.
ORT_VER=1.19.2
# GitHub no longer serves the microsoft/onnxruntime release tarballs (404), so use the official
# NuGet package. It carries the same dylibs for both Mac architectures and the C/C++ headers.
if [ "$(uname -m)" = "arm64" ]; then
  ORT_PLATFORM=macos_arm64; ORT_RID=osx-arm64
else
  ORT_PLATFORM=macos; ORT_RID=osx-x64
fi
ORT_DIR=third_party/onnxruntime/$ORT_PLATFORM

# keyed on the versioned dylib so bumping ORT_VER replaces an older download
if [ ! -f "$ORT_DIR/lib/libonnxruntime.$ORT_VER.dylib" ]; then
  echo "Downloading ONNX Runtime $ORT_VER ($ORT_RID) from NuGet"
  TMP=$(mktemp -d) && trap 'rm -rf "$TMP"' EXIT
  curl -fL -o "$TMP/ort.nupkg" "https://api.nuget.org/v3-flatcontainer/microsoft.ml.onnxruntime/$ORT_VER/microsoft.ml.onnxruntime.$ORT_VER.nupkg"
  mkdir -p "$TMP/ort/include" "$TMP/ort/lib"
  unzip -q -j "$TMP/ort.nupkg" 'build/native/include/*' -d "$TMP/ort/include"
  # install name is @rpath/libonnxruntime.<ver>.dylib; the unversioned symlink is for qmake's -lonnxruntime
  unzip -p "$TMP/ort.nupkg" "runtimes/$ORT_RID/native/libonnxruntime.dylib" > "$TMP/ort/lib/libonnxruntime.$ORT_VER.dylib"
  ln -s "libonnxruntime.$ORT_VER.dylib" "$TMP/ort/lib/libonnxruntime.dylib"
  mkdir -p third_party/onnxruntime
  rm -rf "$ORT_DIR" && mv "$TMP/ort" "$ORT_DIR"
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
