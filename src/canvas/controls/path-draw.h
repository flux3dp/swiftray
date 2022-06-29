#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  constexpr QPointF invalid_point(-1, -1);

  /**
   \class PathDraw
   \brief The PathDraw class represents the drawing control for paths (drawing only)
   */
  class PathDraw : public CanvasControl {
  public:
    explicit PathDraw(Canvas *canvas);

    bool mousePressEvent(QMouseEvent *e) override;

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool keyPressEvent(QKeyEvent *e) override;

    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;

    void paint(QPainter *painter) override;

    void exit() override;

    bool hitTest(QPointF canvas_coord);

    bool hitOrigin(QPointF canvas_coord);

    bool isActive() override;

    bool isDirectionLock() const;

    void setDirectionLock(bool direction_lock);

  private:
    QPainterPath working_path_;
    QPointF cursor_;
    bool is_drawing_curve_;
    bool is_closing_curve_;
    QPointF curve_target_;
    QPointF last_ctrl_pt_;
    bool direction_locked_;
  };

}