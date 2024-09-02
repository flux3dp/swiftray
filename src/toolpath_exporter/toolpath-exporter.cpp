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
#include <constants.h>

ToolpathExporter::ToolpathExporter(BaseGenerator *generator, qreal dpmm, double travel_speed, QPointF end_point, PaddingType padding_type, QTransform move_translate) noexcept :
 gen_(generator), dpmm_(dpmm), padding_type_(padding_type), travel_speed_(travel_speed), end_point_(end_point)
{
  resolution_scale_ = dpmm_ / canvas_mm_ratio_;
  resolution_scale_transform_ = QTransform::fromScale(resolution_scale_, resolution_scale_);
  move_translate_ = move_translate;
  global_transform_ = QTransform() * move_translate_ * resolution_scale_transform_;
}

/**
 * @brief Convert all visible layers in the document into gcode (result is stored in gcode generator)
 * @param layers MUST contain at least one layer
 * @retval true if completed,
 *         false if canceled or error occurred
 */
bool ToolpathExporter::convertStack(const QList<LayerPtr> &layers, bool is_high_speed, bool start_with_home) {
  is_high_speed_ = is_high_speed;
  QElapsedTimer t;
  t.start();
  // Pre cmds
  gen_->turnOffLaser(); // M5
  if(start_with_home) {
    gen_->home();
  }
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

  int processed_layer_cnt = 0;
  for (auto layer_rit = layers.crbegin(); layer_rit != layers.crend(); layer_rit++) {
    if ((*layer_rit)->isVisible()) {
      qInfo() << "[Export] Output layer: " << (*layer_rit)->name();
      for (int i = 0; i < (*layer_rit)->repeat(); i++) {
        convertLayer((*layer_rit));
      }
    }
    if (this->cancelled_) {
      break;
    }
    processed_layer_cnt++;
    Q_EMIT progressChanged(100 * processed_layer_cnt / layers.count());
    QCoreApplication::processEvents();
  }
  
  if (this->cancelled_) {
    return false;
  }

  gen_->finishProgramFlow();
  
  if (this->cancelled_) {
    return false;
  }

  // Post cmds
  // gen_->home();
  moveTo(end_point_, travel_speed_, 0, 0);
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
  global_transform_ = QTransform() * move_translate_ * resolution_scale_transform_;
  polygons_mutex_.lock();
  layer_polygons_.clear();
  polygons_mutex_.unlock();
  with_image_ = false;
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
      with_image_ = true;
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
  global_transform_ = group->globalTransform() * move_translate_ * resolution_scale_transform_;
  for (auto &shape : group->children()) {
    convertShape(shape);
  }
  global_transform_ = QTransform() * move_translate_ * resolution_scale_transform_;
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
  QRectF boundary_mm = resolution_scale_transform_.mapRect(machine_work_area_mm_);

  // Boundary check
  if (exceed_boundary_ == false && 
      (bitmap_dirty_area_.top() < boundary_mm.top() * canvas_mm_ratio_ || 
      bitmap_dirty_area_.bottom() > boundary_mm.bottom() * canvas_mm_ratio_ ||
      bitmap_dirty_area_.left() < boundary_mm.left() * canvas_mm_ratio_ || 
      bitmap_dirty_area_.right() > boundary_mm.right() * canvas_mm_ratio_)) {
    exceed_boundary_ = true;
  }
}

void ToolpathExporter::convertPath(const PathShape *path) {
  // qInfo() << "Convert Path" << path;
  // transformed_path: Express path in unit of dots (depends on document resolution settings)
  QPainterPath transformed_path = (path->transform() * global_transform_).map(path->path());
  QRectF boundary_mm = resolution_scale_transform_.mapRect(machine_work_area_mm_);

  // Boundary check
  if (exceed_boundary_ == false && 
      (transformed_path.boundingRect().top() < boundary_mm.top() * canvas_mm_ratio_ || 
      transformed_path.boundingRect().bottom() > boundary_mm.bottom() * canvas_mm_ratio_ ||
      transformed_path.boundingRect().left() < boundary_mm.left() * canvas_mm_ratio_ || 
      transformed_path.boundingRect().right() > boundary_mm.right() * canvas_mm_ratio_)) {
    exceed_boundary_ = true;
  }

  // Fill shape
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
  // Line shape
  if ((!path->isFilled() && current_layer_->type() == Layer::Type::Mixed) ||
      current_layer_->type() == Layer::Type::Line ||
      current_layer_->type() == Layer::Type::FillLine) {
    polygons_mutex_.lock();
    layer_polygons_.append(transformed_path.toSubpathPolygons());
    polygons_mutex_.unlock();
  }
}

void mergeQList(QList<QPolygonF> &Array, int front, int mid, int end){
  // 利用 QList 的constructor, 
  // 把array[front]~array[mid]放進 LeftSub[]
  // 把array[mid+1]~array[end]放進 RightSub[]
  QList<QPolygonF> LeftSub(Array.begin()+front, Array.begin()+mid+1),
                   RightSub(Array.begin()+mid+1, Array.begin()+end+1);

  LeftSub.insert(LeftSub.end(), QRectF(0,0,std::numeric_limits<double>::max(),1));      // 在LeftSub[]尾端加入值為 Max 的元素
  RightSub.insert(RightSub.end(), QRectF(0,0,std::numeric_limits<double>::max(),1));    // 在RightSub[]尾端加入值為 Max 的元素
 
  int idxLeft = 0, idxRight = 0;
  for (int i = front; i <= end; i++) {
    if (LeftSub[idxLeft].boundingRect().width() <= RightSub[idxRight].boundingRect().width() ) {
      Array[i] = LeftSub[idxLeft];
      idxLeft++;
    }
    else{
      Array[i] = RightSub[idxRight];
      idxRight++;
    }
  }
}

// Performance: O(n log(n))
void mergeSort(QList<QPolygonF> &array, int front, int end) {
                                          // front與end為矩陣範圍
  if (front < end) {                      // 表示目前的矩陣範圍是有效的
    int mid = (front+end)/2;              // mid即是將矩陣對半分的index
    mergeSort(array, front, mid);         // 繼續divide矩陣的前半段subarray
    mergeSort(array, mid+1, end);         // 繼續divide矩陣的後半段subarray
    mergeQList(array, front, mid, end);   // 將兩個subarray做比較, 並合併出排序後的矩陣
  }
}

// Performance: O(n^2)
void nestedSort(QList<QPolygonF> &array) {
  QList<QPolygonF> sort_result;
  for (const auto& polygon: array) {
    int insert_idx = sort_result.size();
    for (int idx = sort_result.size() - 1; idx >= 0; idx--) {
      if (sort_result[idx].boundingRect().contains(polygon.boundingRect())) {
        insert_idx = idx;
      }
    }
    sort_result.insert(insert_idx, polygon);
  }

  array = sort_result;
}

void ToolpathExporter::sortPolygons() {
  switch (sort_rule_)
  {
  case MergeSort:
    mergeSort(layer_polygons_, 0, layer_polygons_.size()-1);
    break;
  case NestedSort:
    nestedSort(layer_polygons_);
    break;
  case NoSort:
  default:
    break;
  }
}

/**
 * @brief Export the layer_polygons_ and layer_bitmap_ converted from objects on canvas 
 *        to generator
 * 
 */
void ToolpathExporter::outputLayerGcode() {
  outputLayerBitmapGcode();
  outputLayerPathGcode();
}

/**
 * @brief Export layer_polygons_ for non-filled geometry
 */
void ToolpathExporter::outputLayerPathGcode() {

  gen_->turnOnLaser(); // M3

  // NOTE: Should convert points from canvas unit to mm
  polygons_mutex_.lock();
  for (auto &poly : layer_polygons_) {
    if (poly.empty()) continue;

    QPointF next_point_mm = poly.first() / dpmm_;
    moveTo(next_point_mm,
           travel_speed_,
           0, 0);

    for (QPointF &point : poly) {
      next_point_mm = point / dpmm_;
      // Divide a long line into small segments
      if ( (next_point_mm - current_pos_mm_).manhattanLength() > 5 ) { // At most 5mm per segment
        int segments = std::max(2.0,
                           std::sqrt(std::pow(next_point_mm.x() - current_pos_mm_.x(), 2) +
                                std::pow(next_point_mm.y() - current_pos_mm_.y(), 2)) / 5 );
        QList<QPointF> interpolate_points;
        for (int i = 1; i <= segments; i++) {
          interpolate_points << (i * next_point_mm + (segments - i) * current_pos_mm_) / float(segments);
        }
        for (const QPointF &interpolate_point : interpolate_points) {
          moveTo(interpolate_point,
                 current_layer_->speed(),
                 current_layer_->power(),
                 0);
        }
      } else {
        moveTo(next_point_mm,
               current_layer_->speed(),
               current_layer_->power(),
               0);
      }

    }

    //gen_->turnOffLaser();
  }
  polygons_mutex_.unlock();
  // gen_->moveTo(gen_->x(), gen_->y(), current_layer_->speed(), 0, 0);
  gen_->turnOffLaser();
}

/**
 * @brief Export layer_bitmap_ for filled geometry and images
 */

bool depthMode = false;

void ToolpathExporter::outputLayerBitmapGcode() {
  if (bitmap_dirty_area_.width() == 0) return;
  // Get the image of entire layer
  QImage layer_image;
  if(with_image_) {
    if (!depthMode) {
      layer_image = layer_bitmap_.toImage()
                        .convertToFormat(QImage::Format_Mono, Qt::MonoOnly | Qt::DiffuseDither)
                        .convertToFormat(QImage::Format_Grayscale8);
    } else {
      layer_image = layer_bitmap_.toImage()
                        .convertToFormat(QImage::Format_Grayscale8);
    }
  } else {
    layer_image = layer_bitmap_.toImage()
                      .convertToFormat(QImage::Format_Grayscale8);
  }

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
         travel_speed_,
         0, 0);

  gen_->useRelativePositioning();

  // Start raster
  if(is_high_speed_) {
    rasterBitmapHighSpeed(layer_image, bbox, ScanDirectionMode::kBidirectionMode, padding_mm);
  }
  else {
    if (!depthMode) {
      rasterBitmap(layer_image, bbox, ScanDirectionMode::kBidirectionMode, padding_mm);
    } else {
      rasterBitmapDepthMode(layer_image, bbox, ScanDirectionMode::kBidirectionMode, padding_mm);
    }
  }

  gen_->useAbsolutePositioning();

  gen_->turnOffLaser();
}

/**
 *
 * @param layer_image the (scaled) bitmap of entire layer
 * @param bbox bounding box of dirty area
 *             NOTE: the bbox has already been scaled by dpmm
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
  // NOTE: Use bbox.left() + bbox.width() instead of bbox.right() 
  //       since the latter is the left position of the last pixel instead of the right boundary of the work area
  for (int y = bbox.top(); y <= bbox.bottom(); y += 1) {
    raster_lines.push_back(QLine{bbox.left(), y,
                                 bbox.left() + bbox.width(), y});
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
         travel_speed_,
         0, 0);

  int idx = 0;
  while (true) {
    if (t_step * idx >= 1) {
      moveTo(path.p2() / dpmm_,
             current_layer_->speed(),
             is_emitting ? current_layer_->power() : 0,
             is_emitting ? current_layer_->xBacklash() : 0);
      break;
    }
    if (int(idx/32) >= data.size()) {
      if (is_emitting) {
        moveTo(path.pointAt(t_step * idx) / dpmm_,
               current_layer_->speed(),
               current_layer_->power(),
               current_layer_->xBacklash());
      }
      moveTo(path.p2() / dpmm_,
             current_layer_->speed(),
             0, 0);
      break;
    }

    if (is_emitting != data[int(idx/32)][31 - (idx % 32)]) {
      moveTo(path.pointAt(t_step * idx) / dpmm_,
             current_layer_->speed(),
             is_emitting ? current_layer_->power() : 0,
             is_emitting ? current_layer_->xBacklash() : 0);
      is_emitting = !is_emitting;
    }
    idx++;
  }

  return true;
}

bool ToolpathExporter::rasterBitmapDepthMode(const QImage &layer_image,
    QRect bbox, 
    ScanDirectionMode direction_mode, 
    qreal padding_mm) {

  const int white_pixel = 255;
  bool reverse_raster_dir = false;

  // Prepare raster line paths
  QList<QLine> raster_lines;
  for (int y = bbox.top(); y <= bbox.bottom(); y += 1) {
    raster_lines.push_back(QLine{bbox.left(), y,
                                 bbox.left() + bbox.width(), y});
  }
  qInfo() << "bbox: " << bbox;
  qInfo() << "# of raster line: " << raster_lines.size();

  // Iterate through each raster line
  for (const auto &raster_line: raster_lines) {
    if (raster_line.isNull()) {
      continue;
    }
    
    std::vector<std::array<unsigned char, 32>> grayscale_data_list;
    
    // Determine start and end points based on raster direction
    const QPoint initial_pos = reverse_raster_dir ? raster_line.p2() : raster_line.p1();
    const QPoint end_pos = reverse_raster_dir ? raster_line.p1() : raster_line.p2();
    const QLineF path{initial_pos, end_pos};
    
    // Calculate step size for scanning
    const qreal t_step = 1 / path.length();
    qreal current_t_sample = t_step / 2;
    QPointF current_pos_sample = path.pointAt(current_t_sample);

    // Scan an entire line of bitmap
    std::array<unsigned char, 32> grayscale_array;
    uint32_t array_idx = 0;
    bool blank_line = true;

    while (1) {
      // Get grayscale value of current pixel
      const uchar *data_ptr = layer_image.constScanLine(current_pos_sample.y());
      unsigned char dot_grayscale = data_ptr[int(current_pos_sample.x())];
      
      // Update blank_line flag if a non-white pixel is found
      if (dot_grayscale < white_pixel && blank_line) {
        blank_line = false;
      }
      
      // Store grayscale value
      grayscale_array[array_idx] = dot_grayscale;

      // If grayscale_array is full, add it to grayscale_data_list and reset
      if (array_idx == 31) {
        grayscale_data_list.push_back(grayscale_array);
        array_idx = 0;
      } else {
        array_idx++;
      }

      // Move to next sample point
      current_t_sample += t_step;
      if (current_t_sample >= 0.99999) {
        if (array_idx != 0) { // Append the remaining partial array
          grayscale_data_list.push_back(grayscale_array);
        }
        break;
      }
      current_pos_sample = path.pointAt(current_t_sample);
    }

    // Skip processing if the line is entirely blank
    if (blank_line) {
      continue;
    }

    // Trim unnecessary white space and adjust for padding
    auto [trimmed_grayscale_array, first_pos_idx, last_pos_idx] =
    adjustPrefixSuffixZero(grayscale_data_list, padding_mm * dpmm_);
    
    // Calculate the actual line segment to be rasterized
    QLineF dirty_line_segment {
            path.pointAt(first_pos_idx * t_step),
            path.pointAt(qMin((last_pos_idx + 1) * t_step, 1.0))
    };

    // Rasterize the line
    rasterLine(dirty_line_segment, trimmed_grayscale_array);

    // Switch scan direction if bidirectional mode is enabled
    if (direction_mode == ScanDirectionMode::kBidirectionMode) {
      reverse_raster_dir = !reverse_raster_dir;
    }
  }

  return true;
}

int ToolpathExporter::calculatePWMPower(unsigned char grayscale) {
  // Implement your PWM calculation logic here
  // For example, you might want to invert the grayscale value and scale it to your PWM range
  return static_cast<int>((255 - grayscale) * current_layer_->power() / 255);
}

bool ToolpathExporter::rasterLine(const QLineF& path, const std::vector<std::array<unsigned char, 32>>& data) {
  const qreal t_step = 1 / path.length();
  
  // Move to the start of the line
  moveTo(path.p1() / dpmm_,
         travel_speed_,
         0, 0);

  int idx = 0;
  while (true) {
    // Check if we've reached the end of the line
    if (t_step * idx >= 1) {
      moveTo(path.p2() / dpmm_,
             current_layer_->speed(),
             0, 0);
      break;
    }
    
    // Check if we've processed all data
    if (int(idx/32) >= data.size()) {
      moveTo(path.p2() / dpmm_,
             current_layer_->speed(),
             0, 0);
      break;
    }

    // Get grayscale value and calculate PWM power
    unsigned char grayscale = data[int(idx/32)][idx % 32];
    int pwm_power = calculatePWMPower(grayscale);

    // Move to the point with calculated PWM power
    moveTo(path.pointAt(t_step * idx) / dpmm_,
           current_layer_->speed(),
           pwm_power,
           current_layer_->xBacklash());

    idx++;
  }

  return true;
}

/**
 * @brief Export
 * @param layer_image  (scaled) image of an entire layer on canvas
 * @param bbox         the (scaled) bounding box for the raster motion (shouldn't be larger than layer_image)
 *                     NOTE: the bbox has already been scaled by dpmm
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
  // NOTE: Use bbox.left() + bbox.width() instead of bbox.right() 
  //       since the latter is the left position of the last pixel instead of the right boundary of the work area
  for (int y = bbox.top(); y <= bbox.bottom(); y += 1) {
    raster_lines.push_back(QLine{bbox.left(), y,
                                  bbox.left() + bbox.width(), y});
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
         travel_speed_,
         0, 0);

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
         current_layer_->power(),
         current_layer_->xBacklash());

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

inline void ToolpathExporter::moveTo(QPointF&& dest, double speed, double power, double x_backlash) {
  gen_->moveTo(dest.x(), dest.y(), speed, power, x_backlash);
  current_pos_mm_ = dest;
}

inline void ToolpathExporter::moveTo(const QPointF& dest, double speed, double power, double x_backlash) {
  gen_->moveTo(dest.x(), dest.y(), speed, power, x_backlash);
  current_pos_mm_ = dest;
}

void ToolpathExporter::handleCancel() {
  this->cancelled_ = true;
}