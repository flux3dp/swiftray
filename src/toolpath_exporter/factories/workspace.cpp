#include "workspace.h"
#include <QDebug>

void Workspace::setup_canvas() {
  if (bitmap_ready) return;
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
  bitmap_ready = true;
  updated = false;
}

void Workspace::set_size(int width, int height) {
  canvas_width = fmax(width, 1);
  canvas_height = fmax(height, 1);
  bitmap_ready = false;
}

void Workspace::set_clip_rect(QRect& clip_rect) {
  this->clip_rect = clip_rect;
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
    updated = false;
  }
  return bitmap_dirty_area;
}

QImage* Workspace::get_bitmap() {
  return &bitmap;
}

void Workspace::invalidate() {
  bitmap_ready = false;
}
