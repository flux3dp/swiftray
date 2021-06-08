#include <QMouseEvent>
#include <canvas/controls/canvas_control.h>
#include <shape/path_shape.h>

#ifndef PATHEDITOR_H
#define PATHEDITOR_H

class PathEditor : public CanvasControl {
  public:
    PathEditor(Scene &scene_) noexcept;
    bool mousePressEvent(QMouseEvent *e) override;
    bool mouseMoveEvent(QMouseEvent *e) override;
    bool mouseReleaseEvent(QMouseEvent *e) override;
    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;
    bool keyPressEvent(QKeyEvent *e) override;
    void moveElementTo(int index, QPointF local_coord);
    void paint(QPainter *painter) override;
    void reset();
    void endEditing();
    int hitTest(QPointF canvas_coord);
    qreal distance(QPointF point);
    PathShape &target();
    void setTarget(ShapePtr target);
    QPainterPath &path();
    QPointF getLocalCoord(QPointF canvas_coord);
    class PathNode {
      public:
        PathShape::NodeType type;
        bool selected;
        PathNode(PathShape::NodeType node_type) : type{node_type} {};
    };

  private:
    ShapePtr target_;
    int dragging_index_;
    bool is_closed_shape_;
    QList<PathNode> cache_;
};

#endif // PATHEDITOR_H
