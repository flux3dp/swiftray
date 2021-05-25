#include <QPoint>
#ifndef CANVAS_DATA_HPP
#define CANVAS_DATA_HPP
class CanvasData {
    public:
        enum class Mode {
            SELECTING,
            MULTI_SELECTING,
            TRANSFORMING
        };
        qreal scroll_x;
        qreal scroll_y;
        qreal scale;
        Mode mode;

        CanvasData() noexcept {
            mode = Mode::SELECTING;
        }

        QPointF getCanvasCoord(QPointF window_coord) const {
            return (window_coord - QPointF(scroll_x, scroll_y)) / scale;
        }
        QPointF scroll() const {
            return QPointF(scroll_x, scroll_y);
        }
};

#endif // CANVAS_DATA_HPP
