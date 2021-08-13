#pragma once

#include <canvas/controls/canvas-control.h>


namespace Controls {

    class Ruler : public CanvasControl {
    public:
        explicit Ruler(Canvas *canvas) : CanvasControl(canvas) {}

        void paint(QPainter *painter) override;

        bool isActive() override;

    private:
        qreal getScaleStep();
        void drawHorizontalRuler(QPainter *painter, qreal step, int thickness);
        void drawVerticalRuler(QPainter *painter, qreal step, int thickness);
    };
}
