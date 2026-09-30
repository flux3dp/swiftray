// sam.h - MobileSAM inference via ONNX Runtime (CPU). Ported from mini-sam/src/sam.h
// (samexporter model contract). Only the C++ API header of ORT is used.
#pragma once
#include <onnxruntime_cxx_api.h>

#include <QDebug>

#include <array>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "sam_image.h"

namespace segment {

struct LowResMask {
  std::vector<float> logits;  // 256*256, best-of-4 candidates
  float iou = 0.f;
};

struct SamConfig {
  std::string ep = "cpu";   // cpu | coreml (macOS) | dml (Windows, DirectML build)
  bool letterbox = false;   // pad encoder input to a constant 1024x1024 (static shape - some CoreML configs)
  int decoder_workers = 0;  // >1: decoder session runs single-threaded per call so parallel Run() calls scale
};

class Sam {
 public:
  Sam(const std::string& encoder_path, const std::string& decoder_path, const SamConfig& cfg = {})
      : env_(ORT_LOGGING_LEVEL_ERROR, "swiftray-segment"), letterbox_(cfg.letterbox) {
    Ort::SessionOptions so_enc;
    so_enc.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    if (cfg.ep == "coreml") {
      // Encoder on Core ML (Apple GPU + Neural Engine); decoder stays on CPU.
      try {
        std::unordered_map<std::string, std::string> opts{{"ModelFormat", "MLProgram"}, {"MLComputeUnits", "ALL"}};
        so_enc.AppendExecutionProvider("CoreML", opts);
        qInfo() << "segment: encoder EP CoreML";
      } catch (const Ort::Exception& e) {
        qWarning() << "segment: CoreML unavailable, encoder on CPU:" << e.what();
      }
    } else if (cfg.ep == "dml") {
      try {
        so_enc.AppendExecutionProvider("DML", {});
        qInfo() << "segment: encoder EP DirectML";
      } catch (const Ort::Exception& e) {
        qWarning() << "segment: DirectML unavailable, encoder on CPU:" << e.what();
      }
    }
    Ort::SessionOptions so_dec;
    so_dec.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
    if (cfg.decoder_workers > 1) {
      so_dec.SetIntraOpNumThreads(1);
      so_dec.DisableCpuMemArena();
      so_dec.DisableMemPattern();
    }
    // u8path: the paths are UTF-8 from QString; path(std::string) would decode them as ANSI on Windows.
    // path::c_str() is wchar_t* on Windows and char* elsewhere - exactly ORTCHAR_T
    enc_ = std::make_unique<Ort::Session>(env_, std::filesystem::u8path(encoder_path).c_str(), so_enc);
    dec_ = std::make_unique<Ort::Session>(env_, std::filesystem::u8path(decoder_path).c_str(), so_dec);

    Ort::AllocatorWithDefaultOptions alloc;
    enc_input_name_ = enc_->GetInputNameAllocated(0, alloc).get();
    enc_rank_ = enc_->GetInputTypeInfo(0).GetTensorTypeAndShapeInfo().GetShape().size();
  }

  // Compute the image embedding. Frame is RGB8 at native resolution.
  void encode(const Image& frame) {
    frame_w_ = frame.w;
    frame_h_ = frame.h;
    scale_ = 1024.0f / (std::max)(frame.w, frame.h);
    nw_ = (int)(frame.w * scale_ + 0.5f);
    nh_ = (int)(frame.h * scale_ + 0.5f);
    Image resized = resize_image(frame, nw_, nh_);

    Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    std::vector<Ort::Value> inputs;
    std::vector<float> data;
    if (enc_rank_ == 3) {  // exported with --use-preprocess: raw HWC RGB float
      const int cw = letterbox_ ? 1024 : nw_, ch = letterbox_ ? 1024 : nh_;
      data.assign((size_t)ch * cw * 3, 0.f);
      for (int y = 0; y < nh_; y++)
        for (int x = 0; x < nw_; x++) {
          const uint8_t* p = resized.at(x, y);
          float* d = &data[((size_t)y * cw + x) * 3];
          d[0] = p[0];
          d[1] = p[1];
          d[2] = p[2];
        }
      std::array<int64_t, 3> shape{ch, cw, 3};
      inputs.push_back(Ort::Value::CreateTensor<float>(mem, data.data(), data.size(), shape.data(), shape.size()));
    } else {  // normalized, padded NCHW 1x3x1024x1024
      const float mean[3] = {123.675f, 116.28f, 103.53f};
      const float stdv[3] = {58.395f, 57.12f, 57.375f};
      data.assign((size_t)3 * 1024 * 1024, 0.f);
      for (int y = 0; y < nh_; y++)
        for (int x = 0; x < nw_; x++) {
          const uint8_t* p = resized.at(x, y);
          const float rgb[3] = {(float)p[0], (float)p[1], (float)p[2]};
          for (int c = 0; c < 3; c++)
            data[(size_t)c * 1024 * 1024 + (size_t)y * 1024 + x] = (rgb[c] - mean[c]) / stdv[c];
        }
      std::array<int64_t, 4> shape{1, 3, 1024, 1024};
      inputs.push_back(Ort::Value::CreateTensor<float>(mem, data.data(), data.size(), shape.data(), shape.size()));
    }
    const char* in_names[] = {enc_input_name_.c_str()};
    const char* out_names[] = {"image_embeddings"};
    auto out = enc_->Run(Ort::RunOptions{nullptr}, in_names, inputs.data(), 1, out_names, 1);
    auto info = out[0].GetTensorTypeAndShapeInfo();
    emb_shape_ = info.GetShape();
    const float* p = out[0].GetTensorData<float>();
    embedding_.assign(p, p + info.GetElementCount());
  }

  // Decode a prompt of N points (frame coords; label 1=include, 0=exclude) -> best low-res mask.
  LowResMask decode_prompt(const std::vector<std::pair<float, float>>& points, const std::vector<float>& labels_in) {
    auto all = decode_prompt_all(points, labels_in);
    size_t best = 0;
    for (size_t i = 1; i < all.size(); i++)
      if (all[i].iou > all[best].iou) best = i;
    return all[best];
  }

  // All granularity levels for one prompt (index 0 = whole-object token, 1..3 = multimask);
  // nested parts (an engraving on a tile) live in the lower-iou outputs.
  std::vector<LowResMask> decode_prompt_all(const std::vector<std::pair<float, float>>& points, const std::vector<float>& labels_in) {
    Ort::MemoryInfo mem = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    const int64_t n = (int64_t)points.size() + 1;  // + padding point
    std::vector<float> coords((size_t)n * 2, 0.f);
    std::vector<float> labels((size_t)n, -1.f);
    for (size_t i = 0; i < points.size(); i++) {
      coords[i * 2] = points[i].first * scale_;
      coords[i * 2 + 1] = points[i].second * scale_;
      labels[i] = i < labels_in.size() ? labels_in[i] : 1.f;
    }
    std::vector<float> mask_in((size_t)256 * 256, 0.f);
    float has_mask = 0.f;
    float orig_size[2] = {(float)frame_h_, (float)frame_w_};

    std::array<int64_t, 3> coords_shape{1, n, 2};
    std::array<int64_t, 2> labels_shape{1, n};
    std::array<int64_t, 4> mask_shape{1, 1, 256, 256};
    std::array<int64_t, 1> has_shape{1};
    std::array<int64_t, 1> size_shape{2};

    std::vector<Ort::Value> inputs;
    inputs.push_back(Ort::Value::CreateTensor<float>(mem, embedding_.data(), embedding_.size(), emb_shape_.data(), emb_shape_.size()));
    inputs.push_back(Ort::Value::CreateTensor<float>(mem, coords.data(), coords.size(), coords_shape.data(), 3));
    inputs.push_back(Ort::Value::CreateTensor<float>(mem, labels.data(), labels.size(), labels_shape.data(), 2));
    inputs.push_back(Ort::Value::CreateTensor<float>(mem, mask_in.data(), mask_in.size(), mask_shape.data(), 4));
    inputs.push_back(Ort::Value::CreateTensor<float>(mem, &has_mask, 1, has_shape.data(), 1));
    inputs.push_back(Ort::Value::CreateTensor<float>(mem, orig_size, 2, size_shape.data(), 1));

    const char* in_names[] = {"image_embeddings", "point_coords", "point_labels", "mask_input", "has_mask_input", "orig_im_size"};
    const char* out_names[] = {"low_res_masks", "iou_predictions"};
    auto out = dec_->Run(Ort::RunOptions{nullptr}, in_names, inputs.data(), inputs.size(), out_names, 2);
    const float* masks = out[0].GetTensorData<float>();  // [1,4,256,256]
    const float* ious = out[1].GetTensorData<float>();   // [1,4]
    int n_cand = (int)out[1].GetTensorTypeAndShapeInfo().GetElementCount();
    std::vector<LowResMask> all(n_cand);
    for (int i = 0; i < n_cand; i++) {
      all[i].iou = ious[i];
      all[i].logits.assign(masks + (size_t)i * 256 * 256, masks + (size_t)(i + 1) * 256 * 256);
    }
    return all;
  }

  LowResMask decode_point(float px, float py) { return decode_prompt({{px, py}}, {1.f}); }
  std::vector<LowResMask> decode_point_all(float px, float py) { return decode_prompt_all({{px, py}}, {1.f}); }

  bool has_embedding() const { return !embedding_.empty(); }

  // Low-res logits (padded-square space) -> full-frame binary mask.
  std::vector<uint8_t> lowres_to_full(const std::vector<float>& logits) const {
    std::vector<float> up = resize_float(logits, 256, 256, 1024, 1024);
    std::vector<float> crop((size_t)nh_ * nw_);
    for (int y = 0; y < nh_; y++) memcpy(&crop[(size_t)y * nw_], &up[(size_t)y * 1024], nw_ * sizeof(float));
    std::vector<float> full = resize_float(crop, nw_, nh_, frame_w_, frame_h_);
    std::vector<uint8_t> mask(full.size());
    for (size_t i = 0; i < full.size(); i++) mask[i] = full[i] > 0.f ? 1 : 0;
    return mask;
  }

  int frame_w() const { return frame_w_; }
  int frame_h() const { return frame_h_; }
  float scale() const { return scale_; }
  int content_w256() const { return (std::max)(2, (int)(nw_ / 4.0f + 0.5f)); }
  int content_h256() const { return (std::max)(2, (int)(nh_ / 4.0f + 0.5f)); }

 private:
  Ort::Env env_;
  std::unique_ptr<Ort::Session> enc_, dec_;
  std::string enc_input_name_;
  size_t enc_rank_ = 3;
  std::vector<float> embedding_;
  std::vector<int64_t> emb_shape_;
  int frame_w_ = 0, frame_h_ = 0, nw_ = 0, nh_ = 0;
  float scale_ = 1.f;
  bool letterbox_ = false;
};

}  // namespace segment
