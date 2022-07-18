#pragma once

#include <QList>
#include <QMutex>
#include <shape/shape.h>
#include <document.h>

class Clipboard {
public:
  Clipboard();

  void set(QList<ShapePtr> &items);

  void cutFrom(Document &doc);

  void pasteTo(Document &doc);

  void pasteTo(Document &doc, QPointF target_point);

  void pasteInPlace(Document &doc);

  void clear();


private:
  QRectF calculateBoundingRect();

  QList<ShapePtr> shapes_;
  QMutex shapes_mutex_;
  QPointF paste_shift_;
};
