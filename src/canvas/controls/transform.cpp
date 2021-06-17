#include <QDebug>
#include <QObject>
#include <canvas/controls/transform.h>

using namespace Controls;

Transform::Transform(Document &scene) noexcept: CanvasControl(scene) {
  active_control_ = Control::NONE;
  connect((QObject *) (&this->scene()), SIGNAL(selectionsChanged()), this,
          SLOT(updateSelections()));
}

bool Transform::isActive() {
  return scene().selections().size() > 0 &&
         (scene().mode() == Document::Mode::Selecting ||
          scene().mode() == Document::Mode::Moving ||
          scene().mode() == Document::Mode::Rotating ||
          scene().mode() == Document::Mode::Transforming);
}

QList<ShapePtr> &Transform::selections() { return selections_; }

void Transform::updateSelections() {
  reset();
  selections().clear();
  selections().append(scene().selections());
  bbox_need_recalc_ = true;
  emit transformChanged();
}

void Transform::updateBoundingRect() {
  // Check if all selection's rotation are the same
  bool all_same_direction = true;
  qreal rotation =
       selections().size() > 0 ? selections().first()->rotation() : 0;

  for (ShapePtr &selection : selections()) {
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
    for (ShapePtr &shape : selections()) {
      local_bbox_path.addPolygon(shape->rotatedBBox());
    }
    QRectF unrotated_bbox =
         QTransform().rotate(-rotation).map(local_bbox_path).boundingRect();
    QPolygonF rotated_bbox = QTransform().rotate(-rotation).inverted().map(
         QPolygonF(unrotated_bbox));

    QPointF global_center = (rotated_bbox[0] + rotated_bbox[1] +
                             rotated_bbox[2] + rotated_bbox[3]) /
                            4;
    bounding_rect_ =
         QRectF(global_center - QPointF(unrotated_bbox.width() / 2,
                                        unrotated_bbox.height() / 2),
                unrotated_bbox.size());
    bbox_angle_ = rotation;
  } else {
    for (ShapePtr &shape : selections()) {
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
    bounding_rect_ = QRectF(left, top, right - left, bottom - top);
    bbox_angle_ = 0;
  }
}

QRectF Transform::boundingRect() {
  if (bbox_need_recalc_) {
    updateBoundingRect();
    bbox_need_recalc_ = false;
  }
  return bounding_rect_;
}

void Transform::applyRotate(bool temporarily) {
  QTransform transform =
       QTransform()
            .translate(action_center_.x(), action_center_.y())
            .rotate(rotation_to_apply_)
            .translate(-action_center_.x(), -action_center_.y());

  for (ShapePtr &shape : selections()) {
    if (temporarily) {
      shape->setTempTransform(transform);
    } else {
      shape->setTempTransform(QTransform());
      shape->applyTransform(transform);
      shape->setRotation(shape->rotation() + rotation_to_apply_);
    }
  }

  if (!temporarily) {
    rotation_to_apply_ = 0;
    bbox_need_recalc_ = true;
    emit transformChanged();
  }
}

void Transform::applyScale(bool temporarily) {
  QTransform transform =
       QTransform()
            .translate(action_center_.x(), action_center_.y())
            .rotate(bbox_angle_)
            .scale(scale_x_to_apply_, scale_y_to_apply_)
            .rotate(-bbox_angle_)
            .translate(-action_center_.x(), -action_center_.y());

  for (ShapePtr &shape : selections()) {
    if (temporarily) {
      shape->setTempTransform(transform);
    } else {
      shape->setTempTransform(QTransform());
      shape->applyTransform(transform);
    }
  }

  if (!temporarily) {
    scale_x_to_apply_ = scale_y_to_apply_ = 1;
    bbox_need_recalc_ = true;
    emit transformChanged();
  }
}

void Transform::applyMove(bool temporarily) {
  QTransform transform = QTransform().translate(translate_to_apply_.x(),
                                                translate_to_apply_.y());

  for (ShapePtr &shape : selections()) {
    if (temporarily) {
      shape->setTempTransform(transform);
    } else {
      shape->setTempTransform(QTransform());
      shape->applyTransform(transform);
    }
  }

  if (!temporarily) {
    bbox_need_recalc_ = true;
    translate_to_apply_ = QPointF();
    emit transformChanged();
  }
}

const QPointF *Transform::controlPoints() {
  QRectF bbox = boundingRect().translated(translate_to_apply_.x(),
                                          translate_to_apply_.y());
  QTransform transform =
       QTransform()
            .translate(bbox.center().x(), bbox.center().y())
            .rotate(bbox_angle_ + rotation_to_apply_)
            .translate(-bbox.center().x(), -bbox.center().y());

  if (scale_x_to_apply_ != 1 || scale_y_to_apply_ != 1) {
    transform = transform *
                QTransform()
                     .translate(action_center_.x(), action_center_.y())
                     .rotate(bbox_angle_)
                     .scale(scale_x_to_apply_, scale_y_to_apply_)
                     .rotate(-bbox_angle_)
                     .translate(-action_center_.x(), -action_center_.y());
  }

  controls_[0] = transform.map(bbox.topLeft());
  controls_[1] = transform.map((bbox.topLeft() + bbox.topRight()) / 2);
  controls_[2] = transform.map(bbox.topRight());
  controls_[3] = transform.map((bbox.topRight() + bbox.bottomRight()) / 2);
  controls_[4] = transform.map(bbox.bottomRight());
  controls_[5] = transform.map((bbox.bottomRight() + bbox.bottomLeft()) / 2);
  controls_[6] = transform.map(bbox.bottomLeft());
  controls_[7] = transform.map((bbox.bottomLeft() + bbox.topLeft()) / 2);
  return controls_;
}

Transform::Control Transform::hitTest(QPointF clickPoint,
                                      float tolerance) {
  controlPoints();
  for (int i = (int) Control::NW; i != (int) Control::ROTATION; i++) {
    if ((controls_[i] - clickPoint).manhattanLength() < tolerance) {
      return (Control) i;
    }
  }

  if (!this->boundingRect().contains(clickPoint)) {
    for (int i = 0; i < 8; i++) {
      if ((controls_[i] - clickPoint).manhattanLength() < tolerance * 1.5) {
        return Control::ROTATION;
      }
    }
  }

  return Control::NONE;
}

bool Transform::mousePressEvent(QMouseEvent *e) {
  QPointF canvas_coord = scene().getCanvasCoord(e->pos());
  reset();

  active_control_ = hitTest(canvas_coord, 10 / scene().scale());
  if (active_control_ == Control::NONE)
    return false;

  if (active_control_ == Control::ROTATION) {
    action_center_ = (controls_[0] + controls_[4]) /
                     2; // Rotate around rotated bbox center
    rotated_from_ = atan2(canvas_coord.y() - action_center_.y(),
                          canvas_coord.x() - action_center_.x());
    scene().setMode(Document::Mode::Rotating);
  } else {
    action_center_ = controls_[((int) active_control_ + 4) % 8];
    transformed_from_ = QSizeF(boundingRect().size());
    scene().setMode(Document::Mode::Transforming);
  }
  return true;
}

bool Transform::mouseReleaseEvent(QMouseEvent *e) {
  // Save before changes apply
  auto undo_evt = make_shared<JoinedEvent>();
  for (auto &shape : selections()) {
    undo_evt << make_shared<TransformChangeEvent>(shape.get(), shape->transform());
    if (rotation_to_apply_ != 0)
      undo_evt << make_shared<RotationChangeEvent>(shape.get(), shape->rotation());
  }

  if (rotation_to_apply_ != 0) applyRotate(false);
  if (translate_to_apply_ != QPointF()) applyMove(false);
  if (scale_x_to_apply_ != 1 || scale_y_to_apply_ != 1) applyScale(false);

  reset();

  scene().setMode(Document::Mode::Selecting);
  scene().addUndoEvent(undo_evt);
  return true;
}

void Transform::calcScale(QPointF canvas_coord) {
  cursor_ = canvas_coord;
  QRectF bbox = boundingRect();
  QTransform local_transform_invert =
       QTransform()
            .translate(bbox.center().x(), bbox.center().y())
            .rotate(bbox_angle_)
            .inverted();
  QPointF local_coord = local_transform_invert.map(canvas_coord);

  QPointF d = local_coord - local_transform_invert.map(action_center_);

  if (active_control_ == Control::N || active_control_ == Control::NE ||
      active_control_ == Control::NW)
    d.setY(-d.y());
  if (active_control_ == Control::NW || active_control_ == Control::W ||
      active_control_ == Control::SW)
    d.setX(-d.x());

  scale_x_to_apply_ = d.x() / transformed_from_.width();
  scale_y_to_apply_ = d.y() / transformed_from_.height();

  if (active_control_ == Control::N || active_control_ == Control::S) {
    scale_x_to_apply_ = 1;
  } else if (active_control_ == Control::W || active_control_ == Control::E) {
    scale_y_to_apply_ = 1;
  }
}

bool Transform::mouseMoveEvent(QMouseEvent *e) {
  QPointF canvas_coord = scene().getCanvasCoord(e->pos());

  if (selections_.empty()) return false;

  if (scene().mode() == Document::Mode::Selecting) {
    // Do move event if user actually dragged
    if ((e->pos() - scene().mousePressedScreenCoord()).manhattanLength() > 3) {
      scene().setMode(Document::Mode::Moving);
    }
  }

  switch (scene().mode()) {
    case Document::Mode::Moving:
      translate_to_apply_ = canvas_coord - scene().mousePressedCanvasCoord();
      applyMove(true);
      break;
    case Document::Mode::Rotating:
      rotation_to_apply_ = (atan2(canvas_coord.y() - action_center_.y(),
                                  canvas_coord.x() - action_center_.x()) -
                            rotated_from_) *
                           180 / 3.1415926;
      applyRotate(true);
      break;

    case Document::Mode::Transforming:
      calcScale(canvas_coord);
      applyScale(true);
      break;
    default:
      return false;
  }
  return true;
}

bool Transform::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
  Control cp =
       hitTest(scene().getCanvasCoord(e->pos()), 10 / scene().scale());

  switch (cp) {
    case Control::ROTATION:
      *cursor = Qt::CrossCursor;
      return true;

    case Control::NW:
    case Control::SE:
      *cursor = Qt::SizeFDiagCursor;
      return true;

    case Control::NE:
    case Control::SW:
      *cursor = Qt::SizeBDiagCursor;
      return true;

    case Control::N:
    case Control::S:
      *cursor = Qt::SizeVerCursor;
      return true;

    case Control::W:
    case Control::E:
      *cursor = Qt::SizeHorCursor;
      return true;

    default:
      break;
  }

  return false;
}

void Transform::paint(QPainter *painter) {
  auto sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
  auto blue_pen = QPen(QBrush(sky_blue), 1, Qt::SolidLine);
  blue_pen.setCosmetic(true);
  auto pt_pen = QPen(sky_blue, 10 / scene().scale(), Qt::PenStyle::SolidLine,
                     Qt::RoundCap);

  if (selections().size() > 0) {
    controlPoints();
    painter->setPen(blue_pen);
    painter->drawPolyline(controls_, 8);
    painter->drawLine(controls_[7], controls_[0]);
    painter->setPen(pt_pen);
    painter->drawPoints(controls_, 8);
    painter->drawPoint((controls_[0] + controls_[4]) / 2);
  }
}

void Transform::reset() {
  active_control_ = Control::NONE;
  scale_x_to_apply_ = scale_y_to_apply_ = 1;
  rotation_to_apply_ = 0;
  translate_to_apply_ = QPointF();
}

bool Transform::keyPressEvent(QKeyEvent *e) {
  /// Transform box does not handle all key events
  return false;
}