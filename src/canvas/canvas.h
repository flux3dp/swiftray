#pragma once

#include <QtQuick>
#include <QQuickWidget>
#include <canvas/controls/canvas-control.h>
#include <canvas/controls/grid.h>
#include <canvas/controls/ruler.h>
#include <canvas/controls/line.h>
#include <canvas/controls/select.h>
#include <canvas/controls/oval.h>
#include <canvas/controls/path-draw.h>
#include <canvas/controls/path-edit.h>
#include <canvas/controls/rect.h>
#include <canvas/controls/polygon.h>
#include <canvas/controls/text.h>
#include <canvas/controls/transform.h>
#include <constants.h>
#include <widgets/components/canvas-text-edit.h>
#include <document.h>
#include <clipboard.h>
#include <shape/shape.h>
#include <parser/svgpp-parser.h>

/**
  \class Canvas
  \brief The Canvas class represents a canvas that display shapes and handle interactive events with its controls

  The canvas should be designed to handle multiple documents,
  carefully choose what properties you want to put in the canvas,
  and what properties you want to put in the document.
*/
class Canvas : public QQuickPaintedItem {
Q_OBJECT
  QML_ELEMENT

public:
  enum class Mode {
    Selecting,
    Moving,
    MultiSelecting,
    Transforming,
    Rotating,
    RectDrawing,
    LineDrawing,
    PolygonDrawing,
    OvalDrawing,
    PathDrawing,
    PathEditing,
    TextDrawing
  };

  Canvas(QQuickItem *parent = 0);

  ~Canvas();

  void paint(QPainter *painter) override;

  void loop();

  void loadSVG(QByteArray &data, bool skip_confirm = false);

  void loadSVG(QString file_name);

  void loadDXF(QString file_name);

  void keyPressEvent(QKeyEvent *e) override;

  void keyReleaseEvent(QKeyEvent *e) override;

  void mousePressEvent(QMouseEvent *e) override;

  void mouseMoveEvent(QMouseEvent *e) override;

  void mouseReleaseEvent(QMouseEvent *e) override;

  void mouseDoubleClickEvent(QMouseEvent *e) override;

  void wheelEvent(QWheelEvent *e) override;

  bool event(QEvent *e) override;

  Document &document();

  Controls::Transform &transformControl() { return ctrl_transform_; }

  Clipboard &clipboard();

  Mode mode() const;

  const QFont &font() const;

  double lineHeight() const;

  CanvasTextEdit *textInput() const;

  // Graphics should be drawn in lower quality is this return true
  bool isVolatile() const;

  // Setters
  void setDocument(Document *document);

  void setMode(Mode mode);

  QRect calculateShapeBoundary();

  void setJobOrigin(bool use_job_origin);

  void setJobOrigin(QPointF job_origin);

  void setUserOrigin(QPointF user_origin);

  void setUserOrigin(bool user_origin) {show_user_origin_ = user_origin;}

  void setCurrentPosition(bool current_position) {current_position_ = current_position;}

  void updateCursor();

  void setHoverMove(bool in_canvas);

  void setCanvasQuality(CanvasQuality quality);

  void save(QDataStream &out);
  
public Q_SLOTS:

  void editCut();

  void editCopy();

  void editPaste();

  void editPasteInRightButton();

  void editPasteInPlace();

  void editDelete();

  void editDuplicate();

  void editUndo();

  void editRedo();

  void editSelectAll(bool with_hiden = false);

  void editClear();

  void editGroup();

  void editUngroup();

  void editDrawRect();

  void editDrawPolygon();

  void editDrawOval();

  void editDrawLine();

  void editDrawPath();

  void editDrawText();

  void editUnion();

  void editSubtract();

  void editIntersect();

  void editDifference();

  void editHFlip();

  void editVFlip();

  void editAlignHLeft();

  void editAlignHCenter();

  void editAlignHRight();

  void editAlignVTop();

  void editAlignVCenter();

  void editAlignVBottom();

  void addEmptyLayer();

  void duplicateLayer(const LayerPtr layer);

  void importImage(QImage image);

  void genPathOffset();

  void genImageTrace();

  void invertImage();

  void sharpenImage();

  void replaceImage(QImage new_image);
  
  void cropImage();

  void setActiveLayer(LayerPtr layer);

  void setLayerOrder(QList<LayerPtr> order);

  void setScaleWithCenter(qreal new_scale);

  void resize();

  void setFontFamily(QString font_family);

  void setPointSize(int point_size);

  void setLetterSpacing(double spacing);

  void setBold(bool bold);

  void setItalic(bool italic);

  void setUnderline(bool underline);

  void emitAllChanges();

  void setLineHeight(float line_height);

  void exitCurrentMode();

  void setWidget(QQuickWidget *widget);

  void updateCurrentPosition(std::tuple<qreal, qreal, qreal> target_pos);

  void canvasUpdated();

  void shapeUpdated();

  void setShapeReference(int reference_origin);

private:
  // Basic attributes
  std::unique_ptr<Document> doc_;
  Mode mode_;
  QFont font_;
  double line_height_;
  Clipboard clipboard_;
  Parser::SVGPPParser svgpp_parser_;

  // Control components
  Controls::Transform ctrl_transform_;
  Controls::Select ctrl_select_;
  Controls::Grid ctrl_grid_;
  Controls::Ruler ctrl_ruler_;
  Controls::Line ctrl_line_;
  Controls::Oval ctrl_oval_;
  Controls::PathDraw ctrl_path_draw_;
  Controls::PathEdit ctrl_path_edit_;
  Controls::Rect ctrl_rect_;
  Controls::Text ctrl_text_;
  Controls::Polygon ctrl_polygon_;
  QList<Controls::CanvasControl *> ctrls_;


  // Display attributes
  QPoint widget_offset_;
  QElapsedTimer volatility_timer;

  // Monitor attributes
  QElapsedTimer fps_timer;
  int fps_count;
  float fps;
  QTimer *timer;
  QThread *mem_thread_;
  bool is_in_canvas_ = false;
  bool is_holding_space_ = false;
  bool is_holding_ctrl_  = false;
  bool is_holding_middle_button_ = false;
  bool is_pop_menu_showing_;
  bool is_temp_scale_lock_;
  bool is_direction_lock_;
  bool use_job_origin_ = false;
  bool show_user_origin_ = true;
  bool current_position_ = true;
  double current_x_;
  double current_y_;
  QPointF right_click_;
  QPointF job_origin_;
  QPointF user_origin_;
  QPixmap canvas_tmpimage_;
  QPixmap canvas_image_;
  bool is_flushed_ = false;
  bool is_shape_flushed_ = false;
  bool is_on_shape_ = false;
  //the line width to display
  CanvasQuality canvas_quality_ = AutoQuality;
  unsigned int canvas_counter_ = 0;
  bool change_quality_ = false;

  QQuickWidget *widget_;

  const QColor backgroundColor();

  QPointF getTopLeftScrollBoundary();

  QPointF getBottomRightScrollBoundary();

  void updateScroll(QPointF scroll, QPointF ref_pos);

  friend class MainWindow;

protected:
  CanvasTextEdit *text_input_;

Q_SIGNALS:

  void canvasContextMenuOpened();

  void scaleChanged();

  void selectionsChanged(QList<ShapePtr> shape_list);

  void fileModifiedChange(bool file_modified);

  void layerChanged();

  void modeChanged();

  void docSettingsChanged();

  void undoCalled();

  void redoCalled();

  void transformChanged(qreal x, qreal y, qreal r, qreal w, qreal h);

  void cursorChanged(Qt::CursorShape cursor);

  void syncJobOrigin();
};
