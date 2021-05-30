#include <QPoint>
#include <canvas/layer.hpp>
#include <shape/shape.hpp>

#ifndef SCENE_H
#define SCENE_H

class Scene : QObject {
        Q_OBJECT
    public:
        enum class Mode {
            SELECTING,
            MOVING,
            MULTI_SELECTING,
            TRANSFORMING,
            DRAWING_SQUARE,
            DRAWING_LINE,
            DRAWING_OVAL,
            DRAWING_PATH,
        };
        Scene() noexcept;
        bool isSelected(ShapePtr shape);
        QList<ShapePtr> &selections();

        Mode mode();
        void setMode(Mode mode);
        QPointF getCanvasCoord(QPointF window_coord) const;
        QPointF scroll() const;
        QList<Layer> &layers();
        void setSelection(ShapePtr shape);
        void setSelections(QList<ShapePtr> &shapes);
        void clearAll();
        void clearSelection();
        void clearSelectionNoFlag();
        void stackStep();
        QList<ShapePtr> &shapes();
        QList<ShapePtr> &clipboard();
        void setClipboard(QList<ShapePtr> &items);
        void undo();
        void redo();
        void addLayer();
        void removeLayer(QString name);
        Layer &activeLayer();

        qreal scroll_x;
        qreal scroll_y;
        qreal scale;

        QList<ShapePtr> shape_clipboard_;
        QPointF pasting_shift;
    signals:
        void selectionsChanged();
        void layerChanged();
    private:
        QList<QList<Layer>> undo_stack_;
        QList<QList<Layer>> redo_stack_;
        QList<Layer> layers_;
        QList<ShapePtr> selections_;
        Mode mode_;
        int new_layer_id_;
        Layer *active_layer_;
};

#endif // SCENE_H
