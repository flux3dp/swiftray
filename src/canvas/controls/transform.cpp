#include <QDebug>
#include <QObject>
#include <canvas/controls/transform.h>
#include <canvas/canvas.h>
#include <QtMath>

using namespace Controls;

// TODO (rewrite since it's too long)

Transform::Transform(Canvas *canvas) noexcept:
     CanvasControl(canvas),
     active_control_(Control::NONE),
     scale_locked_(false),
     direction_locked_(false),
     scale_x_to_apply_(1),
     scale_y_to_apply_(1),
     rotation_to_apply_(0),
     translate_to_apply_(QPointF()) {

  connect(canvas, &Canvas::selectionsChanged, this, &Transform::updateSelections);
}

bool Transform::isActive() {
  return !document().selections().isEmpty() &&
         (canvas().mode() == Canvas::Mode::Selecting ||
          canvas().mode() == Canvas::Mode::Moving ||
          canvas().mode() == Canvas::Mode::Rotating ||
          canvas().mode() == Canvas::Mode::Transforming);
}

QList<ShapePtr> &Transform::selections() { return selections_; }

void Transform::updateSelections(QList<ShapePtr> selections) {
  reset();
  selections_ = selections;
  bbox_need_recalc_ = true;
  Q_EMIT canvas().transformChanged(x(), y(), rotation(), width(), height());
}

void Transform::updateBoundingRect() {
  if (selections().empty()) {
    bounding_rect_ = QRectF(0, 0, 0, 0);
    bbox_angle_ = 0;
  }

  // Check if all selection's rotation are the same
  bool all_same_direction = true;
  qreal rotation = selections().empty() ? 0 : selections().first()->rotation();

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
    rotation = rotation - qFloor(rotation/360) * 360 ;

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

void Transform::applyRotate(QPointF center, double rotation, bool temporarily) {
  QTransform transform =
       QTransform()
            .translate(center.x(), center.y())
            .rotate(rotation)
            .translate(-center.x(), -center.y());

  auto cmd = Commands::Joined();

  for (ShapePtr &shape : selections()) {
    if (temporarily) {
      shape->setTempTransform(transform);
    } else {
      qDebug() << "[Transform] Apply rotation";
      shape->setTempTransform(QTransform());
      cmd << Commands::SetTransform(shape.get(), shape->transform() * transform);
      cmd << Commands::SetRotation(shape.get(), shape->rotation() + rotation_to_apply_);
    }
  }

  if (!temporarily) {
    rotation_to_apply_ = 0;
    bbox_need_recalc_ = true;
    document().execute(cmd);
  }
}

void Transform::applyScale(QPointF center, double scale_x, double scale_y, bool temporarily) {
  QTransform transform =
       QTransform()
            .translate(center.x(), center.y())
            .rotate(bbox_angle_)
            .scale(scale_x, scale_y)
            .rotate(-bbox_angle_)
            .translate(-center.x(), -center.y());
  auto cmd = Commands::Joined();

  for (ShapePtr &shape : selections()) {
    if (temporarily) {
      shape->setTempTransform(transform);
    } else {
      qDebug() << "[Transform] Apply scale";
      shape->setTempTransform(QTransform());
      cmd << Commands::SetTransform(shape.get(), shape->transform() * transform);
    }
  }

  if (!temporarily) {
    scale_x_to_apply_ = scale_y_to_apply_ = 1;
    bbox_need_recalc_ = true;
    document().execute(cmd);
  }
}

void Transform::applyMove(bool temporarily) {
  QTransform transform = QTransform().translate(translate_to_apply_.x(),
                                                translate_to_apply_.y());
  auto cmd = Commands::Joined();
  for (ShapePtr &shape : selections()) {
    if (temporarily) {
      shape->setTempTransform(transform);
    } else {
      qDebug() << "[Transform] Apply move";
      shape->setTempTransform(QTransform());
      cmd << Commands::SetTransform(shape.get(), shape->transform() * transform);
    }
  }

  if (!temporarily) {
    bbox_need_recalc_ = true;
    translate_to_apply_ = QPointF();
    document().execute(cmd);
  }
}

void Transform::controlPoints() {
  QRectF bbox = boundingRect().translated(translate_to_apply_.x(),
                                          translate_to_apply_.y());
  QTransform transform =
       QTransform()
            .translate(bbox.center().x(), bbox.center().y())
            .rotate(bbox_angle_ + rotation_to_apply_)
            .translate(-bbox.center().x(), -bbox.center().y());
  QTransform transformNoScale = transform;

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
  controls_[8] = transform.map((bbox.topLeft() + bbox.topRight()) / 2) 
                + transformNoScale.map((bbox.topLeft() + bbox.topRight()) / 2 + QPointF(0, -40)) 
                - transformNoScale.map((bbox.topLeft() + bbox.topRight()) / 2);
}

Transform::Control Transform::hitTest(QPointF clickPoint,
                                      float tolerance) {
  controlPoints();
  for (int i = (int) Control::NW; i <= (int) Control::ROTATION; i++) {
    if ((controls_[i] - clickPoint).manhattanLength() < tolerance) {
      return (Control) i;
    }
  }

  if (!this->boundingRect().contains(clickPoint)) {
    for (int i = 0; i < 9; i++) {
      if ((controls_[i] - clickPoint).manhattanLength() < tolerance * 1.5) {
        return Control::ROTATION;
      }
    }
  }

  return Control::NONE;
}

bool Transform::mousePressEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  reset();

  active_control_ = hitTest(canvas_coord, 10 / document().scale());
  if (active_control_ == Control::NONE)
    return false;

  if (active_control_ == Control::ROTATION) {
    action_center_ = (controls_[0] + controls_[4]) / 2; // Rotate around rotated bbox center
    rotated_from_ = atan2(canvas_coord.y() - action_center_.y(),
                          canvas_coord.x() - action_center_.x());
    canvas().setMode(Canvas::Mode::Rotating);
  } else {
    action_center_ = controls_[((int) active_control_ + 4) % 8];
    transformed_from_ = QSizeF(boundingRect().size());
    canvas().setMode(Canvas::Mode::Transforming);
  }
  return true;
}

bool Transform::mouseReleaseEvent(QMouseEvent *e) {
  bool transform_changed = false;
  if (rotation_to_apply_ != 0) {
    transform_changed = true;
    applyRotate(action_center_, rotation_to_apply_);
  }
  if (translate_to_apply_ != QPointF()) {
    transform_changed = true;
    applyMove();
  }
  if (scale_x_to_apply_ != 1 || scale_y_to_apply_ != 1) {
    transform_changed = true;
    applyScale(action_center_, scale_x_to_apply_, scale_y_to_apply_);
  }
  if (transform_changed) {
    Q_EMIT canvas().transformChanged(x(), y(), rotation(), width(), height());
    Q_EMIT shapeUpdated();
  }
  reset();
  canvas().setMode(Canvas::Mode::Selecting);
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

  scale_x_to_apply_ = transformed_from_.width() == 0 ? 1 : d.x() / transformed_from_.width();
  scale_y_to_apply_ = transformed_from_.height() == 0 ? 1 : d.y() / transformed_from_.height();

  if (scale_locked_) {
    if (active_control_ == Control::N || active_control_ == Control::S) {
      scale_x_to_apply_ = scale_y_to_apply_;
    } else if (active_control_ == Control::W || active_control_ == Control::E) {
      scale_y_to_apply_ = scale_x_to_apply_;
    } else {
      double min_scale = std::min(scale_x_to_apply_, scale_y_to_apply_);
      scale_x_to_apply_ = min_scale;
      scale_y_to_apply_ = min_scale;
    }
  } else {
    if (active_control_ == Control::N || active_control_ == Control::S) {
      scale_x_to_apply_ = 1;
    } else if (active_control_ == Control::W || active_control_ == Control::E) {
      scale_y_to_apply_ = 1;
    }
  }
}

bool Transform::mouseMoveEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());

  if (selections_.empty()) return false;

  if (canvas().mode() == Canvas::Mode::Selecting) {
    // Do move event if user actually dragged
    if ((e->pos() - document().mousePressedScreenCoord()).manhattanLength() > 3) {
      canvas().setMode(Canvas::Mode::Moving);
    }
  }

  double current_angle;
  switch (canvas().mode()) {
    case Canvas::Mode::Moving:
      translate_to_apply_ = canvas_coord - document().mousePressedCanvasCoord();
      if(direction_locked_) {
        current_angle = atan2 (translate_to_apply_.y(),translate_to_apply_.x()) * 180 / M_PI;
        if(-157.5 < current_angle && current_angle <= -112.5) {
          current_angle = -135;
        } else if(-112.5 < current_angle && current_angle <= -67.5) {
          current_angle = -90;
        } else if(-67.5 < current_angle && current_angle <= -22.5) {
          current_angle = -45;
        } else if(-22.5 < current_angle && current_angle <= 22.5) {
          current_angle = 0;
        } else if(22.5 < current_angle && current_angle <= 67.5) {
          current_angle = 45;
        } else if(67.5 < current_angle && current_angle <= 112.5) {
          current_angle = 90;
        } else if(112.5 < current_angle && current_angle <= 157.5) {
          current_angle = 135;
        } else {
          current_angle = 180;
        }
        if(abs(translate_to_apply_.x()) >= abs(translate_to_apply_.y())) {
          translate_to_apply_.setY(tan(current_angle * M_PI / 180.0) * translate_to_apply_.x());
        } else {
          translate_to_apply_.setX(tan(M_PI/2.0 - (current_angle * M_PI / 180.0)) * translate_to_apply_.y());
        }
      }
      applyMove(true);
      break;
    case Canvas::Mode::Rotating:
      current_angle = atan2(canvas_coord.y() - action_center_.y(),
                            canvas_coord.x() - action_center_.x()) * 180 / M_PI;        
      current_angle -= rotated_from_ * 180 / M_PI;
      if(current_angle > 180) current_angle -= 360;
      if(current_angle < -180) current_angle += 360;
      if(direction_locked_) {
        if(-157.5 < current_angle && current_angle <= -112.5) {
          current_angle = -135;
        } else if(-112.5 < current_angle && current_angle <= -67.5) {
          current_angle = -90;
        } else if(-67.5 < current_angle && current_angle <= -22.5) {
          current_angle = -45;
        } else if(-22.5 < current_angle && current_angle <= 22.5) {
          current_angle = 0;
        } else if(22.5 < current_angle && current_angle <= 67.5) {
          current_angle = 45;
        } else if(67.5 < current_angle && current_angle <= 112.5) {
          current_angle = 90;
        } else if(112.5 < current_angle && current_angle <= 157.5) {
          current_angle = 135;
        } else {
          current_angle = 180;
        }
      }
      rotation_to_apply_ = current_angle;
      applyRotate(action_center_, rotation_to_apply_, true);
      break;

    case Canvas::Mode::Transforming:
      calcScale(canvas_coord);
      applyScale(action_center_, scale_x_to_apply_, scale_y_to_apply_, true);
      break;
    default:
      return false;
  }
  Q_EMIT shapeUpdated();
  return true;
}

bool Transform::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
  Control cp = hitTest(document().getCanvasCoord(e->pos()), 10 / document().scale());
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

/**
 * @brief Draw bounding box and control points of selections
 * @param painter
 */
void Transform::paint(QPainter *painter) {
  auto sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
  auto dash_pen = QPen(QColor(20, 20, 20), 2, Qt::DashLine);
  dash_pen.setCosmetic(true);
  dash_pen.setDashPattern(QVector<qreal>({5, 5}));
  auto pt_pen = QPen(sky_blue, 10 / document().scale(), Qt::PenStyle::SolidLine,
                     Qt::RoundCap);

  if (selections().size() > 0) {
    controlPoints();
    painter->setPen(dash_pen);
    painter->drawPolyline(controls_, 8);
    painter->drawLine(controls_[7], controls_[0]);
    painter->drawLine(controls_[8], controls_[1]);
    painter->setPen(pt_pen);
    painter->drawPoints(controls_, 9);
  }
}

void Transform::reset() {
  active_control_ = Control::NONE;
  scale_x_to_apply_ = scale_y_to_apply_ = 1;
  rotation_to_apply_ = 0;
  translate_to_apply_ = QPointF();
}

bool Transform::keyPressEvent(QKeyEvent *e) {
  switch (e->key()) {
    case Qt::Key::Key_Up:
      translate_to_apply_ = QPointF(0, e->isAutoRepeat() ? -10 : -1);
      break;
    case Qt::Key::Key_Down:
      translate_to_apply_ = QPointF(0, e->isAutoRepeat() ? 10 : 1);
      break;
    case Qt::Key::Key_Left:
      translate_to_apply_ = QPointF(e->isAutoRepeat() ? -10 : -1, 0);
      break;
    case Qt::Key::Key_Right:
      translate_to_apply_ = QPointF(e->isAutoRepeat() ? 10 : 1, 0);
      break;
    default:
      return false;
  }
  applyMove();
  Q_EMIT canvas().transformChanged(x(), y(), rotation(), width(), height());
  Q_EMIT shapeUpdated();
  return true;
}

bool Transform::isScaleLock() const {
  return scale_locked_;
}

void Transform::setScaleLock(bool scale_lock) {
  scale_locked_ = scale_lock;
}

bool Transform::isDirectionLock() const {
  return direction_locked_;
}

void Transform::setDirectionLock(bool direction_lock) {
  direction_locked_ = direction_lock;
}
