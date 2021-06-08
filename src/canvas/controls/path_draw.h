#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>

#ifndef CONTROL_PATH_DRAW_H
#define CONTROL_PATH_DRAW_H
namespace Controls {

constexpr QPointF invalid_point(-1, -1);

class PathDraw : public CanvasControl {
  public:
    PathDraw(Scene &scene_) noexcept;
    bool mousePressEvent(QMouseEvent *e) override;
    bool mouseMoveEvent(QMouseEvent *e) override;
    bool mouseReleaseEvent(QMouseEvent *e) override;
    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;
    void paint(QPainter *painter) override;
    void reset();
    bool hitTest(QPointF canvas_coord);
    bool hitOrigin(QPointF canvas_coord);
    bool isActive() override;

  private:
    QPainterPath working_path_;
    QPointF cursor_;
    bool is_drawing_curve_;
    bool is_closing_curve_;
    QPointF curve_target_;
    QPointF last_ctrl_pt_;
};

}
#endif // CONTROL_PATH_DRAW_H
