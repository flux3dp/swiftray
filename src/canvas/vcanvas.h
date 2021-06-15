#ifndef VCANVAS_H
#define VCANVAS_H

#include <QtQuick>
#include <canvas/controls/canvas-control.h>
#include <canvas/controls/grid.h>
#include <canvas/controls/line.h>
#include <canvas/controls/select.h>
#include <canvas/controls/oval.h>
#include <canvas/controls/path-draw.h>
#include <canvas/controls/path-edit.h>
#include <canvas/controls/rect.h>
#include <canvas/controls/text.h>
#include <canvas/controls/transform.h>
#include <document.h>
#include <parser/svgpp-parser.h>
#include <shape/shape.h>
#include <canvas/memory-monitor.h>

class VCanvas : public QQuickPaintedItem {
Q_OBJECT
  QML_ELEMENT

public:
  VCanvas(QQuickItem *parent = 0);

  ~VCanvas();

  void paint(QPainter *painter) override;

  void loop();

  void loadSVG(QByteArray &data);

  void keyPressEvent(QKeyEvent *e) override;

  void mousePressEvent(QMouseEvent *e) override;

  void mouseMoveEvent(QMouseEvent *e) override;

  void mouseReleaseEvent(QMouseEvent *e) override;

  void mouseDoubleClickEvent(QMouseEvent *e) override;

  void wheelEvent(QWheelEvent *e) override;

  bool event(QEvent *e) override;

  Document &document();

public Q_SLOTS:

  void editCut();

  void editCopy();

  void editPaste();

  void editDelete();

  void editUndo();

  void editRedo();

  void editSelectAll();

  void editGroup();

  void editUngroup();

  void editDrawRect();

  void editDrawOval();

  void editDrawLine();

  void editDrawPath();

  void editDrawText();

  void editUnion();

  void editSubtract();

  void editIntersect();

  void editDifference();

  void importImage(QImage &image);

  void setActiveLayer(LayerPtr &layer);

  void setLayerOrder(QList<LayerPtr> &order);

  void fitWindow();

  void setFont(const QFont &font);

  void exportGcode();

  void setScreenSize(QSize size);

  void setScreenOffset(QPoint offset);

  Controls::Transform &transformControl() {
    return ctrl_transform_;
  }

private:
  bool ready;
  int counter;
  Document scene_;
  SVGPPParser svgpp_parser_;
  Controls::Transform ctrl_transform_;
  Controls::Select ctrl_select_;
  Controls::Grid ctrl_grid_;
  Controls::Line ctrl_line_;
  Controls::Oval ctrl_oval_;
  Controls::PathDraw ctrl_path_draw_;
  Controls::PathEdit ctrl_path_edit_;
  Controls::Rect ctrl_rect_;
  Controls::Text ctrl_text_;
  QList<Controls::CanvasControl *> ctrls_;

  QTimer *timer;
  QPointF paste_shift_;


  QTime fps_timer;
  int fps_count;
  float fps;
  QRectF screen_rect_;
  QPoint screen_offset_;
  QSize screen_size_;
  QString mem_info_;
  QThread *mem_thread_;
  MemoryMonitor mem_monitor_;

signals:

  void rightAlignedChanged();
};

#endif // VCANVAS_H
