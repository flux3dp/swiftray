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

  gen_->turnOnLaser(); // M3/M4

  Q_ASSERT_X(!layers.empty(), "ToolpathExporter", "Must input at least one layer");
  // Generate bitmap canvas
  layer_bitmap_ = QPixmap(QSize((layers.at(0)->document()).width(), (layers.at(0)->document()).height()));
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
  qInfo() << "[Export] Took " << t.elapsed();
}

void ToolpathExporter::convertLayer(const LayerPtr &layer) {
  // Reset context states for the layer
  // TODO (Use layer_painter to manage transform over different sub objects)
  global_transform_ = QTransform();
  layer_polygons_.clear();
  layer_painter_ = make_unique<QPainter>(&layer_bitmap_);
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

void ToolpathExporter::convertBitmap(const BitmapShape *bmp) {
  QTransform transform = QTransform().scale(dpmm_ / 10.0, dpmm_ / 10.0) * bmp->transform() * global_transform_;
  layer_painter_->save();
  layer_painter_->setTransform(transform, false);
  layer_painter_->drawPixmap(0, 0, *bmp->pixmap());
  layer_painter_->restore();
  bitmap_dirty_area_ = bitmap_dirty_area_.united(bmp->boundingRect());
}

void ToolpathExporter::convertPath(const PathShape *path) {
  // qInfo() << "Convert Path" << path;
  QPainterPath transformed_path = (path->transform() * global_transform_).map(path->path());;
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

void ToolpathExporter::outputLayerPathGcode() {
  QPointF current_pos;
  for (auto &poly : layer_polygons_) {
    if (poly.empty()) continue;


    current_pos = poly.first();
    gen_->moveTo(current_pos.x() / 10.0, current_pos.y() / 10.0, travel_speed_, 0);

    for (QPointF &point : poly) {
      if ((point - current_pos).manhattanLength() > 50) { //5mm
        int segments = max(2.0, sqrt(pow(point.x() - current_pos.x(), 2) +
                                     pow(point.y() - current_pos.y(), 2)) / 50);
        for (int i = 1; i <= segments; i++) {
          QPointF insert_point = (i * point + (segments - i) * current_pos) / float(segments);
          gen_->moveTo(insert_point.x() / 10, insert_point.y() / 10, current_layer_->speed(), current_layer_->power());
        }
      } else {
        gen_->moveTo(point.x() / 10.0, point.y() / 10.0, current_layer_->speed(), current_layer_->power());
      }
      current_pos = point;
    }

    //gen_->turnOffLaser();
  }
  // gen_->moveTo(gen_->x(), gen_->y(), current_layer_->speed(), 0);
  gen_->turnOffLaser();
}

void ToolpathExporter::outputLayerBitmapGcode() {
  if (bitmap_dirty_area_.width() == 0) return;
  bool reverse = false;
  QImage image = layer_bitmap_.toImage().convertToFormat(QImage::Format_Grayscale8);

  // rapid move to the start position
  qInfo() << bitmap_dirty_area_;
  gen_->moveTo(bitmap_dirty_area_.left() / dpmm_,bitmap_dirty_area_.top() / dpmm_,
               current_layer_->speed(), 0);

  gen_->useRelativePositioning();
  for (int y = bitmap_dirty_area_.top(); y <= bitmap_dirty_area_.bottom(); y++) {
    rasterBitmapRow(image.scanLine(y), reverse, QPointF());
    // move to the next row (relative move)
    gen_->moveTo(0, 1 / dpmm_,current_layer_->speed(), 0);
    reverse = !reverse;
  }

  gen_->useAbsolutePositioning();
  gen_->turnOffLaser();
}


bool ToolpathExporter::rasterBitmapRowHighSpeed(unsigned char *data, float global_coord_y, bool reverse,
                                                QPointF offset) {
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
    head = max(0, head - init_blank_pixels);
    tail = min(max_x - 31, tail + init_blank_pixels);
    start_x = pixel_size * head - offset.x();
    end_x = pixel_size * (tail + 1) - offset.x();
    pixels_count = tail - head + 1;
  } else {
    head = min(max_x - 31, head + init_blank_pixels);
    tail = max(0, tail - init_blank_pixels);
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
}

bool ToolpathExporter::rasterBitmapRow(unsigned char *data, bool reverse,
                                       QPointF offset) {
  float pixel_size = 1.0 / dpmm_; // mm per pixel (dot)
  int x_max = bitmap_dirty_area_.right();
  int x_min = bitmap_dirty_area_.left();
  // TODO (Pass machine size as argument);
  int MACHINE_MAX_X = 300;
  bool y_moved = false;

  bool laser_should_be_on = false; // whether should be on (in the last pixel but haven't been added to gcode)
  int x_total = x_max - x_min;
  int accumulated_x_pixel = 0;
  for (int x = 0; x < x_total; x++) {
    // proceed one pixel size per loop
    accumulated_x_pixel++;
    float p = data[reverse ? (x_max - x) : (x + x_min)] > 127 ? 255 : 0;
    float laser_pwm = (255.0 - p) / 255.0;

    if (laser_pwm < 0.01) laser_pwm = 0;

    // special case (first pixel)
    if (x == 0 && laser_pwm != 0) {
      laser_should_be_on = true;
    }

    //float global_coord_x = pixel_size * x - offset.x();
    // Check black/white boundary
    if (laser_pwm == 0) {
      if (laser_should_be_on) { // on -> off (add a dark line to the last pixel)
        gen_->moveTo((reverse ? -accumulated_x_pixel*pixel_size : accumulated_x_pixel*pixel_size),0,
                     current_layer_->speed(),current_layer_->power());
        accumulated_x_pixel = 0;
        laser_should_be_on = false;
      }
    } else {
      if (!laser_should_be_on) { // off -> on (add a empty line to the last pixel)
        gen_->moveTo((reverse ? -accumulated_x_pixel*pixel_size : accumulated_x_pixel*pixel_size), 0,
                     current_layer_->speed(), 0);
        accumulated_x_pixel = 0;
        laser_should_be_on = true;
      }
    }

    // finish the line
    if (x == x_total - 1 && accumulated_x_pixel != 0) {
      if (laser_should_be_on) { // off -> on (add a empty line to the last pixel)
        gen_->moveTo((reverse ? -accumulated_x_pixel * pixel_size : accumulated_x_pixel * pixel_size), 0,
                     current_layer_->speed(), current_layer_->power());
      } else {
        gen_->moveTo((reverse ? -accumulated_x_pixel * pixel_size : accumulated_x_pixel * pixel_size), 0,
                     current_layer_->speed(), 0);

      }
    }
  }

  return 0;
}
