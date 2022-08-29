#include <toolpath_exporter/toolpath-exporter.h>
#include <QElapsedTimer>
#include <QDebug>
#include <QVector2D>
#include <QtMath>
#include <QProgressDialog>
#include <QCoreApplication>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <boost/range/irange.hpp>
#include <constants.h>

ToolpathExporter::ToolpathExporter(BaseGenerator *generator, qreal dpmm, PaddingType padding_type) noexcept :
 gen_(generator), dpmm_(dpmm), padding_type_(padding_type)
{
  resolution_scale_ = dpmm_ / canvas_mm_ratio_;
  resolution_scale_transform_ = QTransform::fromScale(resolution_scale_, resolution_scale_);
  global_transform_ = QTransform() * resolution_scale_transform_;
}

/**
 * @brief Convert all visible layers in the document into gcode (result is stored in gcode generator)
 * @param layers MUST contain at least one layer
 * @retval true if completed,
 *         false if canceled or error occurred
 */
bool ToolpathExporter::convertStack(const QList<LayerPtr> &layers, bool is_high_speed, QProgressDialog* dialog) {
  is_high_speed_ = is_high_speed;
  QElapsedTimer t;
  t.start();
  // Pre cmds
  gen_->turnOffLaser(); // M5
  gen_->home();
  gen_->useAbsolutePositioning();

  Q_ASSERT_X(!layers.empty(), "ToolpathExporter", "Must input at least one layer");
  canvas_size_ = QSizeF((layers.at(0)->document()).width(), 
                        (layers.at(0)->document()).height())
                 * resolution_scale_;

  // Generate bitmap canvas
  layer_bitmap_ = QPixmap(QSize(canvas_size_.width(),
                                canvas_size_.height()));
  layer_bitmap_.fill(Qt::white);

  bitmap_dirty_area_ = QRectF();

  bool canceled = false;
  if (dialog != nullptr) {
    connect(dialog, &QProgressDialog::canceled, [&]() {
      canceled = true;
    });
  }
  int processed_layer_cnt = 0;
  for (auto layer_rit = layers.crbegin(); layer_rit != layers.crend(); layer_rit++) {
    if (canceled) {
      return false;
    }
    if ((*layer_rit)->isVisible()) {
      qInfo() << "[Export] Output layer: " << (*layer_rit)->name();
      for (int i = 0; i < (*layer_rit)->repeat(); i++) {
        convertLayer((*layer_rit));
      }
    }
    processed_layer_cnt++;
    if (dialog != nullptr) {
      dialog->setValue(100 * processed_layer_cnt / layers.count());
      QCoreApplication::processEvents();
    }
  }

  // Post cmds
  gen_->home();
  qInfo() << "[Export] Took " << t.elapsed() << " milliseconds";
  return true;
}


/**
 * @brief Convert Shapes on canvas layer into scaled polygons/layer_bitmap (by resolution)
 *        e.g. if dpmm = 10 (Mid res), canvas_mm = 10 -> scale by 1
 *             if dpmm = 20 (High res), canvas_mm = 10 -> scale by 2
 * @param layer
 */
void ToolpathExporter::convertLayer(const LayerPtr &layer) {
  // Reset context states for the layer
  // TODO (Use layer_painter to manage transform over different sub objects)
  global_transform_ = QTransform() * resolution_scale_transform_;
  polygons_mutex_.lock();
  layer_polygons_.clear();
  polygons_mutex_.unlock();
  layer_painter_ = std::make_unique<QPainter>(&layer_bitmap_);
  //layer_painter_->fillRect(bitmap_dirty_area_, Qt::white);
  bitmap_dirty_area_ = QRectF();
  current_layer_ = layer;
  for (auto &shape : layer->children()) {
    convertShape(shape);
  }
  layer_painter_->end();
  sortPolygons();
  outputLayerGcode();
}

void ToolpathExporter::convertShape(const ShapePtr &shape) {
  switch (shape->type()) {
    case Shape::Type::Group:
      convertGroup(dynamic_cast<GroupShape *>(shape.get()));
      break;

    case Shape::Type::Bitmap:
      convertBitmap(dynamic_cast<BitmapShape *>(shape.get()));
      break;

    case Shape::Type::Path:
    case Shape::Type::Text:
      convertPath(dynamic_cast<PathShape *>(shape.get()));
      break;

    default:
      break;
  }
}

void ToolpathExporter::convertGroup(const GroupShape *group) {
  global_transform_ = group->globalTransform() * resolution_scale_transform_;
  for (auto &shape : group->children()) {
    convertShape(shape);
  }
  global_transform_ = QTransform() * resolution_scale_transform_;
}

/**
 * @brief Draw the image shape on canvas onto layer pixmap
 * @param bmp
 */
void ToolpathExporter::convertBitmap(const BitmapShape *bmp) {
  QTransform transform = bmp->transform() * global_transform_;
  layer_painter_->save();
  layer_painter_->setTransform(transform, false);
  if (bmp->gradient()) { // gradient mode
    layer_painter_->drawPixmap(0, 0, QPixmap::fromImage(bmp->sourceImage()));
  } else { // binarize mode
    layer_painter_->drawPixmap(0, 0, QPixmap::fromImage( imageBinarize(bmp->sourceImage(), bmp->thrsh_brightness()) ));
  }
  layer_painter_->restore();
  bitmap_dirty_area_ = bitmap_dirty_area_.united(global_transform_.mapRect(bmp->boundingRect()));
}

void ToolpathExporter::convertPath(const PathShape *path) {
  // qInfo() << "Convert Path" << path;
  QPainterPath transformed_path = (path->transform() * global_transform_).map(path->path());

  if ((path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
      current_layer_->type() == Layer::Type::Fill ||
      current_layer_->type() == Layer::Type::FillLine) {
    // TODO (Fix overlapping fills inside a single layer)
    // TODO (Consider CacheStack as a primary painter for layers?)
    layer_painter_->setPen(Qt::NoPen); // Otherwise, the border would occupy at least 1 pixel
    layer_painter_->setBrush(Qt::black);
    layer_painter_->drawPath(transformed_path);
    layer_painter_->setBrush(Qt::NoBrush);
    bitmap_dirty_area_ = bitmap_dirty_area_.united(transformed_path.boundingRect());
  }
  if ((!path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
      current_layer_->type() == Layer::Type::Line ||
      current_layer_->type() == Layer::Type::FillLine) {
    polygons_mutex_.lock();
    layer_polygons_.append(transformed_path.toSubpathPolygons());
    polygons_mutex_.unlock();
  }
}

void ToolpathExporter::sortPolygons() {
  // Performance: O(n^2)
  QList<QPolygonF> sort_result;
  polygons_mutex_.lock();
  for (const auto& polygon: layer_polygons_) {
    int insert_idx = sort_result.size();
    for (int idx = sort_result.size() - 1; idx >= 0; idx--) {
      if (sort_result[idx].boundingRect().contains(polygon.boundingRect())) {
        insert_idx = idx;
      }
    }
    sort_result.insert(insert_idx, polygon);
  }

  layer_polygons_ = sort_result;
  polygons_mutex_.unlock();
  return;
}

void ToolpathExporter::outputLayerGcode() {
  outputLayerBitmapGcode();
  outputLayerPathGcode();
}

/**
 * @brief Gcode for non-filled geometry
 */
void ToolpathExporter::outputLayerPathGcode() {

  gen_->turnOnLaser(); // M3

  // NOTE: Should convert points from canvas unit to mm
  polygons_mutex_.lock();
  for (auto &poly : layer_polygons_) {
    if (poly.empty()) continue;

    QPointF next_point_mm = poly.first() / dpmm_;
    moveTo(next_point_mm,
           MOVING_SPEED,
           0);

    for (QPointF &point : poly) {
      next_point_mm = point / dpmm_;
      // Divide a long line into small segments
      if ( (next_point_mm - current_pos_).manhattanLength() > 5 ) { // At most 5mm per segment
        int segments = std::max(2.0,
                           std::sqrt(std::pow(next_point_mm.x() - current_pos_.x(), 2) +
                                std::pow(next_point_mm.y() - current_pos_.y(), 2)) / 5 );
        QList<QPointF> interpolate_points;
        for (int i = 1; i <= segments; i++) {
          interpolate_points << (i * next_point_mm + (segments - i) * current_pos_) / float(segments);
        }
        for (const QPointF &interpolate_point : interpolate_points) {
          moveTo(interpolate_point,
                 current_layer_->speed(),
                 current_layer_->power());
        }
      } else {
        moveTo(next_point_mm,
               current_layer_->speed(),
               current_layer_->power());
      }

    }

    //gen_->turnOffLaser();
  }
  polygons_mutex_.unlock();
  // gen_->moveTo(gen_->x(), gen_->y(), current_layer_->speed(), 0);
  gen_->turnOffLaser();
}

/**
 * @brief Gcode for filled geometry and images
 */
void ToolpathExporter::outputLayerBitmapGcode() {
  if (bitmap_dirty_area_.width() == 0) return;
  // Get the image of entire layer
  QImage layer_image = layer_bitmap_.toImage()
                      .convertToFormat(QImage::Format_Mono, Qt::MonoOnly | Qt::DiffuseDither)
                      .convertToFormat(QImage::Format_Grayscale8);

  qreal padding_mm;
  qreal accelerate = 4000; // mm/s^2
  switch (padding_type_) {
    case PaddingType::kFixedPadding:
      padding_mm = fixed_padding_mm_;
      break;
    case PaddingType::kDynamicPadding:
      // Adjust padding based on layer speed
      padding_mm = qreal(current_layer_->speed()*current_layer_->speed()) / (2*accelerate); // d = v^2 / 2a
      break;
    case PaddingType::kNoPadding:
    default:
      padding_mm = 0;
      break;
  }
  // NOTE: Express bbox in # of dots
  //       Reserve x-direction padding in bounding box (for acceleration distance)
  const qreal mm_per_dot = 1.0 / dpmm_;    // unit size of engraving dot (segment)
  QRect bbox{QPoint{qMax(qRound(bitmap_dirty_area_.topLeft().x() - padding_mm * dpmm_), 0),
               qMax(qRound(bitmap_dirty_area_.topLeft().y()), 0)},
             QPoint{qMin(qRound(bitmap_dirty_area_.bottomRight().x() + padding_mm * dpmm_), canvas_size_.toSize().width() - 1),
               qMin(qRound(bitmap_dirty_area_.bottomRight().y()), canvas_size_.toSize().height() - 1)}};

  gen_->turnOnLaserAdpatively(); // M4

  // rapid move to the start position
  moveTo(QPointF{bbox.topLeft()} / dpmm_,
         MOVING_SPEED,
         0);

  gen_->useRelativePositioning();

  // Start raster
  if(is_high_speed_) {
    rasterBitmapHighSpeed(layer_image, bbox,ScanDirectionMode::kBidirectionMode, padding_mm);
  }
  else {
    rasterBitmap(layer_image, bbox, ScanDirectionMode::kBidirectionMode, padding_mm);
  }

  gen_->useAbsolutePositioning();

  gen_->turnOffLaser();
}

/**
 * @brief Remove the suffix and prefix zeros
 *
 * @param src_bit_array
 * @return std::tuple<std::vector<std::bitset<32>>, uint32_t, uint32_t>
 *         - the trimmed bit array
 *         - the bit idx of the trim start
 *         - the bit idx of the trim end
 */
std::tuple<std::vector<std::bitset<32>>, uint32_t, uint32_t> ToolpathExporter::adjustPrefixSuffixZero(
      const std::vector<std::bitset<32>>& src_bit_array, uint32_t padding_dot_cnt) {

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

/**
 *
 * @param layer_image the (scaled) bitmap of entire layer
 * @param bbox bounding box of dirty area
 * @param diection_mode
 * @return
 */
bool ToolpathExporter::rasterBitmap(const QImage &layer_image,
    QRect bbox, 
    ScanDirectionMode direction_mode, 
    qreal padding_mm) {

  const int white_pixel = 255;
  int current_grayscale = white_pixel; // 0-255 from dark (black) to bright (white)
  bool reverse_raster_dir = false;

  // Prepare raster line paths
  QList<QLine> raster_lines;
  for (int y = bbox.top(); y <= bbox.bottom(); y += 1) {
    raster_lines.push_back(QLine{bbox.left(), y,
                                 bbox.right(), y});
  }
  qInfo() << "bbox: " << bbox;
  qInfo() << "# of raster line: " << raster_lines.size();

  // 2-2. iterate
  for (const auto &raster_line: raster_lines) {
    // Initialize
    if (raster_line.isNull()) {
      continue;
    }
    std::vector<std::bitset<32>> dot_data_list;
    // TBD: Consider encapsulate the following into a class/struct
    const QPoint initial_pos = reverse_raster_dir ?
                               raster_line.p2() :
                               raster_line.p1();
    const QPoint end_pos = reverse_raster_dir ?
                           raster_line.p1() :
                           raster_line.p2();
    const QLineF path{initial_pos, end_pos};
    const qreal t_step = 1 / path.length();
    qreal current_t_sample = t_step / 2; // NOTE: Offset by dot_size/2 to get the nearest pixel value
    QPointF current_pos_sample = path.pointAt(current_t_sample);

    // Scan an entire line of bitmap
    std::bitset<32> data_word = 0;
    uint32_t bit_idx = 31;
    bool blank_line = true; // blank line filled with white pixels entirely
    while (1) {
      const uchar *data_ptr = layer_image.constScanLine(current_pos_sample.y());
      int dot_grayscale = data_ptr[int(current_pos_sample.x())];
      //qInfo() << dot_grayscale;
      if (dot_grayscale < white_pixel && blank_line) {
        blank_line = false;
      }
      dot_grayscale == white_pixel ? data_word.reset(bit_idx) : data_word.set(bit_idx);

      if (bit_idx == 0) {
        dot_data_list.push_back(data_word);
        data_word.reset();
        bit_idx = 31;
      } else {
        bit_idx--;
      }

      current_t_sample += t_step;
      if ( current_t_sample >= 0.99999) { // terminate condition: Reach end pos (within epsilon distance)
        if (bit_idx != 31) { // Append the remaining partial word
          dot_data_list.push_back(data_word);
        }
        break;
      }
      current_pos_sample = path.pointAt(current_t_sample);
    } // End of parsing of the raster line

    if (blank_line) {
      continue; // ignore blank line
    }

    auto [ trimmed_bit_array, first_pos_idx, last_pos_idx ] =
    adjustPrefixSuffixZero(dot_data_list, padding_mm * dpmm_);
    // NOTE +1 to the end pos because we move to "the end of the last dot"
    QLineF dirty_line_segment {
            path.pointAt(first_pos_idx * t_step),
            path.pointAt(qMin((last_pos_idx + 1) * t_step, 1.0))
    };

    rasterLine(dirty_line_segment, trimmed_bit_array);

    // Switch scan direction if config
    if (direction_mode == ScanDirectionMode::kBidirectionMode) {
      reverse_raster_dir = !reverse_raster_dir;
    }

  } // end of all raster lines

  return true;
}

bool ToolpathExporter::rasterLine(const QLineF& path, const std::vector<std::bitset<32>>& data) {
  bool is_emitting = false;
  const qreal t_step = 1 / path.length();
  moveTo(path.p1() / dpmm_,
         MOVING_SPEED,
         0);

  int idx = 0;
  while (true) {
    if (t_step * idx >= 1) {
      moveTo(path.p2() / dpmm_,
             current_layer_->speed(),
             is_emitting ? current_layer_->power() : 0);
      break;
    }
    if (int(idx/32) >= data.size()) {
      if (is_emitting) {
        moveTo(path.pointAt(t_step * idx) / dpmm_,
               current_layer_->speed(),
               current_layer_->power());
      }
      moveTo(path.p2() / dpmm_,
             current_layer_->speed(),
             0);
      break;
    }

    if (is_emitting != data[int(idx/32)][31 - (idx % 32)]) {
      moveTo(path.pointAt(t_step * idx) / dpmm_,
             current_layer_->speed(),
             is_emitting ? current_layer_->power() : 0);
      is_emitting = !is_emitting;
    }
    idx++;
  }

  return true;
}

/**
 * @brief Export
 * @param layer_image  (scaled) image of an entire layer on canvas
 * @param bbox         the (scaled) bounding box for the raster motion (shouldn't be larger than layer_image)
 * @return
 */
bool ToolpathExporter::rasterBitmapHighSpeed(const QImage &layer_image,
    QRect bbox,
    ScanDirectionMode direction_mode,
    qreal padding_mm) {

  // 1. Enter fast raster mode
  gen_->appendCustomCmd(std::string("D0R") +
        std::string(1/dpmm_ >= 0.2 ? "L" :
                    1/dpmm_ >= 0.1 ? "M" :
                    1/dpmm_ >= 0.05 ? "H" : "U") +
       std::string("\n")
   );

  // 2. Parsing bitmap data and generate command for each raster line
  const int white_pixel = 255;
  bool is_emitting_laser = false;
  bool reverse_raster_dir = false;

  // 2-1. Prepare raster line paths
  QList<QLine> raster_lines;
  for (int y = bbox.top(); y <= bbox.bottom(); y += 1) {
    raster_lines.push_back(QLine{bbox.left(), y,
                                  bbox.right(), y});
  }
  qInfo() << "bbox: " << bbox;
  qInfo() << "# of raster line: " << raster_lines.size();

  // 2-2. iterate
  for (const auto &raster_line: raster_lines) {
    // Initialize
    if (raster_line.isNull()) {
      continue;
    }
    std::vector<std::bitset<32>> dot_data_list;
    // TBD: Consider encapsulate the following into a class/struct
    const QPoint initial_pos = reverse_raster_dir ?
                       raster_line.p2() :
                       raster_line.p1();
    const QPoint end_pos = reverse_raster_dir ?
                          raster_line.p1() :
                          raster_line.p2();
    const QLineF path{initial_pos, end_pos};
    const qreal t_step = 1 / path.length();
    qreal current_t_sample = t_step / 2; // NOTE: Offset by dot_size/2 to get the nearest pixel value
    QPointF current_pos_sample = path.pointAt(current_t_sample);

    // Scan an entire line of bitmap
    std::bitset<32> data_word = 0;
    uint32_t bit_idx = 31;
    bool blank_line = true; // blank line filled with white pixels entirely
    while (1) {
      const uchar *data_ptr = layer_image.constScanLine(current_pos_sample.y());
      int dot_grayscale = data_ptr[int(current_pos_sample.x())];
      //qInfo() << dot_grayscale;
      if (dot_grayscale < white_pixel && blank_line) {
        blank_line = false;
      }
      dot_grayscale == white_pixel ? data_word.reset(bit_idx) : data_word.set(bit_idx);

      if (bit_idx == 0) {
        dot_data_list.push_back(data_word);
        data_word.reset();
        bit_idx = 31;
      } else {
        bit_idx--;
      }

      current_t_sample += t_step;
      if ( current_t_sample >= 0.99999) { // terminate condition: Reach end pos (within epsilon distance)
        if (bit_idx != 31) { // Append the remaining partial word
          dot_data_list.push_back(data_word);
        }
        break;
      }
      current_pos_sample = path.pointAt(current_t_sample);
    } // End of parsing of the raster line

    if (blank_line) {
      continue; // ignore blank line
    }

    auto [ trimmed_bit_array, first_pos_idx, last_pos_idx ] =
            adjustPrefixSuffixZero(dot_data_list, padding_mm * dpmm_);
    // NOTE +1 to the end pos because we move to "the end of the last dot"
    QLineF dirty_line_segment {
            path.pointAt(first_pos_idx * t_step),
            path.pointAt(qMin((last_pos_idx + 1) * t_step, 1.0))
    };

    rasterLineHighSpeed(dirty_line_segment, trimmed_bit_array);

    // Switch scan direction if config
    if (direction_mode == ScanDirectionMode::kBidirectionMode) {
      reverse_raster_dir = !reverse_raster_dir;
    }

  } // end of all raster lines

  // 5. Exit fast raster mode
  gen_->appendCustomCmd(std::string("D5\n"));

  return true;
}

/**
 * @brief Export D1PC[n], D2WXXXXXX, D3FE and D4PL cmd for an entire row
 * @param data row data
 * @return
 */
bool ToolpathExporter::rasterLineHighSpeed(const QLineF& path, const std::vector<std::bitset<32>>& data) {
  // Generate moveTo cmd to the initial position of raster line
  moveTo(path.p1() / dpmm_,
         MOVING_SPEED,
         0);

  // Generate D1PC, D2W, D3FE, D4PL cmd based on the parsed info
  gen_->appendCustomCmd(std::string("D1PC") + std::to_string(32*data.size()) + std::string("\n"));

  // Generate D2W commands 6*32 pixels per D2W cmd
  // WARNING: Shouldn't exceed the cmd line buffer size (e.g. 80) of grbl machine
  std::stringstream ss;
  for (auto i = 0; i < data.size(); i++) {
    if (i % 6 == 0) {
      ss.str(std::string());
      ss << "D2W";
    }
    ss << std::setfill('0') << std::setw(8) << std::hex << std::uppercase << data[i].to_ulong();
    if (i % 6 == 5 || (i == data.size() - 1)) {
      ss << "\n";
      gen_->appendCustomCmd(ss.str());
    }
  }

  gen_->appendCustomCmd(std::string("D3FE\n"));
  gen_->appendCustomCmd(std::string("D4PL\n"));

  // Generate moveTo cmd to the end position of raster line
  moveTo(path.p2() / dpmm_,
         current_layer_->speed(),
         current_layer_->power());

  return true;
}


/**
 * @brief 
 * 
 * @param src must be Format_ARGB32 grayscaled image
 * @param threshold 
 * @return QImage 
 */
QImage ToolpathExporter::imageBinarize(QImage src, int threshold) {
  Q_ASSERT_X(src.allGray(), "ToolpathExporter", "Input image for imageBinarize() must be grayscaled");
  Q_ASSERT_X(src.format() == QImage::Format_ARGB32, "ToolpathExporter", "Input image for imageBinarize() must be Format_ARGB32");
  
  QImage result_img{src.width(), src.height(), QImage::Format_Grayscale8};

  for (int y = 0; y < src.height(); ++y) {
    for (int x = 0; x < src.width(); ++x) {
      int grayscale_val = qGray(src.pixel(x, y));
      result_img.setPixel(x, y,
                          grayscale_val <= threshold ? qRgb(0, 0, 0) :
                          qRgb(255, 255, 255));
    }
  }
  return result_img;
}

inline void ToolpathExporter::moveTo(QPointF&& dest, double speed, double power) {
  gen_->moveTo(dest.x(), dest.y(), speed, power);
  current_pos_ = dest;
}

inline void ToolpathExporter::moveTo(const QPointF& dest, double speed, double power) {
  gen_->moveTo(dest.x(), dest.y(), speed, power);
  current_pos_ = dest;
}
