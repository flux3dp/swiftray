#include "laser-raster-galvo.h"

#include <QDebug>

#include "toolpath_exporter/generators/galvo-list.h"

LaserRasterGalvoFactory::LaserRasterGalvoFactory(
    const FactoryKwargs& kwargs) noexcept
    : BaseFactory(kwargs) {
  qInfo() << "LaserRasterGalvoFactory created";
}

void LaserRasterGalvoFactory::add_bitmap(const QImage& gray,
                                         const QRectF& bbox_px,
                                         bool dots) {
  if (gray.isNull() || bbox_px.isEmpty()) {
    return;
  }
  Entry entry;
  entry.gray = gray.format() == QImage::Format_Grayscale8
                   ? gray
                   : gray.convertToFormat(QImage::Format_Grayscale8);
  entry.bbox_px = bbox_px;
  entry.dots = dots;
  bitmaps_.append(entry);
}

QRectF LaserRasterGalvoFactory::get_bounds_mm() const {
  QRectF box;
  for (const Entry& entry : bitmaps_) {
    const QRectF mm(entry.bbox_px.left() / pixel_per_mm_x,
                    entry.bbox_px.top() / pixel_per_mm,
                    entry.bbox_px.width() / pixel_per_mm_x,
                    entry.bbox_px.height() / pixel_per_mm);
    box = box.united(mm);
  }
  return box.isEmpty() ? box : box.translated(-offset);
}

void LaserRasterGalvoFactory::setPwm(float pwm) {
  if (current_pwm_ == pwm) {
    return;
  }
  current_pwm_ = pwm;
  proc->set_toolhead_pwm(pwm);
}

void LaserRasterGalvoFactory::emitRow(const Entry& entry,
                                      int row,
                                      int threshold,
                                      bool reversed,
                                      float speed) {
  const uchar* line = entry.gray.constScanLine(row);
  const int width = entry.gray.width();
  // Sample at pixel centres, the way the gcode raster does, so a run covers the
  // dots it is made of rather than stopping half a pixel short of the last one.
  const double top = entry.bbox_px.top();
  const double left = entry.bbox_px.left();
  const double y = (top + row + 0.5) / pixel_per_mm - offset.y();
  auto x_at = [&](int column) {
    return (left + column) / pixel_per_mm_x - offset.x();
  };

  if (entry.dots) {
    // One dot per dark pixel; the writer turns each of these marks into a jump
    // and a LASER_ON, since a dotting time is set for a dithered image.
    for (int i = 0; i < width; i++) {
      const int column = reversed ? width - 1 - i : i;
      if (line[column] >= threshold) {
        continue;
      }
      setPwm(100);
      proc->moveto(
          NamedArgs().rf(speed).rx(x_at(column) + 0.5 / pixel_per_mm_x).ry(y));
    }
    return;
  }

  // Runs of dark pixels, each one marked segment.
  int run_start = -1;
  for (int i = 0; i <= width; i++) {
    const bool on = i < width && line[i] < threshold;
    if (on && run_start < 0) {
      run_start = i;
    } else if (!on && run_start >= 0) {
      // +1 on the end: the run covers the whole of its last pixel.
      double from = x_at(run_start);
      double to = x_at(i);
      if (reversed) {
        std::swap(from, to);
      }
      setPwm(0);
      proc->moveto(NamedArgs().rx(from).ry(y).set_is_travel());
      setPwm(100);
      proc->moveto(NamedArgs().rf(speed).rx(to).ry(y));
      run_start = -1;
    }
  }
}

void LaserRasterGalvoFactory::generate_task_code(float speed, int threshold) {
  if (bitmaps_.isEmpty()) {
    return;
  }
  current_pwm_ = 0;
  for (const Entry& entry : bitmaps_) {
    if (cancelled) {
      return;
    }
    // Dotting is a property of the image, not the layer: a dithered one is dots
    // and a binarised one is runs, and a layer can hold both.
    if (proc->galvo()) {
      proc->galvo()->params().dotting_time_us =
          entry.dots ? dotting_time_us_ : 0;
    }
    const int height = entry.gray.height();
    bool reversed = false;
    for (int row = 0; row < height; row++) {
      if (cancelled) {
        return;
      }
      emitRow(entry, row, threshold, reversed, speed);
      // Alternate direction so the beam does not fly back for every row.
      reversed = !reversed;
    }
  }
  setPwm(0);
  if (proc->galvo()) {
    proc->galvo()->params().dotting_time_us = 0;
  }
}
