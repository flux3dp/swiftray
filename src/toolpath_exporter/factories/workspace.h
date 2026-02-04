#pragma once

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QRectF>

class Workspace {
 private:
  bool bitmap_ready = false;
  QRect clip_rect = QRect();
  QRectF bitmap_dirty_area = QRectF();
  bool updated = false;
  int canvas_width = 1;
  int canvas_height = 1;

  void setup_canvas();

 public:
  QImage bitmap;

  Workspace() = default;

  void set_size(int width, int height);
  void set_clip_rect(QRect& clip_rect);
  void add_image(QImage& img, QRectF& bbox);
  void add_filled_path(QPainterPath& path, QRectF& bbox);
  QRectF get_dirty_area();
  void invalidate();
};
