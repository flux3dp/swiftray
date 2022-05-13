#pragma once

#include <QGraphicsRectItem>
#include <QGraphicsItem>
#include <QCursor>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <memory>

class ResizeableRectItem: public QGraphicsRectItem {
public:
    enum class HandleIdx {
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

    enum class InteractiveAction {
        kNone,
        kMoveHandle,
        kMovePosition,
    };

    ResizeableRectItem(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent = nullptr);
    ResizeableRectItem(const QRectF &rect, QGraphicsItem *parent = nullptr);
    ResizeableRectItem(QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget);
    void updateRect(QRectF new_rect);

    HandleIdx handleAt(QPointF point);

protected:
    void hoverLeaveEvent(QGraphicsSceneHoverEvent * event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent * event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

private:
    Qt::CursorShape getHandleCursor(HandleIdx idx);
    void updateHandlesPos();
    void interactiveMove(QPointF displace);
    void interactiveResize(QPointF mouse_pos);

    std::tuple<HandleIdx, QRectF> handles_[8] = { std::make_tuple(HandleIdx::kHandleNone, QRectF()) };
    HandleIdx handle_selected_ = HandleIdx::kHandleNone;
    QPointF mouse_press_pos_;
    QRectF mouse_press_rect_;
    InteractiveAction current_action_ = InteractiveAction::kNone;

    const qreal handle_size_ = 8.0;
    const qreal handle_space_ = -4.0;
};
