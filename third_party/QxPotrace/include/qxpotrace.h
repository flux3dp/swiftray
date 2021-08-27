#ifndef QXPOTRACE_H
#define QXPOTRACE_H

#include <QImage>
#include <QList>
#include <QPolygonF>
#include <QPainterPath>
#include <potracelib.h>

class QxPotrace
{
public:

  QxPotrace() = default;

  bool trace(const QImage &image, int low_thres, int high_thres,
             int turd_size, qreal smooth, qreal curve_tolerance);
  QPainterPath getContours() { return contours; }

private:

  QPainterPath contours;
  void convert_paths_recursively(potrace_path_s* current_contour_p);
  void convert_potrace_path_to_QPainterPath(potrace_path_t *contour);
};

#endif // QXPOTRACE_H
