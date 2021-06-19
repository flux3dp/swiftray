#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>

#ifndef MULTISELECTIONBOX_H
#define MULTISELECTIONBOX_H
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
  };

}

#endif // MULTISELECTIONBOX_H
