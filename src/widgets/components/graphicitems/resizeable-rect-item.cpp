#include "resizeable-rect-item.h"

#include <QDebug>

ResizeableRectItem::ResizeableRectItem(qreal x, qreal y, qreal width, qreal height, QGraphicsItem *parent):
    QGraphicsRectItem(x, y, width, height, parent) {
  setAcceptHoverEvents(true);
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
  setFlag(QGraphicsItem::ItemIsFocusable, true);
  // Make outline fixed-width during scaling of view
  setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
  updateHandlesPos();
}

ResizeableRectItem::ResizeableRectItem(const QRectF &rect, QGraphicsItem *parent):
    QGraphicsRectItem(rect, parent) {
  setAcceptHoverEvents(true);
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
  setFlag(QGraphicsItem::ItemIsFocusable, true);
  // Make outline fixed-width during scaling of view
  setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
  updateHandlesPos();
}

ResizeableRectItem::ResizeableRectItem(QGraphicsItem *parent):
    QGraphicsRectItem(parent){
  setAcceptHoverEvents(true);
  setFlag(QGraphicsItem::ItemIsMovable, true);
  setFlag(QGraphicsItem::ItemIsSelectable, true);
  setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
  setFlag(QGraphicsItem::ItemIsFocusable, true);
  // Make outline fixed-width during scaling of view
  setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
  updateHandlesPos();
}

void ResizeableRectItem::updateRect(QRectF new_rect) {
  prepareGeometryChange();
  setRect(new_rect);
  updateHandlesPos();
  update();
}

ResizeableRectItem::HandleIdx ResizeableRectItem::handleAt(QPointF point) {
  for (int i = 0; i < 8; i++) {
    if (std::get<0>(handles_[i]) != HandleIdx::kHandleNone) {
      if (std::get<1>(handles_[i]).contains(point)) {
        return std::get<0>(handles_[i]);
      }
    }
  }
  return HandleIdx::kHandleNone;
}

void ResizeableRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
  //if self.isSelected():
  HandleIdx handle = handleAt(event->pos());
  QCursor cursor = getHandleCursor(handle);
  setCursor(cursor);
  QGraphicsItem::hoverMoveEvent(event);
}

void ResizeableRectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
  setCursor(Qt::ArrowCursor);
  QGraphicsItem::hoverLeaveEvent(event);
}

void ResizeableRectItem::mousePressEvent(QGraphicsSceneMouseEvent *event) {
  handle_selected_ = handleAt(event->pos());
  if (handle_selected_ != HandleIdx::kHandleNone) {
    mouse_press_pos_ = event->pos();
    mouse_press_rect_ = boundingRect();
    current_action_ = InteractiveAction::kMoveHandle;
  } else if (shape().contains(event->pos())) {
    mouse_press_pos_ = event->pos();
    current_action_ = InteractiveAction::kMovePosition;
  }
}

void ResizeableRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
  if (current_action_ == InteractiveAction::kMoveHandle) {
    if (handle_selected_ != HandleIdx::kHandleNone) {
      interactiveResize(event->pos());
    }
  } else if (current_action_ == InteractiveAction::kMovePosition) {
    interactiveMove(event->pos() - mouse_press_pos_);
    mouse_press_pos_ = event->pos();
  }
}

void ResizeableRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
  if (current_action_ == InteractiveAction::kMovePosition) {
    interactiveMove(event->pos() - mouse_press_pos_);
  }
  current_action_ = InteractiveAction::kNone;
  handle_selected_ = HandleIdx::kHandleNone;
  mouse_press_pos_ = QPointF();
  mouse_press_rect_ = QRectF();
  update();
}

void ResizeableRectItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                               QWidget *widget) {
  painter->setBrush(QBrush(QColor(125, 125, 125, 50)));
  QPen pen1{QColor(50, 149, 168), 0, Qt::SolidLine};
  painter->setPen(pen1);
  painter->drawRect(this->rect());

  painter->setBrush(QBrush(QColor(50, 149, 168, 255)));
  QPen pen2{QColor(50, 149, 168, 255), 0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin};
  painter->setPen(pen2);
  for (int i = 0; i < 8; i++) {
    //if self.handleSelected is None or handle == self.handleSelected:
    if (std::get<0>(handles_[i]) != HandleIdx::kHandleNone) {
      painter->drawRect(std::get<1>(handles_[i]));
    }
  }
}

Qt::CursorShape ResizeableRectItem::getHandleCursor(HandleIdx idx) {
  switch (idx) {
    case HandleIdx::kHandleTopLeft:
      return Qt::SizeFDiagCursor;
    case HandleIdx::kHandleTopMiddle:
      return Qt::SizeVerCursor;
    case HandleIdx::kHandleTopRight:
      return Qt::SizeBDiagCursor;
    case HandleIdx::kHandleMiddleLeft:
      return Qt::SizeHorCursor;
    case HandleIdx::kHandleMiddleRight:
      return Qt::SizeHorCursor;
    case HandleIdx::kHandleBottomLeft:
      return Qt::SizeBDiagCursor;
    case HandleIdx::kHandleBottomMiddle:
      return Qt::SizeVerCursor;
    case HandleIdx::kHandleBottomRight:
      return Qt::SizeFDiagCursor;
    default:
      return Qt::ArrowCursor;
  }
}

QRectF ResizeableRectItem::boundingRect() {
  return this->rect();
}

/**
 * @brief Must be called when bounding rect is changed,
 *        update the handles' position info accordingly
 */
void ResizeableRectItem::updateHandlesPos() {
  qreal s = handle_size_;
  QRectF b = boundingRect();
  handles_[static_cast<int>(HandleIdx::kHandleTopLeft)] =
          std::make_tuple(HandleIdx::kHandleTopLeft, QRectF(b.left(), b.top(), s, s));
  handles_[static_cast<int>(HandleIdx::kHandleTopMiddle)] =
          std::make_tuple(HandleIdx::kHandleTopMiddle, QRectF(b.center().x() - s / 2, b.top(), s, s));
  handles_[static_cast<int>(HandleIdx::kHandleTopRight)] =
          std::make_tuple(HandleIdx::kHandleTopRight, QRectF(b.right() - s, b.top(), s, s));
  handles_[static_cast<int>(HandleIdx::kHandleMiddleLeft)] =
          std::make_tuple(HandleIdx::kHandleMiddleLeft, QRectF(b.left(), b.center().y() - s / 2, s, s));
  handles_[static_cast<int>(HandleIdx::kHandleMiddleRight)] =
          std::make_tuple(HandleIdx::kHandleMiddleRight, QRectF(b.right() - s, b.center().y() - s / 2, s, s));
  handles_[static_cast<int>(HandleIdx::kHandleBottomLeft)] =
          std::make_tuple(HandleIdx::kHandleBottomLeft, QRectF(b.left(), b.bottom() - s, s, s));
  handles_[static_cast<int>(HandleIdx::kHandleBottomMiddle)] =
          std::make_tuple(HandleIdx::kHandleBottomMiddle, QRectF(b.center().x() - s / 2, b.bottom() - s, s, s));
  handles_[static_cast<int>(HandleIdx::kHandleBottomRight)] =
          std::make_tuple(HandleIdx::kHandleBottomRight, QRectF(b.right() - s, b.bottom() - s, s, s));
}

/**
 * @brief Move the bounding rect
 * @param displace displacement from the previous position
 */
void ResizeableRectItem::interactiveMove(QPointF displace) {
  if (displace == QPointF(0, 0)) {
    return;
  }
  QRectF bounding_rect = boundingRect();
  QRectF rect = QRectF(bounding_rect.topLeft() + displace, bounding_rect.size());
  setRect(rect);
  updateHandlesPos();
}

/**
 * @brief Resize the resizeable rectangle based on current mouse position
 * @param mouse_pos The real-time mouse position in scaled graphicsscene coordinate 
 *                  when any resize handle is being pressed
 */
void ResizeableRectItem::interactiveResize(QPointF mouse_pos) {
  qreal offset = handle_size_ + handle_space_;
  QRectF bounding_rect = boundingRect();
  QRectF rect = this->rect();

  prepareGeometryChange();

  qreal from_x, from_y;
  qreal to_x, to_y;

  switch (handle_selected_) {
    case HandleIdx::kHandleTopLeft:
      from_x = mouse_press_rect_.left();
      from_y = mouse_press_rect_.top();
      to_x = from_x + mouse_pos.x() - mouse_press_pos_.x();
      to_y = from_y + mouse_pos.y() - mouse_press_pos_.y();
      bounding_rect.setLeft(to_x);
      bounding_rect.setTop(to_y);
      rect.setLeft(bounding_rect.left() + offset);
      rect.setTop(bounding_rect.top() + offset);
      setRect(rect);
      break;
    case HandleIdx::kHandleTopMiddle:
      from_y = mouse_press_rect_.top();
      to_y = from_y + mouse_pos.y() - mouse_press_pos_.y();
      bounding_rect.setTop(to_y);
      rect.setTop(bounding_rect.top() + offset);
      setRect(rect);
      break;
    case HandleIdx::kHandleTopRight:
      from_x = mouse_press_rect_.right();
      from_y = mouse_press_rect_.top();
      to_x = from_x + mouse_pos.x() - mouse_press_pos_.x();
      to_y = from_y + mouse_pos.y() - mouse_press_pos_.y();
      bounding_rect.setRight(to_x);
      bounding_rect.setTop(to_y);
      rect.setRight(bounding_rect.right() - offset);
      rect.setTop(bounding_rect.top() + offset);
      setRect(rect);
      break;
    case HandleIdx::kHandleMiddleLeft:
      from_x = mouse_press_rect_.left();
      to_x = from_x + mouse_pos.x() - mouse_press_pos_.x();
      bounding_rect.setLeft(to_x);
      rect.setLeft(bounding_rect.left() + offset);
      setRect(rect);
      break;
    case HandleIdx::kHandleMiddleRight:
      from_x = mouse_press_rect_.right();
      to_x = from_x + mouse_pos.x() - mouse_press_pos_.x();
      bounding_rect.setRight(to_x);
      rect.setRight(bounding_rect.right() - offset);
      setRect(rect);
      break;
    case HandleIdx::kHandleBottomLeft:
      from_x = mouse_press_rect_.left();
      from_y = mouse_press_rect_.bottom();
      to_x = from_x + mouse_pos.x() - mouse_press_pos_.x();
      to_y = from_y + mouse_pos.y() - mouse_press_pos_.y();
      bounding_rect.setLeft(to_x);
      bounding_rect.setBottom(to_y);
      rect.setLeft(bounding_rect.left() + offset);
      rect.setBottom(bounding_rect.bottom() - offset);
      setRect(rect);
      break;
    case HandleIdx::kHandleBottomMiddle:
      from_y = mouse_press_rect_.bottom();
      to_y = from_y + mouse_pos.y() - mouse_press_pos_.y();
      bounding_rect.setBottom(to_y);
      rect.setBottom(bounding_rect.bottom() - offset);
      setRect(rect);
      break;
    case HandleIdx::kHandleBottomRight:
      from_x = mouse_press_rect_.right();
      from_y = mouse_press_rect_.bottom();
      to_x = from_x + mouse_pos.x() - mouse_press_pos_.x();
      to_y = from_y + mouse_pos.y() - mouse_press_pos_.y();
      bounding_rect.setRight(to_x);
      bounding_rect.setBottom(to_y);
      rect.setRight(bounding_rect.right() - offset);
      rect.setBottom(bounding_rect.bottom() - offset);
      setRect(rect);
      break;
    default:
      return;
  }

  updateHandlesPos();
}
