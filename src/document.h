#include <QPoint>
#include <QWidget>
#include <layer.h>
#include <shape/shape.h>
#include <widgets/canvas-text-edit.h>
#include <undo.h>

#ifndef SCENE_H
#define SCENE_H


class Document : public QObject {
Q_OBJECT
public:
  enum class Mode {
    Selecting,
    Moving,
    MultiSelecting,
    Transforming,
    Rotating,
    RectDrawing,
    LineDrawing,
    OvalDrawing,
    PathDrawing,
    PathEditing,
    TextDrawing
  };

  Document() noexcept;

  // Selecting functions
  bool isSelected(ShapePtr &shape) const;

  QList<ShapePtr> &selections();

  void setSelection(ShapePtr &shape);

  void setSelections(const QList<ShapePtr> &shapes);

  void emitAllChanges();

  void clearAll();

  void clearSelections();

  void removeSelections();

  // Test if any shape in the document hit by mouse
  ShapePtr hitTest(QPointF canvas_coord);

  // Layer functions:
  void addLayer();

  void addLayer(LayerPtr &layer);

  void removeLayer(LayerPtr &layer);

  // Dumps layers info
  void dumpStack(QList<LayerPtr> &stack);

  void reorderLayers(QList<LayerPtr> &new_order);

  // Coordinate functions:
  QPointF getCanvasCoord(QPointF window_coord) const;

  QRectF screenRect(QSize screen_size) const;

  // Getters:
  Mode mode() const;

  QList<LayerPtr> &layers();

  LayerPtr &activeLayer();

  QPointF scroll() const;

  qreal scrollX() const;

  qreal scrollY() const;

  qreal scale() const;

  qreal width() const;

  qreal height() const;

  QPointF mousePressedScreenCoord() const;

  QPointF mousePressedCanvasCoord() const;

  const QFont &font() const;

  // Graphics should be drawn in lower quality is this return true
  bool isVolatile();

  // Setters:
  void setMode(Mode mode);

  bool setActiveLayer(QString name);

  bool setActiveLayer(LayerPtr &layer);

  void setWidth(qreal width);

  void setHeight(qreal height);

  void setScroll(QPointF scroll);

  void setScrollX(qreal scroll_x);

  void setScrollY(qreal scroll_y);

  void setScale(qreal scale);

  void setMousePressedScreenCoord(QPointF screen_coord);

  void setFont(QFont &font);

  /* Undo functions */
  void undo();

  void redo();

  void addUndoEvent(BaseUndoEvent *event);

  QList<EventPtr> undo2;
  QList<EventPtr> redo2;

  unique_ptr<CanvasTextEdit> text_box_;

signals:

  void selectionsChanged();

  void layerChanged();

  void modeChanged();


private:
  qreal scroll_x_;
  qreal scroll_y_;
  qreal scale_;
  qreal width_;
  qreal height_;

  QList<LayerPtr> layers_;
  QList<ShapePtr> selections_;

  QFont font_;

  Mode mode_;
  int new_layer_id_;
  LayerPtr active_layer_;

  QPointF mouse_pressed_screen_coord_;
};

#endif // SCENE_H
