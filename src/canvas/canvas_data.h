#include <QPoint>
#include <canvas/layer.h>
#include <shape/shape.h>

#ifndef CANVAS_DATA_HPP
#define CANVAS_DATA_HPP

class Scene : QObject {
        Q_OBJECT
    public:
        enum class Mode {
            SELECTING,
            MOVING,
            MULTI_SELECTING,
            TRANSFORMING
        };
        Scene() noexcept;
        qreal scroll_x;
        qreal scroll_y;
        qreal scale;
        bool isSelected(ShapePtr shape);
        QList<ShapePtr> &selections();

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
        Layer &activeLayer() {
            if (layers_.size() == 0) layers_ << Layer();

            return layers_.first();
        }

        QList<Layer> &layers() {
            return layers_;
        }
        void setSelection(ShapePtr shape);
        void setSelections(QList<ShapePtr> &shapes);
        void clear();
        void clearSelection();
        void clearSelectionNoFlag();
        void stackStep();
        QList<ShapePtr> &shapes();
        QList<ShapePtr> &clipboard();
        void setClipboard(QList<ShapePtr> &items);
        void undo();
        void redo();
        QList<ShapePtr> shape_clipboard_;
        QPointF pasting_shift;
    signals:
        void selectionsChanged();
    private:
        QList<QList<Layer>> undo_stack_;
        QList<QList<Layer>> redo_stack_;
        QList<Layer> layers_;
        QList<ShapePtr> selections_;
        Mode mode_;
};

#endif // CANVAS_DATA_HPP
