#pragma once

#include <QImage>
#include <QPainter>
#include <QPainterPath>
#include <QRect>
#include <QRectF>

class Workspace {
 private:
  QImage bitmap;
  QImage bitmap_scaled; // For high dpi laser layers with varied dpmm and dpmm_x
  bool bitmap_ready = false;
  QRect clip_rect = QRect();
  QRectF bitmap_dirty_area = QRectF();
  QRectF bitmap_dirty_area_scaled = QRectF();
  bool updated = false;
  int canvas_width = 1;
  int canvas_height = 1;
  double scale = 1.0;

  void setup_canvas();

 public:
  Workspace() = default;

  void set_size(int width, int height, double dpmm_x = 1.0, double dpmm_y = 1.0);
  void set_clip_rect(QRect& clip_rect);
  void add_image(QImage& img, QRectF& bbox);
  void add_filled_path(QPainterPath& path, QRectF& bbox);
  QRectF get_dirty_area();
  QImage* get_bitmap();
  void invalidate();
};
