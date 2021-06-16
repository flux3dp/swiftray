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
    JoinedEvent *evt = new JoinedEvent();
    for (auto &layer : doc.layers()) {
      for (auto &shape : layer->children()) {
        if (shape->selected()) evt->events << make_shared<RemoveShapeEvent>(shape);
      }
    }
    doc.addUndoEvent(evt);
    set(doc.selections());
    doc.removeSelections();
  }

  void pasteTo(Document &doc) {
    paste_shift_ += QPointF(20, 20);
    QTransform shift_transform =
         QTransform().translate(paste_shift_.x(), paste_shift_.y());
    int index_clip_begin = doc.activeLayer()->children().length();

    JoinedEvent *evt = new JoinedEvent();
    for (auto &shape : shapes_) {
      ShapePtr new_shape = shape->clone();
      new_shape->applyTransform(shift_transform);
      doc.activeLayer()->addShape(new_shape);
      evt->events << make_shared<AddShapeEvent>(new_shape);
    }
    doc.addUndoEvent(evt);

    QList<ShapePtr> selected_shapes;

    for (int i = index_clip_begin; i < doc.activeLayer()->children().length(); i++) {
      selected_shapes << doc.activeLayer()->children().at(i);
    }

    doc.setSelections(selected_shapes);
  }

  void clear() {
    shapes_.clear();
  }

  QList<ShapePtr> shapes_;
  QPointF paste_shift_;
};

#endif