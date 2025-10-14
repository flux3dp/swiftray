#include "toolpath-utils.h"
#include <algorithm>
#include <QtMath>

using namespace std;
/**
 * @brief Remove the suffix and prefix zeros
 *
 * @param src_bit_array
 * @return std::tuple<vector<std::bitset<32>>, uint32_t, uint32_t>
 *         - the trimmed bit array
 *         - the bit idx of the trim start
 *         - the bit idx of the trim end
 */
tuple<vector<Bitset32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
      const vector<Bitset32>& src_bit_array, uint32_t padding_dot_cnt) {

  enum class SearchStage {
      kSearchForStartPos,
      kSearchForEndPos,
  };
  SearchStage search_stage = SearchStage::kSearchForStartPos;
  uint32_t trim_start_bit_idx = 0;
  uint32_t trim_end_bit_idx = 0;
  for (int i = 0; i < src_bit_array.size(); i++) {
    if (src_bit_array[i].none()) {
      continue;
    }

    if (search_stage == SearchStage::kSearchForStartPos) {
      // Search for the first black dot
      for (int j = 31; j >= 0; j--) { // search from most significant bit
        if (src_bit_array[i][j]) {
          trim_start_bit_idx = i*32 + (31-j);
          search_stage = SearchStage::kSearchForEndPos;
          break;
        }
      }
    }
    if (search_stage == SearchStage::kSearchForEndPos) {
      // Search for the last black dot
      for (int j = 0; j < 32; j++) { // search from least significant bit
        if (src_bit_array[i][j]) {
          trim_end_bit_idx = i*32 + (31-j);
          break;
        }
      }
    }

  } // end of search for loop

  // Calculate padding (with restriction of path boundary)
  trim_start_bit_idx = trim_start_bit_idx > padding_dot_cnt ? (trim_start_bit_idx - padding_dot_cnt) : 0;
  trim_end_bit_idx = qMin(trim_end_bit_idx + padding_dot_cnt,
                          uint32_t(src_bit_array.size() * 32u - 1)); // index max = size - 1

  // Trim zeros but also keep padding
  vector<std::bitset<32>> bit_array{
      src_bit_array.begin() + (trim_start_bit_idx / 32),
      src_bit_array.begin() + (trim_end_bit_idx / 32) + 1 // (+1 because the last iterator is not included)
  };
  // Align the trim first pos with the start of bit array
  int bit_shift = trim_start_bit_idx % 32;
  for (auto i = 0; i < bit_array.size(); i++) {
    if (i+1 == bit_array.size()) {
      bit_array[i] = (bit_array[i] << bit_shift);
    } else {
      bit_array[i] = (bit_array[i] << bit_shift) | (bit_array[i+1] >> (32-bit_shift)) ;
    }
  }
  // Discard all trailing zero bits (not needed, reduce data transmission)
  while (bit_array.back().none()) {
    bit_array.pop_back();
  }

  return std::make_tuple(bit_array, trim_start_bit_idx, trim_end_bit_idx);
}

std::tuple<vector<ByteArray32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
    const vector<ByteArray32>& src_grayscale_array, 
    uint32_t padding_dot_cnt) {

  enum class SearchStage {
      kSearchForStartPos,
      kSearchForEndPos,
  };
  SearchStage search_stage = SearchStage::kSearchForStartPos;
  uint32_t trim_start_idx = 0;
  uint32_t trim_end_idx = 0;
  const unsigned char white_threshold = 250;  // Adjust this value as needed

  for (int i = 0; i < src_grayscale_array.size(); i++) {
    bool non_white_found = false;

    if (search_stage == SearchStage::kSearchForStartPos) {
      // Search for the first non-white dot
      for (int j = 0; j < 32; j++) {
        if (src_grayscale_array[i][j] < white_threshold) {
          trim_start_idx = i * 32 + j;
          search_stage = SearchStage::kSearchForEndPos;
          non_white_found = true;
          break;
        }
      }
    }
    
    if (search_stage == SearchStage::kSearchForEndPos) {
      // Search for the last non-white dot
      for (int j = 31; j >= 0; j--) {
        if (src_grayscale_array[i][j] < white_threshold) {
          trim_end_idx = i * 32 + j;
          non_white_found = true;
          break;
        }
      }
    }

    if (!non_white_found && search_stage == SearchStage::kSearchForEndPos) {
      break;
    }
  }

  // Calculate padding (with restriction of path boundary)
  trim_start_idx = trim_start_idx > padding_dot_cnt ? (trim_start_idx - padding_dot_cnt) : 0;
  trim_end_idx = std::min(trim_end_idx + padding_dot_cnt,
                          uint32_t(src_grayscale_array.size() * 32u - 1));

  // Trim white space but also keep padding
  vector<ByteArray32> grayscale_array{
      src_grayscale_array.begin() + (trim_start_idx / 32),
      src_grayscale_array.begin() + (trim_end_idx / 32) + 1
  };

  // Align the trim first pos with the start of grayscale array
  int shift = trim_start_idx % 32;
  if (shift > 0) {
    for (size_t i = 0; i < grayscale_array.size() - 1; i++) {
      std::rotate(grayscale_array[i].begin(), grayscale_array[i].begin() + shift, grayscale_array[i].end());
      std::copy_n(grayscale_array[i + 1].begin(), shift, grayscale_array[i].end() - shift);
    }
    if (grayscale_array.size() > 1) {
      std::rotate(grayscale_array.back().begin(), grayscale_array.back().begin() + shift, grayscale_array.back().end());
    }
  }

  // Discard all trailing white values (not needed, reduce data transmission)
  while (!grayscale_array.empty() && 
         std::all_of(grayscale_array.back().begin(), grayscale_array.back().end(), 
                     [&](unsigned char v) { return v >= white_threshold; })) {
    grayscale_array.pop_back();
  }

  return std::make_tuple(grayscale_array, trim_start_idx, trim_end_idx);
}

/**
 * @param src must be Format_ARGB32 or Format_ARGB32_Premultiplied grayscaled image
 * @param threshold
 * @return binarized Format_Grayscale8 QImage, transparent pixels are set to WHITE_PIXEL
 */
QImage imageBinarize(QImage* src, int threshold) {
  Q_ASSERT_X(src->allGray(), "toolpath-utils", "Input image for imageBinarize() must be grayscaled");
  Q_ASSERT_X(src->format() == QImage::Format_ARGB32 ||
             src->format() == QImage::Format_ARGB32_Premultiplied,
             "toolpath-utils",
             "Input image for imageBinarize() must be Format_ARGB32");

  QImage result_img{src->width(), src->height(), QImage::Format_Grayscale8};
  for (int y = 0; y < src->height(); ++y) {
    const QRgb* data_ptr = (QRgb*)src->constScanLine(y);
    uchar* result_ptr = result_img.scanLine(y);
    for (int x = 0; x < src->width(); ++x) {
      int alpha = qAlpha(data_ptr[x]);
      if (alpha == 0) {
        result_ptr[x] = WHITE_PIXEL;
      } else {
        int gray = qGray(data_ptr[x]);
        result_ptr[x] = gray <= threshold ? BLACK_PIXEL : WHITE_PIXEL;
      }
    }
  }
  return result_img;
}

template<typename T>
QImage _transpose(QImage* img) {
  QImage result(img->height(), img->width(), img->format());
  for (int y = 0; y < img->height(); ++y) {
    const T* srcLine = reinterpret_cast<const T*>(img->constScanLine(y));
    for (int x = 0; x < img->width(); ++x) {
      T* dstLine = reinterpret_cast<T*>(result.scanLine(x));
      dstLine[y] = srcLine[x];
    }
  }

  return result;
}

QImage imageTranspose(QImage* img) {
  switch (img->format()) {
    case QImage::Format_ARGB32_Premultiplied:
    case QImage::Format_ARGB32:
    case QImage::Format_RGB32:
      return _transpose<QRgb>(img);
    case QImage::Format_Grayscale8:
      return _transpose<uchar>(img);
    default:
      Q_ASSERT_X(false, "toolpath-utils", "Input image for transpose() must be Format_ARGB32 or Format_Grayscale8");
      return QImage();
  }
}

bool findMinMaxPixel(QImage* img, int* min_pixel, int* max_pixel) {
  Q_ASSERT_X(img->allGray(), "toolpath-utils", "Input image for findMinMaxPixel() must be grayscaled");
  Q_ASSERT_X(img->format() == QImage::Format_ARGB32 ||
             img->format() == QImage::Format_ARGB32_Premultiplied,
             "toolpath-utils",
             "Input image for findMinMaxPixel() must be Format_ARGB32");

  int min_p = 256;
  int max_p = 0;
  for (int y = 0; y < img->height(); ++y) {
    const QRgb* data_ptr = (QRgb*)img->constScanLine(y);
    for (int x = 0; x < img->width(); ++x) {
      if (qAlpha(data_ptr[x]) != 0) {
        int gray = qGray(data_ptr[x]);
        if (gray < min_p) min_p = gray;
        if (gray > max_p) max_p = gray;
      }
    }
  }
  if (min_p == 256) return false;
  if (min_pixel) *min_pixel = min_p;
  if (max_pixel) *max_pixel = max_p;
  return true;
}

cv::Mat QPolygonToMat(const QPolygonF& poly) {
  cv::Mat mat(poly.size(), 1, CV_32FC2);
  for (int i = 0; i < poly.size(); ++i) {
    mat.at<cv::Point2f>(i, 0) = cv::Point2f(poly[i].x(), poly[i].y());
  }
  return mat;
}

QPolygon MatIToQPolygon(const cv::Mat& mat) {
  QPolygon poly;
  for (int i = 0; i < mat.rows; ++i) {
    cv::Point2i pt = mat.at<cv::Point2i>(i, 0);
    poly << QPoint(pt.x, pt.y);
  }
  return poly;
}

QPolygonF MatFToQPolygon(const cv::Mat& mat) {
  QPolygonF poly;
  for (int i = 0; i < mat.rows; ++i) {
    cv::Point2f pt = mat.at<cv::Point2f>(i, 0);
    poly << QPointF(pt.x, pt.y);
  }
  return poly;
}
