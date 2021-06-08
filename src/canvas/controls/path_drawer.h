#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>

#ifndef PATHDRAWER_H
#define PATHDRAWER_H

constexpr QPointF invalid_point(-1, -1);

class PathDrawer : public CanvasControl {
  public:
    PathDrawer(Scene &scene_) noexcept;
    bool mousePressEvent(QMouseEvent *e) override;
    bool mouseMoveEvent(QMouseEvent *e) override;
    bool mouseReleaseEvent(QMouseEvent *e) override;
    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;
    void paint(QPainter *painter) override;
    void reset();
    bool hitTest(QPointF canvas_coord);
    bool hitOrigin(QPointF canvas_coord);

  private:
    QPainterPath working_path_;
    QPointF cursor_;
    bool is_drawing_curve_;
    bool is_closing_curve_;
    QPointF curve_target_;
    QPointF last_ctrl_pt_;
};

#endif // PATHDRAWER_H
