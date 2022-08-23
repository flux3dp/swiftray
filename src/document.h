#pragma once

#include <QPoint>
#include <QWidget>
#include <QElapsedTimer>
#include <QMutex>
#include <layer.h>
#include <document-settings.h>
#include <shape/shape.h>
#include <command.h>

class Canvas;

class DocumentSerializer;

/**
  \class Document
  \brief The Document class represents a store for layers, shapes, document settings and editing states.
*/
class Document : public QObject {
Q_OBJECT
public:

  Document() noexcept;

  // Selecting functions

  QList<ShapePtr> &selections();

  void setSelection(std::nullptr_t);

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

  void setLayersOrder(const QList<LayerPtr> &new_order);

  // Group functions

  void groupSelections();

  void ungroupSelections();

  // Coordinate functions:
  QPointF getCanvasCoord(QPointF window_coord) const;

  QRectF screenRect() const;

  // Getters:

  const QList<LayerPtr> &layers() const;

  Layer *activeLayer();

  QPointF scroll() const;

  qreal scale() const;

  qreal width() const;

  qreal height() const;

  QPointF mousePressedScreenCoord() const;

  QPointF mousePressedCanvasCoord() const;

  QPointF mousePressedCanvasScroll() const;

  const QFont &font() const;

  const Canvas *canvas() const;

  DocumentSettings &settings();

  QString currentFile() { return current_file_; }
  bool currentFileModified() { return current_file_modified_; }

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

  void setCurrentFile(QString filename);
  void clearCurrentFileModified();

  const LayerPtr *findLayerByName(const QString &layer_name);

  /* Undo functions */
  void undo();

  void redo();

  void execute(Commands::BaseCmd *cmd);

  void execute(const CmdPtr &cmd);

  void execute(std::initializer_list<CmdPtr> cmds);

  template<typename... Args>
  void execute(const CmdPtr cmd0, Args... args) {
    execute({cmd0, args...});
  }

  friend class DocumentSerializer;

Q_SIGNALS:

  void selectionsChanged();

  void scaleChanged();

  void fileModifiedChange(bool file_modified);

private:
  qreal scroll_x_;
  qreal scroll_y_;
  qreal scale_;
  qreal width_;  // the document width = canvas (mm) * 10
  qreal height_; // the document height = canvas (mm) * 10

  bool screen_changed_;

  QList<LayerPtr> layers_;
  QList<ShapePtr> selections_;
  QMutex layers_mutex_;

  QFont font_;

  int new_layer_id_;
  Layer *active_layer_;

  QPointF mouse_pressed_screen_coord_;
  QPointF mouse_pressed_scroll_coord_;

  QList<CmdPtr> undo2_stack_;
  QList<CmdPtr> redo2_stack_;
  QMutex undo_mutex_;
  QMutex redo_mutex_;

  QSize screen_size_;

  Canvas *canvas_;

  DocumentSettings settings_;

  QString current_file_;
  bool current_file_modified_;
};
