# Prebuilt ONNX Runtime (CPU) for the /segment service (src/segment) and the
# MobileSAM models. Both are gitignored and fetched at configure time by
# scripts/fetch-onnxruntime.{sh,bat} when missing (one-off download per clone).
if(WIN32)
  set(ORT_PLATFORM "windows")
elseif(APPLE AND CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
  set(ORT_PLATFORM "macos_arm64")
else()
  set(ORT_PLATFORM "macos")
endif()

set(ORT_DIR "${CMAKE_SOURCE_DIR}/third_party/onnxruntime/${ORT_PLATFORM}")
set(ORT_INCLUDE_DIR "${ORT_DIR}/include")
set(SEGMENT_MODELS_DIR "${CMAKE_SOURCE_DIR}/resources/models")

if(NOT EXISTS "${ORT_INCLUDE_DIR}/onnxruntime_cxx_api.h" OR NOT EXISTS "${SEGMENT_MODELS_DIR}/mobile_sam.decoder.onnx")
  message(STATUS "Fetching ONNX Runtime and segmentation models (first configure only)")
  if(WIN32)
    set(_fetch_cmd cmd /c "${CMAKE_SOURCE_DIR}/scripts/fetch-onnxruntime.bat")
  else()
    set(_fetch_cmd sh "${CMAKE_SOURCE_DIR}/scripts/fetch-onnxruntime.sh")
  endif()
  execute_process(COMMAND ${_fetch_cmd} WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" RESULT_VARIABLE _fetch_result)
  if(NOT _fetch_result EQUAL 0)
    message(FATAL_ERROR "scripts/fetch-onnxruntime failed (exit ${_fetch_result}); check network access or run it by hand.")
  endif()
endif()
if(NOT EXISTS "${ORT_INCLUDE_DIR}/onnxruntime_cxx_api.h")
  message(FATAL_ERROR "ONNX Runtime still not found at ${ORT_DIR} after fetching.")
endif()

if(WIN32)
  set(ORT_LIBRARY "${ORT_DIR}/lib/onnxruntime.lib")
  set(ORT_RUNTIME_FILES "${ORT_DIR}/lib/onnxruntime.dll")
else()
  set(ORT_LIBRARY "${ORT_DIR}/lib/libonnxruntime.dylib")
  # the versioned dylib only (the unversioned name is a symlink); install name is @rpath/<versioned>
  file(GLOB ORT_RUNTIME_FILES "${ORT_DIR}/lib/libonnxruntime.*.dylib")
endif()

file(GLOB SEGMENT_MODEL_FILES "${SEGMENT_MODELS_DIR}/*.onnx")
if(NOT SEGMENT_MODEL_FILES)
  message(FATAL_ERROR "MobileSAM models still not found in ${SEGMENT_MODELS_DIR} after fetching.")
endif()

message(STATUS "ONNX Runtime: ${ORT_DIR}")
message(STATUS "Segmentation models: ${SEGMENT_MODEL_FILES}")
