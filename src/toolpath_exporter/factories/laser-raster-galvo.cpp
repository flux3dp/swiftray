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

void LaserRasterGalvoFactory::emitLine(const Entry& entry,
                                       int index,
                                       int threshold,
                                       bool reversed,
                                       bool transposed,
                                       float speed) {
  const int width = entry.gray.width();
  const int height = entry.gray.height();
  // Along the row, or down the column when transposed.
  const int length = transposed ? height : width;
  const QImage& gray = entry.gray;
  auto sample = [&](int at) {
    return transposed ? gray.constScanLine(at)[index]
                      : gray.constScanLine(index)[at];
  };
  // Sample at pixel centres, the way the gcode raster does, so a run covers the
  // dots it is made of rather than stopping half a pixel short of the last one.
  const double fixed = transposed
                           ? (entry.bbox_px.left() + index + 0.5) /
                                 pixel_per_mm_x - offset.x()
                           : (entry.bbox_px.top() + index + 0.5) /
                                 pixel_per_mm - offset.y();
  auto moving = [&](int at) {
    return transposed
               ? (entry.bbox_px.top() + at) / pixel_per_mm - offset.y()
               : (entry.bbox_px.left() + at) / pixel_per_mm_x - offset.x();
  };
  auto move = [&](double along, bool travel, float feed) {
    NamedArgs args;
    if (transposed) {
      args.rx(fixed).ry(along);
    } else {
      args.rx(along).ry(fixed);
    }
    if (travel) {
      args.set_is_travel();
    } else {
      args.rf(feed);
    }
    proc->moveto(args);
  };

  if (entry.dots) {
    // One dot per dark pixel; the writer turns each of these marks into a jump
    // and a LASER_ON, since a dotting time is set for a dithered image.
    const double half_pixel =
        0.5 / (transposed ? pixel_per_mm : pixel_per_mm_x);
    for (int i = 0; i < length; i++) {
      const int at = reversed ? length - 1 - i : i;
      if (sample(at) >= threshold) {
        continue;
      }
      setPwm(100);
      move(moving(at) + half_pixel, false, speed);
    }
    return;
  }

  // Runs of dark pixels, each one marked segment.
  int run_start = -1;
  for (int i = 0; i <= length; i++) {
    const bool on = i < length && sample(i) < threshold;
    if (on && run_start < 0) {
      run_start = i;
    } else if (!on && run_start >= 0) {
      // +1 on the end: the run covers the whole of its last pixel.
      double from = moving(run_start);
      double to = moving(i);
      if (reversed) {
        std::swap(from, to);
      }
      setPwm(0);
      move(from, true, 0);
      setPwm(100);
      move(to, false, speed);
      run_start = -1;
    }
  }
}

void LaserRasterGalvoFactory::generate_task_code(float speed, int threshold,
                                                 bool transposed) {
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
    const int lines = transposed ? entry.gray.width() : entry.gray.height();
    bool reversed = false;
    for (int line = 0; line < lines; line++) {
      if (cancelled) {
        return;
      }
      emitLine(entry, line, threshold, reversed, transposed, speed);
      // Alternate direction so the beam does not fly back for every line.
      reversed = !reversed;
    }
  }
  setPwm(0);
  if (proc->galvo()) {
    proc->galvo()->params().dotting_time_us = 0;
  }
}
