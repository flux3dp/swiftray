#include <gcode/toolpath-exporter.h>
#include <QElapsedTimer>
#include <QDebug>
#include <QVector2D>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <boost/range/irange.hpp>
#include <constants.h>

ToolpathExporter::ToolpathExporter(BaseGenerator *generator, qreal dpmm) noexcept :
 gen_(generator), dpmm_(dpmm)
{
  global_transform_ = QTransform();
}

/**
 * @brief Convert all visible layers in the document into gcode (result is stored in gcode generator)
 * @param layers MUST contain at least one layer
 */
void ToolpathExporter::convertStack(const QList<LayerPtr> &layers) {
  QElapsedTimer t;
  t.start();
  // Pre cmds
  gen_->turnOffLaser(); // M5
  gen_->home();
  gen_->useAbsolutePositioning();

  Q_ASSERT_X(!layers.empty(), "ToolpathExporter", "Must input at least one layer");
  canvas_size_ = QSizeF((layers.at(0)->document()).width(), (layers.at(0)->document()).height());

  // Generate bitmap canvas
  layer_bitmap_ = QPixmap(QSize(canvas_size_.width(), canvas_size_.height()));
  layer_bitmap_.fill(Qt::white);

  bitmap_dirty_area_ = QRectF();

  for (auto layer_rit = layers.crbegin(); layer_rit != layers.crend(); layer_rit++) {
    if (!(*layer_rit)->isVisible()) {
      continue;
    }
    qInfo() << "[Export] Output layer: " << (*layer_rit)->name();
    for (int i = 0; i < (*layer_rit)->repeat(); i++) {
      convertLayer((*layer_rit));
    }
  }

  // Post cmds
  gen_->home();
  qInfo() << "[Export] Took " << t.elapsed() << " milliseconds";
}

void ToolpathExporter::convertLayer(const LayerPtr &layer) {
  // Reset context states for the layer
  // TODO (Use layer_painter to manage transform over different sub objects)
  global_transform_ = QTransform();
  layer_polygons_.clear();
  layer_painter_ = std::make_unique<QPainter>(&layer_bitmap_);
  layer_painter_->fillRect(bitmap_dirty_area_, Qt::white);
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
  global_transform_ = group->globalTransform();
  for (auto &shape : group->children()) {
    convertShape(shape);
  }
  global_transform_ = QTransform();
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
    layer_painter_->drawPixmap(0, 0, QPixmap::fromImage(bmp->sourceImage().convertToFormat(QImage::Format_Mono, Qt::MonoOnly | Qt::DiffuseDither)));
  } else { // binarize mode
    layer_painter_->drawPixmap(0, 0, QPixmap::fromImage( imageBinarize(bmp->sourceImage(), bmp->thrsh_brightness()) ));
  }
  layer_painter_->restore();
  bitmap_dirty_area_ = bitmap_dirty_area_.united(bmp->boundingRect());
}

void ToolpathExporter::convertPath(const PathShape *path) {
  // qInfo() << "Convert Path" << path;
  QPainterPath transformed_path = (path->transform() * global_transform_).map(path->path());

  if ((path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
      current_layer_->type() == Layer::Type::Fill ||
      current_layer_->type() == Layer::Type::FillLine) {
    // TODO (Fix overlapping fills inside a single layer)
    // TODO (Consider CacheStack as a primary painter for layers?)
    layer_painter_->setBrush(Qt::black);
    layer_painter_->drawPath(transformed_path);
    layer_painter_->setBrush(Qt::NoBrush);
    bitmap_dirty_area_ = bitmap_dirty_area_.united(transformed_path.boundingRect());
  }
  if ((!path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
      current_layer_->type() == Layer::Type::Line ||
      current_layer_->type() == Layer::Type::FillLine) {
    layer_polygons_.append(transformed_path.toSubpathPolygons());
  }
}

void ToolpathExporter::sortPolygons() {
  // TODO (Path order optimization)
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
  for (auto &poly : layer_polygons_) {
    if (poly.empty()) continue;

    QPointF next_point_mm = poly.first() / canvas_mm_ratio_;
    moveTo(next_point_mm,
           MOVING_SPEED,
           0);

    for (QPointF &point : poly) {
      next_point_mm = point / canvas_mm_ratio_;
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
  // gen_->moveTo(gen_->x(), gen_->y(), current_layer_->speed(), 0);
  gen_->turnOffLaser();
}

/**
 * @brief Gcode for filled geometry and images
 */
void ToolpathExporter::outputLayerBitmapGcode() {
  if (bitmap_dirty_area_.width() == 0) return;
  // Get the image of entire layer
  QImage layer_image = layer_bitmap_.toImage().convertToFormat(QImage::Format_Grayscale8);

  qreal canvas_physical_width = canvas_size_.width() / canvas_mm_ratio_;
  layer_image.setDotsPerMeterX(1000 * layer_image.width() / canvas_physical_width); // m/pixel = 1000 * mm/pixel
  layer_image.setDotsPerMeterY(1000 * layer_image.width() / canvas_physical_width);
  const qreal mm_per_pixel = 1000.0 / layer_image.dotsPerMeterX();
  // NOTE: Should convert points from canvas unit to mm
  //       Reserve x-direction padding in bounding box (for acceleration distance)
  QRectF bbox_mm{QPointF{qMax(bitmap_dirty_area_.topLeft().x() / canvas_mm_ratio_ - padding_mm_, 0.0),
                                 bitmap_dirty_area_.topLeft().y() / canvas_mm_ratio_},
                 QPointF{qMin(bitmap_dirty_area_.bottomRight().x() / canvas_mm_ratio_ + padding_mm_, (qreal)(layer_image.width())),
                                   bitmap_dirty_area_.bottomRight().y() / canvas_mm_ratio_}};
  const qreal mm_per_dot = 1.0 / dpmm_;    // unit size of engraving dot (segment)

  gen_->turnOnLaserAdpatively(); // M4

  // rapid move to the start position
  moveTo(bbox_mm.topLeft(),
         MOVING_SPEED,
         0);

  gen_->useRelativePositioning();

  // Start raster
  rasterBitmapHighSpeed(layer_image, mm_per_pixel,
                mm_per_dot, bbox_mm,
                ScanDirectionMode::kBidirectionMode);
  //rasterBitmap(layer_image, mm_per_pixel,
  //              mm_per_dot, bbox_mm,
  //              ScanDirectionMode::kBidirectionMode);

  gen_->useAbsolutePositioning();

  gen_->turnOffLaser();
}

/**
 *
 * @param layer_image the bitmap of entire layer
 * @param mm_per_pixel
 * @param mm_per_dot   unit length of engraving segment
 * @param bbox_mm bounding box of dirty area
 * @param diection_mode
 * @return
 */
bool ToolpathExporter::rasterBitmap(const QImage &layer_image,
    const qreal &mm_per_pixel,
    const qreal &mm_per_dot,
    QRectF bbox_mm, ScanDirectionMode direction_mode) {

  const int white_pixel = 255;
  int current_grayscale = white_pixel; // 0-255 from dark (black) to bright (white)
  bool reverse_raster_dir = false;

  // Prepare raster line paths
  QList<QLineF> raster_lines;
  for (qreal y = bbox_mm.top(); y <= bbox_mm.bottom(); y += mm_per_dot) {
    raster_lines.push_back(QLineF{bbox_mm.left(), y,
                                     bbox_mm.right(), y});
  }
  qInfo() << "mm_per_pixel: " << mm_per_pixel << ", mm_per_dot: " << mm_per_dot;
  qInfo() << "bbox: " << bbox_mm;
  qInfo() << "# of raster line: " << raster_lines.size();

  for (const auto &raster_line: raster_lines) {
    // Initialize
    QPointF current_pos = reverse_raster_dir ?
            raster_line.p2() :
            raster_line.p1();
    QVector2D unit_dir = reverse_raster_dir ?
            QVector2D{raster_line.p1() - raster_line.p2()} :
            QVector2D{raster_line.p2() - raster_line.p1()};
    unit_dir.normalize();
    current_grayscale = white_pixel; // 0 power for motion across lines
    bool first_segment = true; // indicate the first segment in the current raster line
    const QPointF displace_per_dot = mm_per_dot * unit_dir.toPointF();

    // Start stepping each dot
    while (1) {
      QPointF next_pos = current_pos + displace_per_dot;
      if ( !bbox_mm.contains(next_pos) ) { // finish the remaining
        if (current_grayscale != white_pixel) {
          moveTo(current_pos,
                 current_layer_->speed(),
                 qRound(current_layer_->power() * (white_pixel - current_grayscale) / 255.0));
          current_grayscale = white_pixel;
        }
        break;
      }

      // Get pixel value from pos
      current_pos = next_pos;
      const uchar *data_ptr = layer_image.constScanLine(current_pos.y() / mm_per_pixel);
      int new_grayscale = data_ptr[int(current_pos.x() / mm_per_pixel)];
      //qInfo() << new_grayscale;
      // When encountering a grayscale change, the parsing for the next segment finished.
      // moveTo the pos_of_next_segment_end (current parser pos) with corresponding power
      if (new_grayscale != current_grayscale) {
        if (current_grayscale == white_pixel) { // white path - black path transition
          moveTo(current_pos,
                 first_segment ? MOVING_SPEED : current_layer_->speed(),
                 0);
        } else { // black path - white path transition
          moveTo(current_pos,
                 current_layer_->speed(),
                 qRound(current_layer_->power() * (white_pixel - current_grayscale) / 255.0));
        }
        if (first_segment) {
          first_segment = false;
        }
        current_grayscale = new_grayscale;
      }
    } // end of a raster line

    // Update scan direction
    if (direction_mode == ScanDirectionMode::kBidirectionMode) {
      if (!first_segment) { // if any segment exist in this line -> reverse direction in the next line
        reverse_raster_dir = !reverse_raster_dir;
      }
    }

  } // end of all raster lines

  return true;
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
static std::tuple<std::vector<std::bitset<32>>, uint32_t, uint32_t> trim_zero(
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
 * @brief Export
 * @param layer_image  image of an entire layer on canvas
 * @param mm_per_pixel canvas pixel size
 * @param mm_per_dot   laser unit size (machine engraving resolution)
 * @param bbox_mm      the STRICT bounding box for the raster motion
 * @return
 */
bool ToolpathExporter::rasterBitmapHighSpeed(const QImage &layer_image,
    const qreal &mm_per_pixel, 
    const qreal &mm_per_dot, 
    QRectF bbox_mm,
    ScanDirectionMode direction_mode) {

  // 1. Enter fast raster mode
  gen_->appendCustomCmd(std::string("D0R") +
        std::string(mm_per_pixel >= 0.2 ? "L" :
                    mm_per_pixel >= 0.1 ? "M" :
                    mm_per_pixel >= 0.05 ? "H" : "U") +
       std::string("\n")
   );

  // 2. Parsing bitmap data and generate command for each raster line
  const int white_pixel = 255;
  bool is_emitting_laser = false;
  bool reverse_raster_dir = false;

  // 2-1. Prepare raster line paths
  QList<QLineF> raster_lines;
  for (qreal y = bbox_mm.top(); y <= bbox_mm.bottom(); y += mm_per_dot) {
    raster_lines.push_back(QLineF{bbox_mm.left(), y,
                                  bbox_mm.right(), y});
  }
  qInfo() << "mm_per_pixel: " << mm_per_pixel << ", mm_per_dot: " << mm_per_dot;
  qInfo() << "bbox: " << bbox_mm;
  qInfo() << "# of raster line: " << raster_lines.size();

  // 2-2. iterate
  for (const auto &raster_line: raster_lines) {
    // Initialize
    if (raster_line.isNull()) {
      continue;
    }
    std::vector<std::bitset<32>> dot_data_list;
    // TBD: Consider encapsulate the following into a class/struct
    const QPointF initial_pos = reverse_raster_dir ?
                       raster_line.p2() :
                       raster_line.p1();
    const QPointF end_pos = reverse_raster_dir ?
                          raster_line.p1() :
                          raster_line.p2();
    const QLineF path{initial_pos, end_pos};
    const qreal t_per_dot = mm_per_dot / path.length();
    qreal current_t = 0;
    QPointF current_pos = path.pointAt(current_t);

    // Scan an entire line of bitmap
    std::bitset<32> data_word = 0;
    uint32_t bit_idx = 31;
    bool blank_line = true; // blank line filled with white pixels entirely
    while (1) {

      const uchar *data_ptr = layer_image.constScanLine(current_pos.y() / mm_per_pixel);
      int dot_grayscale = data_ptr[int(current_pos.x() / mm_per_pixel)];
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

      current_t += t_per_dot;
      if ( current_t >= 0.9999) { // terminate condition: Reach end pos (within epsilon distance)
        if (bit_idx != 31) { // Append the remaining partial word
          dot_data_list.push_back(data_word);
        }
        break;
      }
      current_pos = path.pointAt(current_t);
    } // End of parsing of the raster line

    if (blank_line) {
      continue; // ignore blank line
    }

    auto [ trimmed_bit_array, first_pos_idx, last_pos_idx ] =
            trim_zero(dot_data_list, padding_mm_ / mm_per_dot);
    // NOTE +1 to the end pos because we move to "the end of the last dot"
    QLineF dirty_line_segment {
            path.pointAt(first_pos_idx * t_per_dot),
            path.pointAt(qMin((last_pos_idx + 1) * t_per_dot, 1.0))
    };


    // Generate moveTo cmd to the initial position of raster line
    moveTo(dirty_line_segment.p1(),
           MOVING_SPEED,
           0);
    // Generate D1PC, D2W, D3FE, D4PL cmd based on the parsed info
    rasterLineHighSpeed(trimmed_bit_array);
    // Generate moveTo cmd to the end position of raster line
    moveTo(dirty_line_segment.p2(),
           current_layer_->speed(),
           current_layer_->power());

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
bool ToolpathExporter::rasterLineHighSpeed(const std::vector<std::bitset<32>>& data) {
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

inline void ToolpathExporter::moveTo(QPointF&& dest, int speed, int power) {
  gen_->moveTo(dest.x(), dest.y(), speed, power);
  current_pos_ = dest;
}

inline void ToolpathExporter::moveTo(const QPointF& dest, int speed, int power) {
  gen_->moveTo(dest.x(), dest.y(), speed, power);
  current_pos_ = dest;
}
