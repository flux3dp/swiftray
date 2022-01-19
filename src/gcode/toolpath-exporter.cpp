#include <gcode/toolpath-exporter.h>
#include <QElapsedTimer>
#include <QDebug>
#include <iostream>
#include <cmath>
#include <boost/range/irange.hpp>

ToolpathExporter::ToolpathExporter(BaseGenerator *generator) noexcept {
  global_transform_ = QTransform();
  gen_ = generator;
  dpmm_ = 10;
  travel_speed_ = 100;
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

  for (auto &layer : layers) {
    if (!layer->isVisible()) {
      continue;
    }
    qInfo() << "[Export] Output layer: " << layer->name();
    for (int i = 0; i < layer->repeat(); i++) {
      convertLayer(layer);
    }
  }

  // Post cmds
  gen_->home();
  qInfo() << "[Export] Took " << t.elapsed() << " seconds";
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
    layer_painter_->drawPixmap(0, 0, QPixmap::fromImage(bmp->image().convertToFormat(QImage::Format_Mono, Qt::MonoOnly | Qt::DiffuseDither)));
  } else { // binarize mode
    layer_painter_->drawPixmap(0, 0, QPixmap::fromImage( imageBinarize(bmp->image(), bmp->thrsh_brightness()) ));
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

  gen_->turnOnLaser(); // M3/M4

  // NOTE: Should convert points from canvas unit to mm
  for (auto &poly : layer_polygons_) {
    if (poly.empty()) continue;

    QPointF next_point_mm = poly.first() / canvas_mm_ratio_;
    moveTo(next_point_mm,
           current_layer_->speed(),
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
  bool reverse = false;
  QImage image = layer_bitmap_.toImage().convertToFormat(QImage::Format_Grayscale8);

  qreal mm_per_dot = 1.0 / dpmm_;            // unit size of engraving dot (segment)
  qreal mm_per_pixel = canvas_size_.height() / canvas_mm_ratio_ / image.height();
  // layer_bitmap_ image size = width & height of the document

  // NOTE: Should convert points from canvas unit to mm
  qreal real_x_left = bitmap_dirty_area_.left() / canvas_mm_ratio_;   // in unit of mm (real world coordinate)
  qreal real_x_right = bitmap_dirty_area_.right() / canvas_mm_ratio_;   // in unit of mm (real world coordinate)
  qreal real_y_top = bitmap_dirty_area_.top() / canvas_mm_ratio_;   // in unit of mm (real world coordinate)
  qreal real_y_bottom = bitmap_dirty_area_.bottom() / canvas_mm_ratio_; // in unit of mm (real world coordinate)

  gen_->turnOnLaser(); // M3/M4

  // rapid move to the start position
  moveTo(QPointF{real_x_left, real_y_top},
         current_layer_->speed(),
         0);

  gen_->useRelativePositioning();
  // Handle first row
  qreal real_y_current_pos = real_y_top; // NOTE: y top < y bottom (in canvas coordinate)
  while (real_y_current_pos < real_y_bottom) {
    // scan a row
    rasterBitmapRow(image.scanLine(int(real_y_current_pos / mm_per_pixel)), real_y_current_pos, image.width(), reverse, QPointF());
    // enter the next row (y axis)
    real_y_current_pos += mm_per_dot;
    if (real_y_current_pos > real_y_bottom) {
      break;
    }
    if (reverse) {
      moveTo(QPointF{real_x_left, real_y_current_pos},
             current_layer_->speed(),
             0);
    } else {
      moveTo(QPointF{real_x_right, real_y_current_pos},
             current_layer_->speed(),
             0);
    }
    reverse = !reverse;
  }
  gen_->useAbsolutePositioning();

  gen_->turnOffLaser();
}

/**
 * @brief
 * @param data
 * @param row_pixel_cnt Number of bitmap pixels in the row (Not the number of engraving dot)
 * @param reverse true: left to right; false: right to left
 * @param offset
 * @return
 */
bool ToolpathExporter::rasterBitmapRow(unsigned char *data, qreal real_y_pos, int row_pixel_cnt,
                                       bool reverse, QPointF offset) {
  qreal mm_per_dot = 1.0 / dpmm_;            // unit size of engraving dot (segment)
  qreal mm_per_pixel = canvas_size_.width() / canvas_mm_ratio_ / row_pixel_cnt;

  qreal real_x_left_boundary = bitmap_dirty_area_.left() / canvas_mm_ratio_;   // in unit of mm (real world coordinate)
  qreal real_x_right_boundary = bitmap_dirty_area_.right() / canvas_mm_ratio_; // in unit of mm (real world coordinate)

  bool laser_should_be_on = false; // whether should be on (in the last pixel but haven't been added to gcode)

  qreal real_x_current_pos = reverse ? real_x_right_boundary : real_x_left_boundary;

  // 1. Handle the first pixel in the row (initial state)
  float p = data[int(real_x_current_pos / mm_per_pixel)] > 127 ? 255 : 0;
  float laser_pwm = (255.0 - p) / 255.0; // black (0) -> 100% pwm; white (255) -> 0% pwm
  if (laser_pwm < 0.01) laser_pwm = 0;
  if (laser_pwm != 0) {
    laser_should_be_on = true;
  }
  // 2. Iterate until the boundary
  while ( (reverse && real_x_current_pos > real_x_left_boundary) ||
          (!reverse && real_x_current_pos < real_x_right_boundary) )
  {
    if (reverse) {
      real_x_current_pos -= mm_per_dot;
    } else {
      real_x_current_pos += mm_per_dot;
    }
    if (reverse && real_x_current_pos <= real_x_left_boundary) {
      // End of reversed Line
      real_x_current_pos = real_x_left_boundary;
      if (laser_should_be_on) { // on -> off (add a dark segment until the last pixel)
        moveTo(QPointF{real_x_current_pos, real_y_pos},
               current_layer_->speed(),
               current_layer_->power());
      } else if (!laser_should_be_on) { // off -> on (add an white segment until the last pixel)
        moveTo(QPointF{real_x_current_pos, real_y_pos},
               current_layer_->speed(),
               0);
      }
      break;
    } else if (!reverse && real_x_current_pos >= real_x_right_boundary) {
      // End of forward Line
      real_x_current_pos = real_x_right_boundary;
      if (laser_should_be_on) { // on -> off (add a dark segment until the last pixel)
        moveTo(QPointF{real_x_current_pos, real_y_pos},
               current_layer_->speed(),
               current_layer_->power());
      } else if (!laser_should_be_on) { // off -> on (add an white segment until the last pixel)
        moveTo(QPointF{real_x_current_pos, real_y_pos},
               current_layer_->speed(),
               0);
      }
      break;
    }

    // Dots (segments) in the middle
    float p = data[int(real_x_current_pos / mm_per_pixel)] > 127 ? 255 : 0;
    float laser_pwm = (255.0 - p) / 255.0;
    if (laser_pwm < 0.01) laser_pwm = 0; // black (0) -> 100% pwm; white (255) -> 0% pwm
    if (laser_pwm == 0 && laser_should_be_on) { // on -> off (add a dark segment until the last pixel)
      moveTo(QPointF{real_x_current_pos, real_y_pos},
             current_layer_->speed(),
             current_layer_->power());
      laser_should_be_on = false;
    } else if (laser_pwm != 0 && !laser_should_be_on) { // off -> on (add an white segment until the last pixel)
      moveTo(QPointF{real_x_current_pos, real_y_pos},
             current_layer_->speed(),
             0);
      laser_should_be_on = true;
    }

  }

  return 0;
}

bool ToolpathExporter::rasterBitmapRowHighSpeed(unsigned char *data, float global_coord_y, bool reverse,
                                                QPointF offset) {
  /*
  float pixel_size = 1.0 / dpmm_;
  int max_x = layer_bitmap_.width();
  max_x -= 3;
  int init_blank_pixels = int(5 / pixel_size);
  int head = -1, tail = -1;
  bool pixels[max_x];

  for (int x = 0; x < max_x; x++) {
    pixels[x] = data[x] < 127;

    if (!pixels[x]) {
      if (head < 0) head = x;

      tail = x;
    }
  }

  if (head < 0) return false;

  int pixels32 = 0, pixels32_mask = 31, pixels_count;
  float start_x, end_x;

  if (!reverse) {
    head = std::max(0, head - init_blank_pixels);
    tail = std::min(max_x - 31, tail + init_blank_pixels);
    start_x = pixel_size * head - offset.x();
    end_x = pixel_size * (tail + 1) - offset.x();
    pixels_count = tail - head + 1;
  } else {
    head = std::min(max_x - 31, head + init_blank_pixels);
    tail = std::max(0, tail - init_blank_pixels);
    start_x = pixel_size * (head + 1) - offset.x();
    end_x = pixel_size * tail - offset.x();
    pixels_count = head - tail + 1;
  }

  gen_->moveTo(start_x, global_coord_y, travel_speed_, 0);
  gen_->setSpeed(current_layer_->speed());
  gen_->beginHighSpeedRastering(pixels_count);
  auto x_range = reverse ? boost::irange(head, tail - 1, -1) : boost::irange(head, tail + 1, 1);

  for (int x : x_range) {
    if (pixels[x]) {
      pixels32 |= 1 << pixels32_mask;
    }

    pixels32_mask--;

    if (pixels32_mask < 0) {
      pixels32_mask = 31;
      gen_->pushRasteringPixels32(pixels32);
      pixels32 = 0;
    }
  }

  if (pixels32) gen_->pushRasteringPixels32(pixels32);

  gen_->endHighSpeedRastering();
  gen_->moveToX(end_x);

  return true;
  */

  return false;
}

QImage ToolpathExporter::imageBinarize(QImage src, int threshold) {
  QImage result_img{src.width(), src.height(), QImage::Format_Grayscale8};
  bool apply_alpha = src.hasAlphaChannel();

  for (int y = 0; y < src.height(); ++y) {
    for (int x = 0; x < src.width(); ++x) {
      int grayscale_val = qGray(src.pixel(x, y));
      if (apply_alpha) {
        grayscale_val = 255 - (255 - grayscale_val) * qAlpha(src.pixel(x, y)) / 255;
      }
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
