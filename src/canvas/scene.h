#include <QPoint>
#include <canvas/layer.h>
#include <shape/shape.h>

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
            DRAWING_RECT,
            DRAWING_LINE,
            DRAWING_OVAL,
            DRAWING_PATH,
            EDITING_PATH
        };
        Scene() noexcept;
        // Selection
        bool isSelected(ShapePtr shape);
        QList<ShapePtr> &selections();
        void setSelection(ShapePtr shape);
        void setSelections(QList<ShapePtr> &shapes);
        void clearAll();
        void clearSelections();
        void removeSelections();
        ShapePtr hitTest(QPointF canvas_coord);
        // Mode
        Mode mode();
        void setMode(Mode mode);
        // Clipboard
        QList<ShapePtr> &clipboard();
        void setClipboard(QList<ShapePtr> &items);
        // Layer
        QList<Layer> &layers();
        Layer &activeLayer();
        bool setActiveLayer(QString name);
        void addLayer();
        void removeLayer(QString name);
        // Undo
        void undo();
        void redo();
        void stackStep();

        // View
        QPointF getCanvasCoord(QPointF window_coord) const;
        QPointF scroll() const;
        qreal scrollX() const;
        qreal scrollY() const;
        qreal scale() const;
        qreal width() const;
        qreal height() const;

        void setWidth(qreal width);
        void setHeight(qreal height);
        void setScroll(QPointF scroll);
        void setScrollX(qreal scroll_x);
        void setScrollY(qreal scroll_y);
        void setScale(qreal scale);

        QList<ShapePtr> &shapes();

        QPointF pasting_shift;
    signals:
        void selectionsChanged();
        void layerChanged();
        void modeChanged();
    private:
        void stackRedo();
        void stackUndo();
        void dumpStack(QList<Layer> &stack);
        QList<Layer> cloneStack(QList<Layer> &stack);

        qreal scroll_x_;
        qreal scroll_y_;
        qreal scale_;
        qreal width_;
        qreal height_;

        QList<ShapePtr> shape_clipboard_;

        QList<QList<Layer>> undo_stack_;
        QList<QList<Layer>> redo_stack_;
        QList<Layer> layers_;
        QList<ShapePtr> selections_;
        
        Mode mode_;
        int new_layer_id_;
        Layer *active_layer_;
};

#endif // SCENE_H
