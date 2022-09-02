#pragma once

#include <QMouseEvent>
#include <canvas/controls/canvas-control.h>
#include <QPoint>


namespace Controls {

    class Polygon : public CanvasControl {
        static constexpr unsigned int kDefaultNumSide = 5;
        static constexpr unsigned int kMinimumNumSide = 3;
    public:
        Polygon(Canvas *canvas) noexcept;

        bool mouseMoveEvent(QMouseEvent *e) override;

        bool mouseReleaseEvent(QMouseEvent *e) override;

        bool keyPressEvent(QKeyEvent *e) override;

        bool keyReleaseEvent(QKeyEvent *e) override;

        void paint(QPainter *painter) override;

        void exit() override;

        bool isActive() override;

        unsigned int getNumSide() const { return num_side_; }

        bool setNumSide(unsigned int numSide);

        void setScaleLock(bool scale_lock);

    private:
        unsigned int num_side_;
        QPointF initial_vertex_;
        QPointF center_;
        QPolygonF polygon_;
        bool scale_locked_;
        float aspect_ratio_ = 1;

        void updateVertices(const QPointF &center, const QPointF &start_vertex);
    };
}
