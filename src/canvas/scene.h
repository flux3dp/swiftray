#include <QPoint>
#include <QWidget>
#include <canvas/layer.h>
#include <shape/shape.h>
#include <widgets/canvas_text_edit.h>

#ifndef SCENE_H
#define SCENE_H

class Scene : public QObject {
    Q_OBJECT
  public:
    enum class Mode {
        SELECTING,
        MOVING,
        MULTI_SELECTING,
        TRANSFORMING,
        ROTATING,
        DRAWING_RECT,
        DRAWING_LINE,
        DRAWING_OVAL,
        DRAWING_PATH,
        DRAWING_TEXT,
        EDITING_PATH
    };
    Scene() noexcept;
    // Selection
    bool isSelected(ShapePtr &shape) const;
    QList<ShapePtr> &selections();
    void setSelection(ShapePtr &shape);
    void setSelections(const QList<ShapePtr> &shapes);
    void emitAllChanges();
    void clearAll();
    void clearSelections();
    void removeSelections();
    ShapePtr hitTest(QPointF canvas_coord);
    // Mode
    Mode mode() const;
    void setMode(Mode mode);
    // Clipboard
    const QList<ShapePtr> &clipboard() const;
    void setClipboard(QList<ShapePtr> &items);
    void clearClipboard();
    // Layer
    QList<LayerPtr> &layers();
    LayerPtr& activeLayer();
    bool setActiveLayer(QString name);
    bool setActiveLayer(LayerPtr &layer);
    void addLayer();
    void addLayer(LayerPtr &layer);
    void removeLayer(QString name);
    void dumpStack(QList<LayerPtr> &stack);
    void reorderLayers(QList<LayerPtr> &new_order);

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
    QPointF mousePressedScreenCoord() const;
    QPointF mousePressedCanvasCoord() const;

    void setWidth(qreal width);
    void setHeight(qreal height);
    void setScroll(QPointF scroll);
    void setScrollX(qreal scroll_x);
    void setScrollY(qreal scroll_y);
    void setScale(qreal scale);
    void setMousePressedScreenCoord(QPointF screen_coord);

    void setFont(QFont &font);
    const QFont& font() const;

    unique_ptr<CanvasTextEdit> text_box_;
  signals:
    void selectionsChanged();
    void layerChanged();
    void modeChanged();

  private:
    void stackRedo();
    void stackUndo();
    QList<LayerPtr> cloneStack(QList<LayerPtr> &stack);

    qreal scroll_x_;
    qreal scroll_y_;
    qreal scale_;
    qreal width_;
    qreal height_;

    QList<ShapePtr> shape_clipboard_;

    QList<QList<LayerPtr>> undo_stack_;
    QList<QList<LayerPtr>> redo_stack_;
    QList<LayerPtr> layers_;
    QList<ShapePtr> selections_;

    QFont font_;

    Mode mode_;
    int new_layer_id_;
    LayerPtr active_layer_;

    QPointF mouse_pressed_screen_coord_;
};

#endif // SCENE_H
