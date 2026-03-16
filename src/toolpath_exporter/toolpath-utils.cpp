#include "toolpath-utils.h"
#include "toolpath-exporter-constants.h"
#include <QDebug>
#include <opencv2/imgproc.hpp>

/**
 * @brief Remove the suffix and prefix zeros
 *
 * @param src_bit_array
 * @return std::tuple<vector<std::bitset<32>>, uint32_t, uint32_t>
 *         - the trimmed bit array
 *         - the bit idx of the trim start
 *         - the bit idx of the trim end
 */
std::tuple<std::vector<Bitset32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
    const std::vector<Bitset32>& src_bit_array, uint32_t padding_dot_cnt) {
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
  std::vector<std::bitset<32>> bit_array{
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

std::tuple<std::vector<ByteArray32>, uint32_t, uint32_t> adjustPrefixSuffixZero(
    const std::vector<ByteArray32>& src_grayscale_array,
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
  std::vector<ByteArray32> grayscale_array{
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
         std::all_of(grayscale_array.back().begin(),
                     grayscale_array.back().end(),
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

/**
 * binarized Format_ARGB32 QImage in-place
 * @param src must be Format_ARGB32 or Format_ARGB32_Premultiplied grayscaled
 * image
 * @param threshold
 */
void imageBinarizeARGB32(QImage* src, int threshold) {
  Q_ASSERT_X(src->allGray(), "toolpath-utils", "Input image for imageBinarizeARGB32() must be grayscaled");
  Q_ASSERT_X(src->format() == QImage::Format_ARGB32 ||
             src->format() == QImage::Format_ARGB32_Premultiplied,
             "toolpath-utils",
             "Input image for imageBinarizeARGB32() must be Format_ARGB32");

  QRgb white = qRgba(0, 0, 0, 0);  // transparent black
  QRgb black = qRgba(BLACK_PIXEL, BLACK_PIXEL, BLACK_PIXEL, 255);
  for (int y = 0; y < src->height(); ++y) {
    QRgb* data_ptr = (QRgb*)src->scanLine(y);
    for (int x = 0; x < src->width(); ++x) {
      int alpha = qAlpha(data_ptr[x]);
      if (alpha == 0) {
        data_ptr[x] = white;
      } else {
        int gray = qGray(data_ptr[x]);
        data_ptr[x] = gray <= threshold ? black : white;
      }
    }
  }
}

template <typename T>
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

cv::Mat QImageToMat(const QImage& img) {
  Q_ASSERT(img.format() == QImage::Format_Grayscale8);
  return cv::Mat(img.height(), img.width(), CV_8UC1,
                 const_cast<uchar*>(img.bits()), img.bytesPerLine());
}

QImage MatToQImage(const cv::Mat& mat) {
  Q_ASSERT(mat.type() == CV_8UC1);
  QImage img(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8);
  return img.copy();
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

void sortByBoundingRect(QVector<QPolygonF>& polys) {
  std::sort(polys.begin(), polys.end(),
    [](const QPolygonF& a, const QPolygonF& b) {
      QRectF bbox_a = a.boundingRect();
      QRectF bbox_b = b.boundingRect();
      return bbox_a.width() * bbox_a.height() > bbox_b.width() * bbox_b.height();
    });
}

void traverse(QVector<QPolygonF>& polys, const NestedPolygonF& node) {
  for (const auto& child : node.children) {
    traverse(polys, child);
  }
  if (!node.polygon.isEmpty()) {
    polys.push_back(node.polygon);
  }
}

// ================ PathUtils =================
double PathUtils::getLength(QPointF& start, QPointF& end) {
  double dx = end.x() - start.x();
  double dy = end.y() - start.y();
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

void PathUtils::preprocessPath(QVector<NestedPolygonF>& polys) {
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

void PathUtils::sortByDistance(QVector<NestedPolygonF>& polys) {
  QVector<NestedPolygonF> sorted_polys;
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

void PathUtils::findChildren(QVector<QPolygonF>& polys, NestedPolygonF& parent, int index) {
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
void PathUtils::sortAndPreprocessPolygons(QVector<QPolygonF>& polys) {
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

NozzleSettings default_nozzle_settings;
QByteArray generate_nozzle_setting_payload(int saturation,
                                           float voltage,
                                           float pulse_width) {
  if (isnan(voltage)) {
    voltage = default_nozzle_settings.voltage;
  }
  if (isnan(pulse_width)) {
    pulse_width = default_nozzle_settings.pulse_width;
  }

  QByteArray payload;
  payload.append((const char*)(&voltage), 4);
  payload.append((const char*)(&pulse_width), 4);
  payload.append((const char*)(&saturation), 4);
  payload.append((const char*)(&default_nozzle_settings.DPI), 4);
  payload.append((const char*)(&default_nozzle_settings.ink_catridge_count), 4);
  payload.append((const char*)(&default_nozzle_settings.ink_type), 4);
  payload.append((const char*)(&default_nozzle_settings.nozzle_select), 4);
  payload.append((const char*)(&default_nozzle_settings.spray_time), 4);
  payload.append((const char*)(&default_nozzle_settings.ink_exchange), 4);
  payload.append((const char*)(&default_nozzle_settings.h_gap_ink1_ink2), 4);
  payload.append((const char*)(&default_nozzle_settings.v_gap_ink1_ink2), 4);
  payload.append((const char*)(&default_nozzle_settings.h_gap_ink2_ink3), 4);
  payload.append((const char*)(&default_nozzle_settings.v_gap_ink2_ink3), 4);
  payload.append((const char*)(&default_nozzle_settings.h_gap_ink3_ink4), 4);
  payload.append((const char*)(&default_nozzle_settings.v_gap_ink3_ink4), 4);
  return payload;
}

QSet<CollisionRegions> check_intersection(
    const QList<LayerPtr>& layers,
    HardwareType hardware,
    QSet<LayerModule> all_modules,
    double acc,
    QMap<LayerModule, QPointF> module_offsets,
    double min_printing_padding,
    double min_engraving_padding) {
  if (hardware != HardwareType::BM2) {
    return {};
  }

  QSet<CollisionRegions> regions;
  if (all_modules.contains(LayerModule::PRINTER_4C)) {
    regions << CollisionRegions::FBM2_SLIDING_TABLE;
  }
  if (regions.isEmpty()) {
    return {};
  }

  QSet<CollisionRegions> intersected_regions;
  std::vector<cv::Point2f> printer_points = {{0, 0}};
  std::vector<cv::Point2f> laser_points = {{0, 0}};
  double ppmm = 10;

  for (const LayerPtr& layer : layers) {
    if (!layer->isVisible() || layer->repeat() == 0 ||
        layer->children().isEmpty()) {
      continue;
    }
    LayerModule module = LayerModule(layer->module());
    bool is_printing = is_printing_module(module);
    QPointF offset = module_offsets.value(module, QPointF(0, 0));

    QString raw_bbox = layer->rawBBox();
    QStringList bbox_data = raw_bbox.split(u',');
    if (bbox_data.size() != 4) {
      qWarning() << "Invalid bbox data:" << raw_bbox;
      continue;
    }
    QRectF bbox(bbox_data[0].toDouble() / ppmm, bbox_data[1].toDouble() / ppmm,
                bbox_data[2].toDouble() / ppmm, bbox_data[3].toDouble() / ppmm);
    double acc_dist = 0;
    if (layer->type() != Layer::Type::Line) {
      double min_padding = is_printing ? min_printing_padding : min_engraving_padding;
      double speed = is_printing ? layer->printingSpeed() : layer->speed();
      acc_dist = get_padding_dist(min_padding, speed, acc);
    }
    double min_x = bbox.x() - offset.x();
    double min_y = bbox.y() - offset.y();
    double max_x = min_x + bbox.width() + acc_dist;
    double max_y = min_y + bbox.height();
    min_x -= acc_dist;
    if (is_printing) {
      printer_points.emplace_back(min_x, min_y);
      printer_points.emplace_back(max_x, min_y);
      printer_points.emplace_back(max_x, max_y);
      printer_points.emplace_back(min_x, max_y);
    } else {
      laser_points.emplace_back(min_x, min_y);
      laser_points.emplace_back(max_x, min_y);
      laser_points.emplace_back(max_x, max_y);
      laser_points.emplace_back(min_x, max_y);
    }
  }

  std::vector<cv::Point2f> printer_hull;
  std::vector<cv::Point2f> laser_hull;
  std::vector<cv::Point2f> convex_hull;
  cv::convexHull(printer_points, printer_hull, true, true);
  cv::convexHull(laser_points, laser_hull, true, true);
  std::vector<cv::Point2f> all_points = printer_hull;
  all_points.insert(all_points.end(), laser_hull.begin(), laser_hull.end());
  cv::convexHull(all_points, convex_hull, true, true);

  for (auto key = regions.constBegin(); key != regions.constEnd(); ++key) {
    if (intersected_regions.contains(*key)) {
      continue;
    }
    RegionData data = REGIONS.value(*key);
    std::vector<cv::Point2f>* hull = &convex_hull;
    if (data.type == RegionType::PRINTER) {
      hull = &printer_hull;
    } else if (data.type == RegionType::LASER) {
      hull = &laser_hull;
    }
    if (cv::intersectConvexConvex(*hull, data.points, cv::noArray(), true)) {
      intersected_regions << *key;
    }
  }

  return intersected_regions;
}

HardwareType model_to_hardware_type(const QString& model) {
  if (model == "fbm1")
    return HardwareType::beamo;
  if (model == "fbb1p")
    return HardwareType::BeamboxPro;
  if (model == "fhexa1")
    return HardwareType::HEXA;
  if (model == "ado1")
    return HardwareType::Ador;
  if (model == "fbb2")
    return HardwareType::BB2;
  if (model == "fbm2")
    return HardwareType::BM2;
  if (model.startsWith("fhx2rf"))
    return HardwareType::RF;
  // default beambox
  return HardwareType::Beambox;
}

/**
 * @param speed speed in mm/min
 * @return backlash compensation in mm
 */
double get_backlash_compensation(HardwareType hw_type,
                                 double speed,
                                 MachineModules machine_module) {
  double speed_mm_s = speed / 60;
  if (hw_type == HardwareType::Ador) {
    if (speed_mm_s >= 325)
      return 0.4;
    if (speed_mm_s >= 225)
      return 0.3;
    if (speed_mm_s >= 150)
      return 0.2;
    if (speed_mm_s >= 75)
      return 0.1;
    return 0;
  } else if (hw_type == HardwareType::BM2) {
    if (machine_module == MachineModules::PRINTER_4C) {
      if (speed_mm_s >= 500)
        return 0.3;
      if (speed_mm_s >= 100)
        return 0.2;
      return 0;
    }
    if (machine_module == MachineModules::LASER_1064) {
      return 0.1;
    }
    if (speed_mm_s >= 350)
      return 0.4;
    if (speed_mm_s >= 200)
      return 0.3;
    if (speed_mm_s >= 100)
      return 0.2;
    return 0;
  }
  return 0;
}

/**
 * @return laser delay in ms
 */
int get_laser_delay(HardwareType hw_type, int watt) {
  if (hw_type == HardwareType::RF) {
    if (watt == 30)
      return 1500;
    return 1390;
  }
  return 0;
}

bool is_printing_module(LayerModule module) {
  switch (module) {
    case LayerModule::PRINTER:
    case LayerModule::PRINTER_4C:
      return true;
    default:
      return false;
  }
}
bool is_uv_module(LayerModule module) {
  switch (module) {
    case LayerModule::WHITE_INK:
    case LayerModule::VARNISH:
      return true;
    default:
      return false;
  }
}

PrintingColor get_color(QString hex_color) {
  hex_color = hex_color.toUpper();
  if (hex_color == "#9FE3FF" || hex_color == "#009FE3") {
    return PrintingColor::CYAN;
  } else if (hex_color == "#E6007E") {
    return PrintingColor::MAGENTA;
  } else if (hex_color == "#FFED00") {
    return PrintingColor::YELLOW;
  } else if (hex_color == "#1D1D1B") {
    return PrintingColor::BLACK;
  } else if (hex_color == "#E2E2E2") {
    return PrintingColor::WHITE;
  } else {
    return PrintingColor::BLACK;
  }
}

// Get boundary for different machine and module combination
// return InwardRect in mm
InwardRect get_boundary(HardwareType hw_type, LayerModule layer_module) {
  InwardRect rect;
  if (hw_type == HardwareType::Ador) {
    if (layer_module == LayerModule::LASER_10W) {
      rect.bottom = 20;
    } else if (layer_module == LayerModule::PRINTER) {
      rect.bottom = 50;
    } else if (layer_module == LayerModule::LASER_20W) {
      rect.bottom = 30;
    } else if (layer_module == LayerModule::LASER_1064) {
      rect.bottom = 38;
    }
  } else if (hw_type == HardwareType::BM2) {
    if (layer_module == LayerModule::LASER_1064) {
      rect.right = 90;
      rect.bottom = 60;
    } else {
      rect.bottom = 40;
    }
  }
  return rect;
}

double get_default_min_padding(HardwareType hw_type,
                               LayerModule layer_module,
                               MachineModules machine_module,
                               bool is_high_quality) {
  double value = is_high_quality ? 10 : 0;
  double hardware_padding = HARDWARE_MIN_PADDING.value(hw_type, 0);
  value = qMax(value, hardware_padding);
  double machine_module_padding = MACHINE_MODULE_MIN_PADDING.value(machine_module, 0);
  value = qMax(value, machine_module_padding);
  if (!SUPPORT_INFO.value(hw_type).MODULES) {
    return value;
  }
  return qMax(value, MIN_PADDING.value(layer_module, 0));
}

/**
 * @param min_padding in mm
 * @param speed in mm/s
 * @param acc in mm/s^2
 * @return padding distance in mm
 */
double get_padding_dist(double min_padding, float speed, float acc) {
  return qMax((pow(speed, 2)) / (2.0 * acc), qMax(min_padding, 0.0));
}
