#pragma once

#include <QGraphicsRectItem>
#include <QGraphicsItem>
#include <QCursor>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <memory>

class ResizeableRectItem: public QGraphicsRectItem {
public:
    enum class HandleIdx{
        kHandleNone = -1,
        kHandleTopLeft = 0,
        kHandleTopMiddle = 1,
        kHandleTopRight = 2,
        kHandleMiddleLeft = 3,
        kHandleMiddleRight = 4,
        kHandleBottomLeft = 5,
        kHandleBottomMiddle = 6,
        kHandleBottomRight = 7
    };

    ResizeableRectItem(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent = nullptr);
    ResizeableRectItem(const QRectF &rect, QGraphicsItem *parent = nullptr);
    ResizeableRectItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget);
    void setMaxBoundary(QRectF max_boundary) { max_boundary_ = max_boundary; }
    void updateRect(QRectF new_rect);

    HandleIdx handleAt(QPointF point);

protected:
    void hoverLeaveEvent(QGraphicsSceneHoverEvent * event);
    void hoverMoveEvent(QGraphicsSceneHoverEvent * event);
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
    Qt::CursorShape getHandleCursor(HandleIdx idx);
    void updateHandlesPos();
    void interactiveResize(QPointF mouse_pos);

    std::tuple<HandleIdx, QRectF> handles_[8] = { std::make_tuple(HandleIdx::kHandleNone, QRectF()) };
    HandleIdx handle_selected_ = HandleIdx::kHandleNone;
    QPointF mouse_press_pos_;
    QRectF mouse_press_rect_;

    const qreal handle_size_ = 16.0;
    const qreal handle_space_ = -8.0;

    QRectF max_boundary_ = QRectF(); // Constraint for resize

};