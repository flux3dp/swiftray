// ort_loader.h - binds the ONNX Runtime C API at first use. On macOS the dylib is dlopen'ed
// instead of linked, so one this OS can't load (e.g. built for a newer macOS: dyld "Symbol not
// found" in libc++) only disables /segment instead of keeping Swiftray from launching.
#pragma once
#define ORT_API_MANUAL_INIT
#include <onnxruntime_cxx_api.h>
#undef ORT_API_MANUAL_INIT

#include <string>
#ifdef __APPLE__
#include <dlfcn.h>
#endif

namespace segment {

// Empty on success, else why ONNX Runtime is unavailable. Must succeed before any Ort:: object is made.
inline const std::string& initOrt() {
  static const std::string error = []() -> std::string {
#ifdef __APPLE__
    // ORT_DYLIB is the bundled versioned dylib name (cmake/Qt6/unix/CMakeLists.txt), found via
    // the app's @executable_path/../Frameworks rpath; tests may define it as a full path.
    void* lib = dlopen(ORT_DYLIB, RTLD_NOW | RTLD_LOCAL);
    auto get_base = lib ? reinterpret_cast<const OrtApiBase* (*)()>(dlsym(lib, "OrtGetApiBase")) : nullptr;
    if (!get_base) {
      const char* e = dlerror();
      return e ? e : "cannot load ONNX Runtime";
    }
    const OrtApi* api = get_base()->GetApi(ORT_API_VERSION);
#else
    const OrtApi* api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
#endif
    if (!api) return "ONNX Runtime does not provide API version " + std::to_string(ORT_API_VERSION);
    Ort::InitApi(api);
    return {};
  }();
  return error;
}

}  // namespace segment
