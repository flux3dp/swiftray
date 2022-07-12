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

  void pasteInPlace(Document &doc);

  void clear();

private:
  QList<ShapePtr> shapes_;
  QMutex shapes_mutex_;
  QPointF paste_shift_;
};
