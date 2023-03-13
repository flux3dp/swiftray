#include <QApplication>
#include <QDebug>
#include <QPainterPath>
#include <canvas/controls/path-draw.h>
#include <shape/path-shape.h>
#include <canvas/canvas.h>

using namespace Controls;

bool PathDraw::isActive() {
  return canvas().mode() == Canvas::Mode::PathDrawing;
}

PathDraw::PathDraw(Canvas *canvas) : CanvasControl(canvas) {
  curve_target_ = invalid_point;
  last_ctrl_pt_ = invalid_point;
  is_drawing_curve_ = false;
  is_closing_curve_ = false;
  direction_locked_ = false;
}

bool PathDraw::mousePressEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  if(direction_locked_ && working_path_.elementCount() !=0) {
    QPointF reference_point = working_path_.elementAt(working_path_.elementCount()-1);
    QPointF move_point = canvas_coord - reference_point;
    double current_angle = atan2 (move_point.y(),move_point.x()) * 180 / M_PI;
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
    if(abs(move_point.x()) >= abs(move_point.y())) {
      canvas_coord.setY(tan(current_angle * M_PI / 180.0) * move_point.x() + reference_point.y());
    } else {
      canvas_coord.setX(tan(M_PI/2.0 - (current_angle * M_PI / 180.0)) * move_point.y() + reference_point.x());
    }
  }

  curve_target_ = canvas_coord;

  if (hitOrigin(canvas_coord)) {
    curve_target_ = working_path_.elementAt(0);
  }
  if (hitTest(canvas_coord)) {
    is_closing_curve_ = true;
  }
  return true;
}

bool PathDraw::mouseMoveEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  if ((canvas_coord - curve_target_).manhattanLength() < 10)
    return false;
  is_drawing_curve_ = true;
  if(direction_locked_) {
    QPointF reference_point = curve_target_;
    QPointF move_point = canvas_coord - reference_point;
    double current_angle = atan2 (move_point.y(),move_point.x()) * 180 / M_PI;
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
    if(abs(move_point.x()) >= abs(move_point.y())) {
      canvas_coord.setY(tan(current_angle * M_PI / 180.0) * move_point.x() + reference_point.y());
    } else {
      canvas_coord.setX(tan(M_PI/2.0 - (current_angle * M_PI / 180.0)) * move_point.y() + reference_point.x());
    }
  }
  cursor_ = canvas_coord;
  Q_EMIT canvasUpdated();
  return true;
}

bool PathDraw::hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) {
  *cursor = Qt::CrossCursor;
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  if(direction_locked_ && working_path_.elementCount() !=0) {
    QPointF reference_point = working_path_.elementAt(working_path_.elementCount()-1);
    QPointF move_point = canvas_coord - reference_point;
    double current_angle = atan2 (move_point.y(),move_point.x()) * 180 / M_PI;
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
    if(abs(move_point.x()) >= abs(move_point.y())) {
      canvas_coord.setY(tan(current_angle * M_PI / 180.0) * move_point.x() + reference_point.y());
    } else {
      canvas_coord.setX(tan(M_PI/2.0 - (current_angle * M_PI / 180.0)) * move_point.y() + reference_point.x());
    }
  }
  cursor_ = canvas_coord;
  return true;
}

bool PathDraw::mouseReleaseEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());

  if (working_path_.elementCount() == 0) {
    working_path_.moveTo(canvas_coord);
    last_ctrl_pt_ = canvas_coord;
    return true;
  }

  if (!hitTest(canvas_coord) || hitOrigin(canvas_coord)) {
    if (is_drawing_curve_) {
      if (curve_target_ != invalid_point) {
        if (last_ctrl_pt_ != invalid_point) {
          working_path_.cubicTo(last_ctrl_pt_,
                                curve_target_ * 2 - cursor_,
                                curve_target_);
        } else {
          working_path_.cubicTo(curve_target_ * 2 - cursor_,
                                curve_target_ * 2 - cursor_,
                                curve_target_);
        }
        last_ctrl_pt_ = cursor_;
        curve_target_ = invalid_point;
      } else {
        qInfo() << "Release"
                << "Write Curve Point 2";
        working_path_.cubicTo(last_ctrl_pt_, cursor_, cursor_);
        last_ctrl_pt_ = invalid_point;
        is_drawing_curve_ = false;
      }
    } else {
      qInfo() << "Release"
              << "Write Line Point";
      working_path_.lineTo(cursor_);
    }
  }

  if (is_closing_curve_) {
    ShapePtr new_shape = std::make_shared<PathShape>(working_path_);
    document().execute(
         Commands::AddShape(document().activeLayer(), new_shape),
         Commands::Select(&document(), {new_shape})
    );
    exit();
  }
  Q_EMIT canvasUpdated();
  return true;
}

bool PathDraw::hitOrigin(QPointF canvas_coord) {
  if (working_path_.elementCount() > 0 &&
      (working_path_.elementAt(0) - canvas_coord).manhattanLength() <
      15 / document().scale()) {
    return true;
  }
  return false;
}

bool PathDraw::hitTest(QPointF canvas_coord) {
  for (int i = 0; i < working_path_.elementCount(); i++) {
    QPainterPath::Element ele = working_path_.elementAt(i);
    if (ele.isMoveTo() || ele.isLineTo()) {
      if ((ele - canvas_coord).manhattanLength() < 15 / document().scale()) {
        return true;
      }
    } else if (ele.isCurveTo()) {
      QPointF ele_end_point = working_path_.elementAt(i + 2);
      if ((ele_end_point - canvas_coord).manhattanLength() <
          15 / document().scale()) {
        return true;
      }
    }
  }
  return false;
}

void PathDraw::paint(QPainter *painter) {
  auto sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
  auto blue_pen = QPen(sky_blue, 2, Qt::SolidLine);
  auto black_pen = QPen(document().activeLayer()->color(), 2, Qt::SolidLine);
  float point_size = 4 / document().scale();
  blue_pen.setCosmetic(true);
  black_pen.setCosmetic(true);
  painter->setPen(black_pen);
  if (working_path_.elementCount() > 0) {
    if (is_drawing_curve_) {
      QPainterPath wp_clone = working_path_;
      if (curve_target_ != invalid_point) {
        wp_clone.cubicTo(last_ctrl_pt_, curve_target_ * 2 - cursor_,
                         curve_target_);
        painter->drawPath(wp_clone);
        painter->setPen(blue_pen);
        painter->drawLine(cursor_, curve_target_);
        painter->drawLine(curve_target_ * 2 - cursor_, curve_target_);
        painter->drawEllipse(curve_target_ * 2 - cursor_, point_size, point_size);
        painter->drawEllipse(curve_target_, point_size, point_size);
        painter->drawEllipse(cursor_, point_size, point_size);
      } else {
        QPainterPath wp_clone = working_path_;
        wp_clone.cubicTo(last_ctrl_pt_, cursor_, cursor_);
        painter->drawPath(wp_clone);
        painter->drawEllipse(cursor_, point_size, point_size);
      }
    } else {
      painter->drawPath(working_path_);
      painter->drawLine(working_path_.pointAtPercent(1), cursor_);
    }
  }

  painter->setPen(blue_pen);
  for (int i = 0; i < working_path_.elementCount(); i++) {
    QPainterPath::Element ele = working_path_.elementAt(i);
    if (ele.isMoveTo() || ele.isLineTo()) {
      painter->drawEllipse(ele, point_size, point_size);
    } else if (ele.isCurveTo()) {
      QPointF ele_end_point = working_path_.elementAt(i + 2);
      painter->drawEllipse(ele_end_point, point_size, point_size);
    }
  }
}


bool PathDraw::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key::Key_Backspace) {
    exit();
    return true;
  } else if (e->key() == Qt::Key::Key_Escape) {
    if (working_path_.elementCount() > 1) { // require at least two points to form a path
      ShapePtr new_shape = std::make_shared<PathShape>(working_path_);
      document().execute(
              Commands::AddShape(document().activeLayer(), new_shape),
              Commands::Select(&document(), {new_shape})
      );
    }
    exit();
    return true;
  }
  return false;
}

void PathDraw::exit() {
  working_path_ = QPainterPath();
  curve_target_ = invalid_point;
  last_ctrl_pt_ = invalid_point;
  is_drawing_curve_ = false;
  is_closing_curve_ = false;
  canvas().setMode(Canvas::Mode::Selecting);
}

bool PathDraw::isDirectionLock() const {
  return direction_locked_;
}

void PathDraw::setDirectionLock(bool direction_lock) {
  direction_locked_ = direction_lock;
}