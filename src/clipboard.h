#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QList>
#include <shape/shape.h>
#include <document.h>

class Clipboard {
public:
  Clipboard();

  void set(QList<ShapePtr> &items);

  void cutFrom(Document &doc);

  void pasteTo(Document &doc);

  void clear();

private:
  QList<ShapePtr> shapes_;
  QPointF paste_shift_;
};

#endif