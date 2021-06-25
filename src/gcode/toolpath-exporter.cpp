#include <gcode/toolpath-exporter.h>
#include <QElapsedTimer>
#include <QDebug>
#include <iostream>
#include <boost/range/irange.hpp>

ToolpathExporter::ToolpathExporter(BaseGenerator *generator) noexcept {
  global_transform_ = QTransform();
  gen_ = generator;
  dpmm_ = 10;
  travel_speed_ = 100;
}

void ToolpathExporter::convertStack(const QList<LayerPtr> &layers) {
  QElapsedTimer t;
  t.start();
  gen_->turnOffLaser();
  gen_->home();
  gen_->useAbsolutePositioning();

  // Generate bitmap canvas
  layer_bitmap_ = QPixmap(QSize(300 * dpmm_, 200 * dpmm_));
  layer_bitmap_.fill(Qt::white);
  bitmap_dirty_area_ = QRectF();

  for (auto &layer : layers) {
    if (!layer->isVisible()) return;

    qInfo() << "[Export] Output layer: " << layer->name();
    for (int i = 0; i < layer->repeat(); i++) {
      convertLayer(layer);
    }
  }
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
  gen_->setLaserPower(current_layer_->power());

  for (auto &poly : layer_polygons_) {
    if (poly.empty()) continue;

    current_pos = poly.first();
    gen_->turnOffLaser();
    gen_->moveTo(current_pos.x() / 10.0, current_pos.y() / 10.0, travel_speed_);
    gen_->turnOnLaser();

    for (QPointF &point : poly) {
      gen_->moveTo(point.x() / 10.0, point.y() / 10.0, current_layer_->speed());
    }

    gen_->turnOffLaser();
  }
}

void ToolpathExporter::outputLayerBitmapGcode() {
  if (bitmap_dirty_area_.width() == 0) return;
  bool reverse = false;
  QImage image = layer_bitmap_.toImage().convertToFormat(QImage::Format_Grayscale8);

  for (int y = bitmap_dirty_area_.top(); y <= bitmap_dirty_area_.bottom(); y++) {
    rasterBitmapRow(image.scanLine(y), (float) y / dpmm_, reverse, QPointF());
    reverse = !reverse;
  }

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

  gen_->moveTo(start_x, global_coord_y, travel_speed_);
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

bool ToolpathExporter::rasterBitmapRow(unsigned char *data, float global_coord_y, bool reverse,
                                       QPointF offset) {
  float pixel_size = 1.0 / dpmm_;
  int x_max = bitmap_dirty_area_.right();
  int x_min = bitmap_dirty_area_.left();
  // TODO (Pass machine size as argument);
  int MACHINE_MAX_X = 300;
  bool x_moved = false;
  bool y_moved = false;
  auto x_range = reverse ? boost::irange(x_max - 1, x_min, -1) : boost::irange(x_min, x_max - 1, 1);

  gen_->setLaserPower(0);

  for (int x : x_range) {
    float p = data[x] > 127 ? 255 : 0;
    float laser_pwm = (255.0 - p) / 255.0;

    if (laser_pwm < 0.01) laser_pwm = 0;

    float global_coord_x = pixel_size * (reverse ? x + 1 : x) - offset.x();

    if (laser_pwm == 0) {
      if (laser_pwm != gen_->power()) {
        gen_->moveToX(global_coord_x);
        gen_->setLaserPower(0);
        x_moved = true;
      } else {
        continue;
      }
    } else {
      if (laser_pwm != gen_->power()) {
        if (!y_moved) {
          gen_->turnOffLaser();
          gen_->moveTo(global_coord_x, global_coord_y, current_layer_->speed());
          y_moved = true;
        }

        gen_->moveToX(global_coord_x);
        gen_->setLaserPower(laser_pwm);
        x_moved = true;
      }
    }
  }

  if (gen_->power() > 0) {
    // If the last pixel is still powered on
    gen_->moveToX(*(x_range.end() - 1) * pixel_size - offset.x());
    x_moved = true;
  }

  if (x_moved) {
    float buffer_x = reverse ? std::max(0.0, gen_->x() - 25.0) :
                     std::min(0.0 + MACHINE_MAX_X, gen_->x() + 25.0);
    gen_->turnOffLaser();
    gen_->moveToX(buffer_x);
  }

  return x_moved;
}
