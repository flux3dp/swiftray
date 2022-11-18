#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

namespace Controls {

  class Select : public CanvasControl {
  public:
    Select(Canvas *canvas) noexcept;

    bool mouseMoveEvent(QMouseEvent *e) override;

    bool mouseReleaseEvent(QMouseEvent *e) override;

    void paint(QPainter *painter) override;

    bool isActive() override;

  private:
    QRectF selection_box_;
    bool check_inside_;
  };

}