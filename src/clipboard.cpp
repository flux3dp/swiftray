#include <clipboard.h>

Clipboard::Clipboard() {
  paste_shift_ = QPointF();
}

void Clipboard::set(QList<ShapePtr> &items) {
  shapes_mutex_.lock();
  shapes_.clear();
  for (auto &item : items) {
    shapes_.push_back(item->clone());
  }
  shapes_mutex_.unlock();
  paste_shift_ = QPointF(0, 0);
}

void Clipboard::cutFrom(Document &doc) {
  this->set(doc.selections());
  doc.execute(
       Commands::RemoveSelections(&doc)
  );
}

void Clipboard::pasteTo(Document &doc) {
  paste_shift_ += QPointF(20, 20);
  QTransform shift_transform =
       QTransform().translate(paste_shift_.x(), paste_shift_.y());

  QList<ShapePtr> new_shapes;
  shapes_mutex_.lock();
  for (auto &shape : shapes_) {
    ShapePtr new_shape = shape->clone();
    new_shape->applyTransform(shift_transform);
    new_shapes << new_shape;
  }
  shapes_mutex_.unlock();

  doc.execute(
       Commands::AddShapes(doc.activeLayer(), new_shapes),
       Commands::Select(&doc, new_shapes)
  );
}

void Clipboard::pasteTo(Document &doc, QPointF target_point) {

  QList<ShapePtr> new_shapes;
  shapes_mutex_.lock();
  for (auto &shape : shapes_) {
    ShapePtr new_shape = shape->clone();
    QTransform shift_transform =
       QTransform().translate((target_point-shape->pos()).x(), (target_point-shape->pos()).y());
    new_shape->applyTransform(shift_transform);
    new_shapes << new_shape;
  }
  shapes_mutex_.unlock();

  doc.execute(
       Commands::AddShapes(doc.activeLayer(), new_shapes),
       Commands::Select(&doc, new_shapes)
  );
}

void Clipboard::pasteInPlace(Document &doc) {
  QList<ShapePtr> new_shapes;
  shapes_mutex_.lock();
  for (auto &shape : shapes_) {
    ShapePtr new_shape = shape->clone();
    new_shapes << new_shape;
  }
  shapes_mutex_.unlock();

  doc.execute(
       Commands::AddShapes(doc.activeLayer(), new_shapes),
       Commands::Select(&doc, new_shapes)
  );
}

void Clipboard::clear() {
  shapes_mutex_.lock();
  shapes_.clear();
  shapes_mutex_.unlock();
}
