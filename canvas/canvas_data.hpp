#include <QPoint>
#include <shape/shape.hpp>
#ifndef CANVAS_DATA_HPP
#define CANVAS_DATA_HPP
class CanvasData : QObject {
        Q_OBJECT
    public:
        enum class Mode {
            SELECTING,
            MOVING,
            MULTI_SELECTING,
            TRANSFORMING
        };
        qreal scroll_x;
        qreal scroll_y;
        qreal scale;
        bool isSelected(Shape *shape);
        QList<Shape *> &selections();

        CanvasData() noexcept {
            mode_ = Mode::SELECTING;
        }

        Mode mode() {
            return mode_;
        }

        void setMode(Mode mode) {
            mode_ = mode;
        }

        QPointF getCanvasCoord(QPointF window_coord) const {
            return (window_coord - QPointF(scroll_x, scroll_y)) / scale;
        }
        QPointF scroll() const {
            return QPointF(scroll_x, scroll_y);
        }
        void setSelection(Shape *shape);
        void setSelections(QList<Shape *> &shape);
        void clear();
        void clearSelection();
        void clearSelectionNoFlag();
        void stackStep();
        void undo();
        void redo();
        Shape *shapesAt(int index);
        QList<QList<Shape>> undo_stack;
        QList<QList<Shape>> redo_stack;
        QList<Shape> shapes_;
        QList<Shape> shape_clipboard;
    signals:
        void selectionsChanged();
    private:
        QList<Shape *> selections_;
        Mode mode_;
};

#endif // CANVAS_DATA_HPP
