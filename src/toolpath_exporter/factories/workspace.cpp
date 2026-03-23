#include "workspace.h"
#include <QDebug>

void Workspace::setup_canvas() {
  if (bitmap_ready) return;
  bitmap_scaled = QImage();
  if (bitmap.width() >= canvas_width && bitmap.height() >= canvas_height) {
    // Clear old content
    QPainter painter(&bitmap);
    painter.fillRect(bitmap_dirty_area, Qt::white);
  } else {
    // Create new bitmap
    bitmap = QImage(canvas_width, canvas_height, QImage::Format_Grayscale8);
    bitmap.fill(Qt::white);
  }
  bitmap_dirty_area = QRectF();
  bitmap_dirty_area_scaled = QRectF();
  bitmap_ready = true;
  updated = false;
}

void Workspace::set_size(int width, int height, double dpmm_x, double dpmm_y) {
  scale = dpmm_x / dpmm_y;
  canvas_width = fmax(width / scale, 1);
  canvas_height = fmax(height, 1);
  bitmap_ready = false;
}

void Workspace::set_clip_rect(QRect& clip_rect) {
  this->clip_rect = QRect(clip_rect.x() / scale, clip_rect.y(),
                          clip_rect.width() / scale, clip_rect.height());
}

void Workspace::add_image(QImage& img, QRectF& bbox) {
  setup_canvas();
  QPainter painter(&bitmap);
  painter.setClipRect(clip_rect);
  painter.drawImage(bbox.topLeft(), img);
  bitmap_dirty_area = bitmap_dirty_area.united(bbox);
  updated = true;
}

void Workspace::add_filled_path(QPainterPath& path, QRectF& bbox) {
  setup_canvas();
  QPainter painter(&bitmap);
  painter.setClipRect(clip_rect);
  painter.setPen(Qt::NoPen);
  painter.setBrush(Qt::black);
  painter.drawPath(path);
  painter.setBrush(Qt::NoBrush);
  bitmap_dirty_area = bitmap_dirty_area.united(bbox);
  updated = true;
}

QRectF Workspace::get_dirty_area() {
  if (updated) {
    bitmap_dirty_area = bitmap_dirty_area.intersected(clip_rect);
    bitmap_dirty_area_scaled = QRectF(bitmap_dirty_area.x() * scale, bitmap_dirty_area.y(),
                                      bitmap_dirty_area.width() * scale, bitmap_dirty_area.height());
    updated = false;
  }
  return bitmap_dirty_area_scaled;
}

QImage* Workspace::get_bitmap() {
  if (scale != 1) {
    if (bitmap_scaled.isNull()) {
      bitmap_scaled = bitmap.scaled(canvas_width * scale, canvas_height);
    }
    return &bitmap_scaled;
  }
  return &bitmap;
}

void Workspace::invalidate() {
  bitmap_ready = false;
}
