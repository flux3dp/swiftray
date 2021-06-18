#include <QPoint>
#include <QWidget>
#include <QElapsedTimer>
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

  QList<ShapePtr> &selections();

  QList<ShapePtr> &lastSelections();

  void setSelection(nullptr_t);

  void setSelection(ShapePtr &shape);

  void setSelections(const QList<ShapePtr> &new_selections);

  void emitAllChanges();

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

  // Group functions

  void groupSelections();

  void ungroupSelections();

  // Coordinate functions:
  QPointF getCanvasCoord(QPointF window_coord) const;

  QRectF screenRect(QSize screen_size) const;

  // Getters:
  Mode mode() const;

  QList<LayerPtr> &layers();

  LayerPtr &activeLayer();

  QPointF scroll() const;

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

  bool setActiveLayer(const QString &name);

  void setActiveLayer(LayerPtr &layer);

  void setWidth(qreal width);

  void setHeight(qreal height);

  void setRecordingUndo(bool recording_undo);

  void setScroll(QPointF scroll);

  void setScale(qreal scale);

  void setMousePressedScreenCoord(QPointF screen_coord);

  void setFont(QFont &font);

  /* Undo functions */
  void undo();

  void redo();

  void addUndoEvent(BaseUndoEvent *event);

  void addUndoEvent(const EventPtr &e);

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

  bool is_recording_undo_;

  QList<LayerPtr> layers_;
  QList<ShapePtr> selections_;
  QList<ShapePtr> last_selections_;

  QFont font_;

  Mode mode_;
  int new_layer_id_;
  LayerPtr active_layer_;

  QPointF mouse_pressed_screen_coord_;
  QElapsedTimer volatility_timer;
};

#endif // SCENE_H
