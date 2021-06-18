#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>
#include <shape/path-shape.h>

#ifndef CONTROL_PATH_EDIT_H
#define CONTROL_PATH_EDIT_H
namespace Controls {

  class PathEdit : public CanvasControl {
  public:
    class PathNode {
    public:
      PathShape::NodeType type;
      bool selected;

      PathNode(PathShape::NodeType node_type) : type{node_type} {};
    };

    PathEdit(Document &scene_) noexcept;

    bool mousePressEvent(QMouseEvent *e) override;

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    bool hoverEvent(QHoverEvent *e, Qt::CursorShape *cursor) override;

    bool keyPressEvent(QKeyEvent *e) override;

    void paint(QPainter *painter) override;

    bool isActive() override;

    void endEditing();

    void moveElementTo(int index, QPointF local_coord);

    void exit() override;

    int hitTest(QPointF canvas_coord);

    qreal distance(QPointF point);

    PathShape &target();

    QPointF getLocalCoord(QPointF canvas_coord);

    void setTarget(ShapePtr &target);

    const QPainterPath &path() const;

    void setPath(const QPainterPath &path);

  private:
    ShapePtr target_;
    QPainterPath path_;
    int dragging_index_;
    bool is_closed_shape_;
    QList<PathNode> cache_;
  };

}
#endif // CONTROL_PATH_EDIT_H
