#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QPoint>
#include <QWidget>
#include <QElapsedTimer>
#include <layer.h>
#include <shape/shape.h>
#include <widgets/canvas-text-edit.h>
#include <command.h>

class Canvas;

class Document : public QObject {
Q_OBJECT
public:

  Document() noexcept;

  // Selecting functions

  QList<ShapePtr> &selections();

  void setSelection(nullptr_t);

  void setSelection(ShapePtr &shape);

  void setSelections(const QList<ShapePtr> &new_selections);

  // Test if any shape in the document hit by mouse
  ShapePtr hitTest(QPointF canvas_coord);

  // Layer functions:

  void addLayer(LayerPtr &layer);

  void removeLayer(LayerPtr &layer);

  // Paint functions

  void paint(QPainter *painter);

  // Dumps layers info
  void dumpStack(QList<LayerPtr> &stack);

  void reorderLayers(QList<LayerPtr> &new_order);

  // Group functions

  void groupSelections();

  void ungroupSelections();

  // Coordinate functions:
  QPointF getCanvasCoord(QPointF window_coord) const;

  QRectF screenRect() const;

  // Getters:

  QList<LayerPtr> &layers();

  Layer *activeLayer();

  QPointF scroll() const;

  qreal scale() const;

  qreal width() const;

  qreal height() const;

  QPointF mousePressedScreenCoord() const;

  QPointF mousePressedCanvasCoord() const;

  const QFont &font() const;

  const Canvas *canvas() const;

  // Frames rendered after start
  int framesCount() const;

  // Setters:
  bool setActiveLayer(const QString &name);

  void setActiveLayer(LayerPtr &layer);

  void setWidth(qreal width);

  void setHeight(qreal height);

  void setScroll(QPointF scroll);

  void setScale(qreal scale);

  void setCanvas(Canvas *canvas);

  void setMousePressedScreenCoord(QPointF screen_coord);

  void setFont(const QFont &font);

  void setScreenSize(QSize size);

  const LayerPtr *findLayerByName(const QString &layer_name);

  /* Undo functions */
  void undo();

  void redo();

  void execute(Commands::BaseCmd *cmd);

  void execute(const CmdPtr &cmd);

  void execute(initializer_list<CmdPtr> cmds);

  template<typename... Args>
  void execute(const CmdPtr cmd0, Args... args) {
    execute({cmd0, args...});
  }

  unique_ptr<CanvasTextEdit> text_box_;

signals:

  void selectionsChanged();

private:
  qreal scroll_x_;
  qreal scroll_y_;
  qreal scale_;
  qreal width_;
  qreal height_;

  bool screen_changed_;

  QList<LayerPtr> layers_;
  QList<ShapePtr> selections_;

  QFont font_;

  int new_layer_id_;
  Layer *active_layer_;

  QPointF mouse_pressed_screen_coord_;

  QList<CmdPtr> undo2_stack_;
  QList<CmdPtr> redo2_stack_;

  QSize screen_size_;

  int frames_count_;

  Canvas *canvas_;
};

#endif // DOCUMENT_H
