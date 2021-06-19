#ifndef CLIPBOARD_H
#define CLIPBOARD_H

#include <QList>
#include <shape/shape.h>
#include <document.h>

class Clipboard {
public:
  Clipboard() {
    paste_shift_ = QPointF();
  }

  void set(QList<ShapePtr> &items) {
    shapes_.clear();
    for (auto &item : items) {
      shapes_.push_back(item->clone());
    }
    paste_shift_ = QPointF(0, 0);
  }

  void cutFrom(Document &doc) {
    this->set(doc.selections());
    doc.execute(
         Commands::JoinedCmd::removeSelections(&doc)
    );
  }

  void pasteTo(Document &doc) {
    paste_shift_ += QPointF(20, 20);
    QTransform shift_transform =
         QTransform().translate(paste_shift_.x(), paste_shift_.y());

    QList<ShapePtr> new_shapes;
    for (auto &shape : shapes_) {
      ShapePtr new_shape = shape->clone();
      new_shape->applyTransform(shift_transform);
      new_shapes << new_shape;
    }

    doc.execute(
         Commands::JoinedCmd::addShapes(doc.activeLayer(), new_shapes) +
         Commands::Select::shared(&doc, new_shapes)
    );
  }

  void clear() {
    shapes_.clear();
  }

  QList<ShapePtr> shapes_;
  QPointF paste_shift_;
};

#endif