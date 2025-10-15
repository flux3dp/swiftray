#include "toolpath-utils.h"
#include <algorithm>
#include <QtMath>
#include <cmath>

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

bool polygonContainsPolygon(const QPolygonF& outer, const QPolygonF& inner) {
  for (const QPointF& p : inner) {
    if (!outer.containsPoint(p, Qt::WindingFill)) return false;
  }
  return true;
}

void sortByBoundingRect(QList<QPolygonF>& polys) {
  std::sort(polys.begin(), polys.end(),
    [](const QPolygonF& a, const QPolygonF& b) {
      QRectF bbox_a = a.boundingRect();
      QRectF bbox_b = b.boundingRect();
      return bbox_a.width() * bbox_a.height() > bbox_b.width() * bbox_b.height();
    });
}

void traverse(QList<QPolygonF>& polys, const NestedPolygonF& node) {
  for (const auto& child : node.children) {
    traverse(polys, child);
  }
  if (!node.polygon.isEmpty()) {
    polys.push_back(node.polygon);
  }
}

// ================ PathUtils =================
double PathUtils::getLength(QPointF& start, QPointF& end) {
  double dx = (end.x() - start.x()) * length_ratio_x_;
  double dy = (end.y() - start.y()) * length_ratio_y_;
  return std::sqrt(dx * dx + dy * dy);
}

void PathUtils::loopCompensate(QPolygonF& poly) {
  if (!should_compensate_ || !poly.isClosed()) return;

  QPointF curr, next;
  double lineLength;
  double distance = loop_compensation_;
  int ptsSize = poly.size();
  for (int i = 1; i < ptsSize && distance >= 0.0001; ++i) {
    curr = poly[i - 1];
    next = poly[i];
    lineLength = getLength(curr, next);
    if (lineLength > distance) {
      poly << QLineF(curr, next).pointAt(distance / lineLength);
      break;
    } else {
      poly << next;
      distance -= lineLength;
    }
  }
}

int PathUtils::clipWorkarea(QPointF* start, QPointF* end) {
  int clip_result = 0;
  QLineF scanLine(*start, *end);
  bool ok = false;
  double x1 = start->x();
  double y1 = start->y();
  bool is_p1_outside = (x1 < left_x_ || x1 > right_x_ || y1 < top_y_ || y1 > bottom_y_);
  if (is_p1_outside) {
    clip_result |= CLIP_FLAG_START;
    if (x1 < left_x_) {
      ok = scanLine.intersects(left_border_, start) == QLineF::BoundedIntersection;
    } else if (x1 > right_x_) {
      ok = scanLine.intersects(right_border_, start) == QLineF::BoundedIntersection;
    }
    if (!ok) {
      if (y1 < top_y_) {
        ok = scanLine.intersects(top_border_, start) == QLineF::BoundedIntersection;
      } else {
        ok = scanLine.intersects(bottom_border_, start) == QLineF::BoundedIntersection;
      }
      if (!ok) {
        start->setX(x1);
        start->setY(y1);
        return -1;
      }
    }
  }

  ok = false;
  double x2 = end->x();
  double y2 = end->y();
  bool is_p2_outside = (x2 < left_x_ || x2 > right_x_ || y2 < top_y_ || y2 > bottom_y_);
  if (is_p2_outside) {
    clip_result |= CLIP_FLAG_END;
    if (x2 < left_x_) {
      ok = scanLine.intersects(left_border_, end) == QLineF::BoundedIntersection;
    } else if (x2 > right_x_) {
      ok = scanLine.intersects(right_border_, end) == QLineF::BoundedIntersection;
    }
    if (!ok) {
      if (y2 < top_y_) {
        ok = scanLine.intersects(top_border_, end) == QLineF::BoundedIntersection;
      } else {
        ok = scanLine.intersects(bottom_border_, end) == QLineF::BoundedIntersection;
      }
      if (!ok) {
        end->setX(x2);
        end->setY(y2);
        return -1;
      }
    }
  }
  return clip_result;
}

void PathUtils::preprocessPath(QList<NestedPolygonF>& polys) {
  int original_size = polys.size();
  for (int p = 0; p < original_size; ++p) {
    QPolygonF poly = polys[p].polygon;
    if (poly.isEmpty()) continue;

    QPolygonF clipped_poly;
    bool is_first = true;
    for (int i = 1; i < poly.size(); ++i) {
      QPointF curr = poly[i - 1];
      QPointF next = poly[i];
      int clip_result = clipWorkarea(&curr, &next);
      if (clip_result == -1) continue;
      if (clipped_poly.isEmpty()) {
        clipped_poly << curr;
      }
      clipped_poly << next;
      if ((clip_result & CLIP_FLAG_END) || (i == poly.size() - 1)) {
        if (should_compensate_) {
          loopCompensate(clipped_poly);
        }
        if (is_first) {
          polys[p].polygon = std::move(clipped_poly);
          is_first = false;
        } else {
          NestedPolygonF clipped_item = {std::move(clipped_poly), {}};
          polys.push_back(std::move(clipped_item));
        }
      }
    }
    if (is_first) {
      // All points are outside workarea
      polys.removeAt(p);
      --p;
      --original_size;
    }
  }
}

void PathUtils::sortByDistance(QList<NestedPolygonF>& polys) {
  QList<NestedPolygonF> sorted_polys;
  if (polys.empty()) return;

  preprocessPath(polys);

  auto currentIter = polys.begin();
  bool needReverse = false;
  auto minIter = polys.end();
  float minDist;
  float d;
  QPointF currentEnd;
  while (currentIter < polys.end()) {
    if (needReverse) {
      std::reverse(currentIter->polygon.begin(), currentIter->polygon.end());
    }
    sorted_polys.push_back(std::move(*currentIter));
    currentEnd = sorted_polys.back().polygon.last();
    polys.erase(currentIter);
    minDist = NAN;
    minIter = polys.begin();

    for (auto it = polys.begin(); it != polys.end(); ++it) {
      d = getLength(currentEnd, it->polygon.first());
      if (std::isnan(minDist) || d < minDist) {
        minDist = d;
        minIter = it;
        needReverse = false;
      }

      d = getLength(currentEnd, it->polygon.last());
      if (d < minDist) {
        minDist = d;
        minIter = it;
        needReverse = true;
      }
    }
    currentIter = minIter;
  }

  polys = std::move(sorted_polys);
}

void PathUtils::findChildren(QList<QPolygonF>& polys, NestedPolygonF& parent, int index) {
  if (polys.empty()) return;

  for (; index < polys.size();) {
    if (polygonContainsPolygon(parent.polygon, polys[index])) {
      NestedPolygonF child;
      child.polygon = polys[index];
      parent.children.push_back(std::move(child));
      polys.removeAt(index);
      findChildren(polys, parent.children.back(), index);
    } else {
      ++index;
    }
  }
  sortByDistance(parent.children);
}

/**
 * Sort polygons by nested containment relationship
 * Clip polygons to the working area
 * Add loop compensation if needed
 * Optimize travel distance in same containment level
 */
void PathUtils::sortAndPreprocessPolygons(QList<QPolygonF>& polys) {
  NestedPolygonF root;
  // Sort by bounding rect for some containment relationship hints
  sortByBoundingRect(polys);

  while (!polys.isEmpty()) {
    NestedPolygonF front;
    front.polygon = polys.front();
    polys.pop_front();
    findChildren(polys, front, 0);
    root.children.push_back(std::move(front));
  }

  sortByDistance(root.children);
  polys.clear();
  traverse(polys, root);
}
