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
  QRectF bounding_rect = calculateBoundingRect();
  QPointF paste_shift(bounding_rect.center().x(), bounding_rect.center().y());
  paste_shift = doc.getCanvasCoord(target_point) - paste_shift;

  QList<ShapePtr> new_shapes;
  shapes_mutex_.lock();
  for (auto &shape : shapes_) {
    ShapePtr new_shape = shape->clone();
    QTransform shift_transform =
       QTransform().translate(paste_shift.x(), paste_shift.y());
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

QRectF Clipboard::calculateBoundingRect() {
  QRectF bounding_rect;
  if (shapes_.empty()) {
    bounding_rect = QRectF(0, 0, 0, 0);
  }

  // Check if all selection's rotation are the same
  bool all_same_direction = true;
  qreal rotation = shapes_.empty() ? 0 : shapes_.first()->rotation();

  for (ShapePtr &selection : shapes_) {
    if (selection->rotation() != rotation) {
      all_same_direction = false;
      break;
    }
  }

  float top = std::numeric_limits<float>::max();
  float bottom = std::numeric_limits<float>::min();
  float left = std::numeric_limits<float>::max();
  float right = std::numeric_limits<float>::min();

  if (all_same_direction) {
    QPainterPath local_bbox_path;
    for (ShapePtr &shape : shapes_) {
      local_bbox_path.addPolygon(shape->rotatedBBox());
    }
    QRectF unrotated_bbox =
         QTransform().rotate(-rotation).map(local_bbox_path).boundingRect();
    QPolygonF rotated_bbox = QTransform().rotate(-rotation).inverted().map(
         QPolygonF(unrotated_bbox));

    QPointF global_center = (rotated_bbox[0] + rotated_bbox[1] +
                             rotated_bbox[2] + rotated_bbox[3]) /
                            4;
    bounding_rect =
         QRectF(global_center - QPointF(unrotated_bbox.width() / 2,
                                        unrotated_bbox.height() / 2),
                unrotated_bbox.size());
  } else {
    for (ShapePtr &shape : shapes_) {
      QRectF bb = shape->boundingRect();
      if (bb.left() < left)
        left = bb.left();
      if (bb.top() < top)
        top = bb.top();
      if (bb.right() > right)
        right = bb.right();
      if (bb.bottom() > bottom)
        bottom = bb.bottom();
    }
    bounding_rect = QRectF(left, top, right - left, bottom - top);
  }
  return bounding_rect;
}
