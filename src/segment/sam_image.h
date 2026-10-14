// sam_image.h - RGB8 image container + bilinear resizes used by the MobileSAM
// pipeline. Ported from mini-sam/src/img.h (stb decode replaced by QImage).
// Keep the resizes hand-rolled: the golden parity (test/compare.py in mini-sam)
// was validated against exactly these kernels.
#pragma once
#include <QImage>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace segment {

struct Image {  // 8-bit RGB interleaved
  int w = 0, h = 0;
  std::vector<uint8_t> px;  // w*h*3
  uint8_t* at(int x, int y) { return &px[(size_t)(y * w + x) * 3]; }
  const uint8_t* at(int x, int y) const { return &px[(size_t)(y * w + x) * 3]; }
  bool empty() const { return w == 0 || h == 0; }
};

inline Image imageFromQImage(const QImage& src) {
  QImage rgb = src.convertToFormat(QImage::Format_RGB888);
  Image img;
  img.w = rgb.width();
  img.h = rgb.height();
  img.px.resize((size_t)img.w * img.h * 3);
  for (int y = 0; y < img.h; y++)
    memcpy(&img.px[(size_t)y * img.w * 3], rgb.constScanLine(y), (size_t)img.w * 3);
  return img;
}

// bilinear resize, u8 3-channel
inline Image resize_image(const Image& src, int nw, int nh) {
  Image dst;
  dst.w = nw;
  dst.h = nh;
  dst.px.resize((size_t)nw * nh * 3);
  const float sx = (float)src.w / nw, sy = (float)src.h / nh;
  for (int y = 0; y < nh; y++) {
    float fy = (y + 0.5f) * sy - 0.5f;
    int y0 = (int)std::floor(fy);
    float wy = fy - y0;
    int y1 = (std::min)(y0 + 1, src.h - 1);
    y0 = (std::max)(y0, 0);
    for (int x = 0; x < nw; x++) {
      float fx = (x + 0.5f) * sx - 0.5f;
      int x0 = (int)std::floor(fx);
      float wx = fx - x0;
      int x1 = (std::min)(x0 + 1, src.w - 1);
      x0 = (std::max)(x0, 0);
      const uint8_t* p00 = src.at(x0, y0);
      const uint8_t* p10 = src.at(x1, y0);
      const uint8_t* p01 = src.at(x0, y1);
      const uint8_t* p11 = src.at(x1, y1);
      uint8_t* d = dst.at(x, y);
      for (int c = 0; c < 3; c++) {
        float v = (1 - wy) * ((1 - wx) * p00[c] + wx * p10[c]) + wy * ((1 - wx) * p01[c] + wx * p11[c]);
        d[c] = (uint8_t)(v + 0.5f);
      }
    }
  }
  return dst;
}

// bilinear resize, single-channel float
inline std::vector<float> resize_float(const std::vector<float>& src, int sw, int sh, int nw, int nh) {
  std::vector<float> dst((size_t)nw * nh);
  const float sx = (float)sw / nw, sy = (float)sh / nh;
  for (int y = 0; y < nh; y++) {
    float fy = (y + 0.5f) * sy - 0.5f;
    int y0 = (int)std::floor(fy);
    float wy = fy - y0;
    int y1 = (std::min)(y0 + 1, sh - 1);
    y0 = (std::max)(y0, 0);
    for (int x = 0; x < nw; x++) {
      float fx = (x + 0.5f) * sx - 0.5f;
      int x0 = (int)std::floor(fx);
      float wx = fx - x0;
      int x1 = (std::min)(x0 + 1, sw - 1);
      x0 = (std::max)(x0, 0);
      dst[(size_t)y * nw + x] =
          (1 - wy) * ((1 - wx) * src[(size_t)y0 * sw + x0] + wx * src[(size_t)y0 * sw + x1]) +
          wy * ((1 - wx) * src[(size_t)y1 * sw + x0] + wx * src[(size_t)y1 * sw + x1]);
    }
  }
  return dst;
}

}  // namespace segment
