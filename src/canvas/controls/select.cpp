#include <QDebug>

#include <canvas/controls/select.h>
#include <canvas/canvas.h>

using namespace Controls;

Select::Select(Canvas *canvas) noexcept: CanvasControl(canvas) {
  selection_box_ = QRectF(0, 0, 0, 0);
}

bool Select::isActive() {
  return canvas().mode() == Canvas::Mode::MultiSelecting;
}

bool Select::mouseMoveEvent(QMouseEvent *e) {
  QPointF start = document().mousePressedCanvasCoord();
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  selection_box_ = QRectF(std::min(start.x(), canvas_coord.x()),
                          std::min(start.y(), canvas_coord.y()),
                          std::abs(start.x() - canvas_coord.x()),
                          std::abs(start.y() - canvas_coord.y()));
  return true;
}

bool Select::mouseReleaseEvent(QMouseEvent *e) {
  if (selection_box_.width() != 0 || selection_box_.height() != 0) {
    QList<ShapePtr> selected;
    for (auto &layer : document().layers()) {
      if (!layer->isVisible()) {
        continue;
      }
      for (auto &shape : layer->children()) {
        if (shape->hitTest(selection_box_)) {
          selected << shape;
        }
      }
    }
    document().setSelections(selected);
    if(selected.empty()) {
      canvas().setMode(Canvas::Mode::Selecting);
      selection_box_ = QRectF(0, 0, 0, 0);
      return true;
    }
    QString target_layer = selected.first()->layer()->name();
    bool is_same = true;
    for(unsigned int i = 0; i < selected.size(); ++i) {
      if(target_layer != selected[i]->layer()->name()) {
        is_same = false;
        break;
      }
    }
    if(is_same) {
      document().setActiveLayer(target_layer);
      emit canvas().layerChanged();
    }
  }

  canvas().setMode(Canvas::Mode::Selecting);
  selection_box_ = QRectF(0, 0, 0, 0);
  return true;
}

void Select::paint(QPainter *painter) {
  painter->setPen(
       QPen(QColor::fromRgb(0x00, 0x99, 0xCC, 255), 0, Qt::DashLine));
  painter->fillRect(selection_box_,
                    QBrush(QColor::fromRgb(0x00, 0x99, 0xCC, 30)));
  painter->drawRect(selection_box_);
}