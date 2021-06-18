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
    QList<ShapePtr> items;
    items.append(doc.selections());
    this->set(items);
    doc.removeSelections();
    doc.addUndoEvent(SelectionEvent::shared(items) +
                     JoinedEvent::removeShapes(items));
  }

  void pasteTo(Document &doc) {
    auto undo_evt = make_shared<JoinedEvent>();
    paste_shift_ += QPointF(20, 20);
    QTransform shift_transform =
         QTransform().translate(paste_shift_.x(), paste_shift_.y());
    undo_evt << new SelectionEvent(doc.selections());

    int index_clip_begin = doc.activeLayer()->children().length();
    for (auto &shape : shapes_) {
      ShapePtr new_shape = shape->clone();
      new_shape->applyTransform(shift_transform);
      doc.activeLayer()->addShape(new_shape);
      undo_evt << new AddShapeEvent(new_shape);
    }

    QList<ShapePtr> selected_shapes;

    for (int i = index_clip_begin; i < doc.activeLayer()->children().length(); i++) {
      selected_shapes << doc.activeLayer()->children().at(i);
    }

    doc.setSelections(selected_shapes);
    doc.addUndoEvent(undo_evt);
  }

  void clear() {
    shapes_.clear();
  }

  QList<ShapePtr> shapes_;
  QPointF paste_shift_;
};

#endif