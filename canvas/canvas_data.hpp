#include <QPoint>
#include <shape/shape.hpp>
#include <container/shape_collection.h>
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
        bool isSelected(ShapePtr shape);
        QList<ShapePtr> &selections();

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
        void setSelection(ShapePtr shape);
        void setSelections(QList<ShapePtr> &shapes);
        void clear();
        void clearSelection();
        void clearSelectionNoFlag();
        void stackStep();
        ShapeCollection &shapes();
        void undo();
        void redo();
        QList<ShapeCollection> undo_stack;
        QList<ShapeCollection> redo_stack;
        ShapeCollection shape_clipboard;
    signals:
        void selectionsChanged();
    private:
        ShapeCollection shapes_;
        QList<ShapePtr> selections_;
        Mode mode_;
};

#endif // CANVAS_DATA_HPP
