#include <canvas/canvas.h>
#include <QQuickWidget>
#include <QCursor>
#include <QDebug>
#include <QHoverEvent>
#include <QPainter>
#include <QMessageBox>
#include <constants.h>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/group-shape.h>
#include <shape/path-shape.h>
#include <toolpath_exporter/toolpath-exporter.h>
#include <windows/preview-window.h>
#include <windows/osxwindow.h>
#include <document-serializer.h>
#include <windows/path-offset-dialog.h>
#include <windows/image-sharpen-dialog.h>
#include <windows/image-trace-dialog.h>
#include <windows/image-crop-dialog.h>
#include <settings/file-path-settings.h>
#include <QFileDialog>
#include <QAbstractButton>
#include <QPushButton>
#include "parser/dxf_reader.h"

#include <private/qsvgtinydocument_p.h>
#include "parser/my_qsvg_handler.h"

Canvas::Canvas(QQuickItem *parent)
     : QQuickPaintedItem(parent),
       ctrl_transform_(Controls::Transform(this)),
       ctrl_select_(Controls::Select(this)),
       ctrl_grid_(Controls::Grid(this)),
       ctrl_ruler_(Controls::Ruler(this)),
       ctrl_line_(Controls::Line(this)),
       ctrl_oval_(Controls::Oval(this)),
       ctrl_path_draw_(Controls::PathDraw(this)),
       ctrl_path_edit_(Controls::PathEdit(this)),
       ctrl_rect_(Controls::Rect(this)),
       ctrl_polygon_(Controls::Polygon(this)),
       ctrl_text_(Controls::Text(this)),
       svgpp_parser_(Parser::SVGPPParser()),
       widget_(nullptr),
       fps(0),
       font_(QFont(FONT_TYPE, FONT_SIZE, QFont::Bold)),
       line_height_(LINE_HEIGHT),
       timer(new QTimer(this)),
       mem_thread_(new QThread(this)) {

  setRenderTarget(RenderTarget::FramebufferObject);
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true);
  setAcceptTouchEvents(true);
  setAntialiasing(true);
  setOpaquePainting(true);

  // Set document & mode
  setDocument(new Document());
  setMode(Mode::Selecting);

  // Set main loop and timers
  connect(timer, &QTimer::timeout, this, &Canvas::loop);
  timer->start(16);//16 for about 60 fps
  volatility_timer.start();

  // Register controls
  ctrls_ << &ctrl_transform_ << &ctrl_select_ << &ctrl_rect_ << &ctrl_polygon_ << &ctrl_oval_
         << &ctrl_line_ << &ctrl_path_draw_ << &ctrl_path_edit_
         << &ctrl_text_;

  // FPS
  fps_count = 0;
  fps_timer.start();

  // Register events
  connect(this, &Canvas::selectionsChanged, [=](QList<ShapePtr> shape_list) {
    for (auto &layer : document().layers()) {
      layer->flushCache();
    }
  });

  connect(this, &QQuickPaintedItem::widthChanged, this, &Canvas::resize);

  connect(this, &QQuickPaintedItem::heightChanged, this, &Canvas::resize);
  connect(&ctrl_transform_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_select_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_rect_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_polygon_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_oval_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_line_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_path_draw_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_path_edit_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_text_, &Controls::CanvasControl::canvasUpdated, this, &Canvas::canvasUpdated);
  connect(&ctrl_transform_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
  connect(&ctrl_select_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
  connect(&ctrl_rect_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
  connect(&ctrl_polygon_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
  connect(&ctrl_oval_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
  connect(&ctrl_line_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
  connect(&ctrl_path_draw_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
  connect(&ctrl_path_edit_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
  connect(&ctrl_text_, &Controls::CanvasControl::shapeUpdated, this, &Canvas::shapeUpdated);
}

Canvas::~Canvas() {
  mem_thread_->requestInterruption();
  mem_thread_->wait(1300);
}

void Canvas::loadSVG(QByteArray &svg_data) {
  // TODO(Add undo events for loading svg)
  QElapsedTimer t;
  t.start();
  QList<LayerPtr> svg_layers;
  bool success = svgpp_parser_.parse(&document(), svg_data, &svg_layers);
  setAntialiasing(true);

  if (success) {
    QList<ShapePtr> all_shapes;
    for (auto &layer : svg_layers) {
      all_shapes.append(layer->children());
    }
    document().setSelections(all_shapes);
    double scale = 254 / 72.0;
    transformControl().applyScale(QPointF(0,0), scale, scale, false);
    if (all_shapes.size() == 1) {
      document().setActiveLayer(all_shapes.first()->layer()->name());
      Q_EMIT layerChanged();
    }

    forceActiveFocus();
    emitAllChanges();
    update();
  }
  qInfo() << "[Parser] Took" << t.elapsed();
}

void Canvas::loadSVG(QString file_name) {
  QFile file(file_name);
  if (!file.open(QFile::ReadOnly)) {
      return;
  }

  // QSvgTinyDocument *doc = 0;
  QList<LayerPtr> svg_layers;
  MyQSvgHandler::ReadType read_type;
  QMessageBox msgBox;
  msgBox.setText(tr("Select layering style:"));
  QAbstractButton *byLayerButton = dynamic_cast<QAbstractButton*>(msgBox.addButton(tr("Layer"), QMessageBox::AcceptRole));
  QAbstractButton *byColorButton = dynamic_cast<QAbstractButton*>(msgBox.addButton(tr("Color"), QMessageBox::AcceptRole));
  //QMessageBox need one RejectRole
  QAbstractButton *singleLayerButton = dynamic_cast<QAbstractButton*>(msgBox.addButton(tr("Single Layer"), QMessageBox::RejectRole));
  int ret = msgBox.exec();
  if (msgBox.clickedButton() == byColorButton) {
    read_type = MyQSvgHandler::ByColors;
  } else if (msgBox.clickedButton() == singleLayerButton) {
    read_type = MyQSvgHandler::InSingleLayer;
  } else {
    read_type = MyQSvgHandler::ByLayers;
  }
  MyQSvgHandler handler(&file, &document(), &svg_layers, read_type);
  if (handler.ok()) {
    handler.document();
    handler.animationDuration();
    QList<ShapePtr> all_shapes;
    for (auto &layer : svg_layers) {
      all_shapes.append(layer->children());
    }
    document().setSelections(all_shapes);
    if (all_shapes.size() == 1) {
      document().setActiveLayer(all_shapes.first()->layer()->name());
      Q_EMIT layerChanged();
    }

    forceActiveFocus();
    emitAllChanges();
    update();
  } else {
    delete handler.document();
  }
}

void Canvas::loadDXF(QString file_name) {
  DXFReader dxf_reader;
  QList<LayerPtr> dxf_layers;
  DXFReader::ReadType read_type;
  QMessageBox msgBox;
  msgBox.setText(tr("Select layering style:"));
  QAbstractButton *byLayerButton = dynamic_cast<QAbstractButton*>(msgBox.addButton(tr("Layer"), QMessageBox::AcceptRole));
  QAbstractButton *byColorButton = dynamic_cast<QAbstractButton*>(msgBox.addButton(tr("Color"), QMessageBox::AcceptRole));
  //QMessageBox need one RejectRole
  QAbstractButton *singleLayerButton = dynamic_cast<QAbstractButton*>(msgBox.addButton(tr("Single Layer"), QMessageBox::RejectRole));
  int ret = msgBox.exec();
  if (msgBox.clickedButton() == byColorButton) {
    read_type = DXFReader::ByColors;
  } else if (msgBox.clickedButton() == singleLayerButton) {
    read_type = DXFReader::InSingleLayer;
  } else {
    read_type = DXFReader::ByLayers;
  }
  dxf_reader.openFile(file_name, dxf_layers, read_type);
  QList<ShapePtr> all_shapes;
  for (auto &layer : dxf_layers) {
    all_shapes.append(layer->children());
    document().addLayer(layer);
  }
  document().setSelections(all_shapes);
  if (all_shapes.size() == 1) {
    document().setActiveLayer(all_shapes.first()->layer()->name());
    Q_EMIT layerChanged();
  }

  forceActiveFocus();
  emitAllChanges();
  update();
}

void Canvas::paint(QPainter *painter) {
  // return;
  // QElapsedTimer timer;
  // timer.start();
  Q_EMIT syncJobOrigin();
  painter->setRenderHint(QPainter::RenderHint::Antialiasing, fps > 30);
  painter->save();
  if(document().isScreenChanged() || !is_flushed_ || !is_shape_flushed_) {
    is_flushed_ = true;
    if(document().isScreenChanged() || !is_shape_flushed_) {
      is_shape_flushed_ = true;
      canvas_tmpimage_ = QPixmap(width()*2, height()*2);
      canvas_tmpimage_.setDevicePixelRatio(2);
      // canvas_tmpimage_.fill(QColor(0,0,0,0));
      QPainter image_painter(&canvas_tmpimage_);
      image_painter.fillRect(0, 0, width(), height(), backgroundColor());
      // Move to scroll and scale
      image_painter.translate(document().scroll());
      image_painter.scale(document().scale(), document().scale());
      ctrl_grid_.paint(&image_painter);
      document().paintUnselected(&image_painter);
    }

    canvas_image_ = QPixmap(canvas_tmpimage_);
    QPainter canvas_painter(&canvas_image_);
    canvas_painter.translate(document().scroll());
    canvas_painter.scale(document().scale(), document().scale());
    document().paintSelected(&canvas_painter);
  }
  painter->drawPixmap(0, 0, canvas_image_);

  painter->translate(document().scroll());
  painter->scale(document().scale(), document().scale());
  for (auto &control : ctrls_) {
    if (control->isActive()) {
      control->paint(painter);
    }
  }
  double rect_width = width() / document().scale() / 100;
  if(show_user_origin_) {
    painter->fillRect(user_origin_.x() * 10 - rect_width/2, user_origin_.y() * 10 - rect_width/2, rect_width, rect_width, Qt::magenta);
  }
  if(current_position_) {
    painter->fillRect(current_x_ * 10 - rect_width/2, current_y_ * 10 - rect_width/2, rect_width, rect_width, Qt::red);
  }
  if(use_job_origin_) {
    painter->fillRect(job_origin_.x() - rect_width/2, job_origin_.y() - rect_width/2, rect_width, rect_width, Qt::green);
  }

  painter->restore();
  ctrl_ruler_.paint(painter);

  // qDebug() << Q_FUNC_INFO << "Rendering (" << timer.elapsed() << "ms)";
}

void Canvas::keyPressEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key::Key_Space) {
    is_holding_space_ = true;
  }

  if (e->modifiers() & Qt::ControlModifier) {
    qInfo() << (e->modifiers() & Qt::ControlModifier);
    is_holding_ctrl_ = e->modifiers() & Qt::ControlModifier;
  }

  if (is_holding_ctrl_) {
    if (e->key() == Qt::Key_Equal || e->key() == Qt::Key_Plus) {
      setScaleWithCenter(qreal(qRound((document().scale() + 0.0051)*100))/100);
    } else if (e->key() == Qt::Key_Minus) {
      setScaleWithCenter(qreal(qRound((document().scale() - 0.0051)*100))/100);
    }
  }

  if (e->modifiers() & Qt::ShiftModifier) {
    is_temp_scale_lock_ = true;
    ctrl_transform_.setScaleLock(!ctrl_transform_.isScaleLock());
  }

  if (e->modifiers() & Qt::ShiftModifier) {
    is_direction_lock_ = true;
    ctrl_transform_.setDirectionLock(e->modifiers() & Qt::ShiftModifier);
    ctrl_line_.setDirectionLock(e->modifiers() & Qt::ShiftModifier);
    ctrl_path_draw_.setDirectionLock(e->modifiers() & Qt::ShiftModifier);
  }

  for (auto &control : ctrls_) {
    if (control->isActive() && control->keyPressEvent(e))
      return;
  }

  if (e->key() == Qt::Key::Key_Delete || e->key() == Qt::Key::Key_Backspace ||
      e->key() == Qt::Key::Key_Back) {
    editDelete();
  }

  if (e->key() == Qt::Key::Key_Escape) {
    document().setSelection(nullptr);
  }
}

void Canvas::keyReleaseEvent(QKeyEvent *e) {
  if (e->key() == Qt::Key::Key_Space) {
    is_holding_space_ = false;
  }

  if (is_holding_ctrl_) {
    qInfo() << (e->modifiers() & Qt::ControlModifier);
    is_holding_ctrl_ = e->modifiers() & Qt::ControlModifier;
  }

  if (is_temp_scale_lock_) {
    is_temp_scale_lock_ = false;
    ctrl_transform_.setScaleLock(!ctrl_transform_.isScaleLock());
  }

  if (is_direction_lock_) {
    is_direction_lock_ = false;
    ctrl_transform_.setDirectionLock(e->modifiers() & Qt::ShiftModifier);
    ctrl_line_.setDirectionLock(e->modifiers() & Qt::ShiftModifier);
    ctrl_path_draw_.setDirectionLock(e->modifiers() & Qt::ShiftModifier);
  }

  for (auto &control : ctrls_) {
    if (control->isActive() && control->keyReleaseEvent(e))
      return;
  }
}

void Canvas::mousePressEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  document().setMousePressedScreenCoord(e->pos());
  qInfo() << "Mouse Press (screen)" << e->pos() << " -> (canvas)"
          << canvas_coord;

  if (e->button()==Qt::MiddleButton) {
    Q_EMIT cursorChanged(Qt::ClosedHandCursor);
    is_holding_middle_button_ = true;
    return;
  }
  if (e->button()==Qt::RightButton) {
    is_pop_menu_showing_ = true;
    return;
  }
  if (is_holding_space_) {
    Q_EMIT cursorChanged(Qt::ClosedHandCursor);
  }

  for (auto &control : ctrls_) {
    if (control->isActive() && control->mousePressEvent(e))
      return;
  }

  if (mode() == Mode::Selecting && is_on_shape_) {
    ShapePtr hit = document().hitTest(canvas_coord, true);

    if (hit != nullptr) {
      Q_EMIT cursorChanged(Qt::ClosedHandCursor);
      if (!hit->selected()) {
        document().setSelection(hit);
        document().setActiveLayer(hit->layer()->name());
        Q_EMIT layerChanged();
      }
    }
  } else if(mode() == Mode::Selecting) {
    document().setSelection(nullptr);
    setMode(Mode::MultiSelecting);
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *e) {
  if (is_pop_menu_showing_) {
    return;
  }

  if (is_holding_space_ || is_holding_middle_button_) {
    QPointF movement = (document().getCanvasCoord(e->pos()) - document().mousePressedCanvasCoord()) 
                        * document().scale();
    QPointF new_scroll = document().mousePressedCanvasScroll() + movement;

    updateScroll(new_scroll, movement);
    volatility_timer.restart();
    return; 
  }
  for (auto &control : ctrls_) {
    if (control->isActive() && control->mouseMoveEvent(e))
      return;
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *e) {
  if (e->button()==Qt::RightButton) {
    right_click_ = e->pos();
    is_pop_menu_showing_ = true;
    Q_EMIT canvasContextMenuOpened();
    return;
  }
  if (e->button()==Qt::RightButton && !is_pop_menu_showing_) {
    is_pop_menu_showing_ = true;
    return;
  }

  if (e->button()==Qt::MiddleButton) {
    is_holding_middle_button_ = false;
    updateCursor();
    return;
  }

  if (is_pop_menu_showing_) {
    is_pop_menu_showing_ = false;
    return;
  }

  for (auto &control : ctrls_) {
    if (control->isActive() && control->mouseReleaseEvent(e))
      return;
  }

  setMode(Mode::Selecting);
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  ShapePtr hit = document().hitTest(canvas_coord, true);
  if (mode() == Mode::Selecting) {
    if (hit != nullptr) {
      switch (hit->type()) {
        case Shape::Type::Text:
          qInfo() << "[Canvas] Double clicked text" << hit.get();
          document().setSelection(hit);
          document().setActiveLayer(hit->layer()->name());
          Q_EMIT layerChanged();
          if (document().activeLayer()->isLocked()) {
            break;
          }
          ctrl_text_.setTarget(hit);
          ((TextShape*) hit.get())->setEditing(true);
          is_flushed_ = false;
          setMode(Mode::TextDrawing);
          break;
        case Shape::Type::Path:
          qInfo() << "[Canvas] Double clicked path" << hit.get();
          document().setSelection(nullptr);
          if (document().activeLayer()->isLocked()) {
            break;
          }
          ctrl_path_edit_.setTarget(hit);
          setMode(Mode::PathEditing);
          break;
        default:
          break;
      }
    }
  } else if (mode() == Mode::PathEditing) {
    ctrl_path_edit_.exit();
  } else if (mode() == Mode::TextDrawing) {
    // NOTE: If Double click outside of text edit box, finish the current text edit
    if (!ctrl_text_.target().boundingRect().contains(canvas_coord)) {
      ctrl_text_.exit();
    }
  }
}

/**
 *
 * @return  upper bound value (positive value) for document scroll
 */
QPointF Canvas::getTopLeftScrollBoundary() {

  qreal scroll_x_max =
       1 * std::max((width() - document().width() * document().scale()) / 2, document().width() * document().scale());
  qreal scroll_y_max =
       1 * std::max((height() - document().height() * document().scale()) / 2, document().height() * document().scale());

  return QPointF{scroll_x_max, scroll_y_max};
}

/**
 *
 * @return  lower bound value (negative value) for document scroll
 */
QPointF Canvas::getBottomRightScrollBoundary() {
  qreal scroll_x_min =
       (-1) * std::max(0.5 * document().width() * document().scale(), 2 * document().width() * document().scale() - width());
  qreal scroll_y_min = (-1) * std::max(0.5 * document().height() * document().scale(),
                                 2 * document().height() * document().scale() - height());

  return QPointF{scroll_x_min, scroll_y_min};
}


void Canvas::updateScroll(
    QPointF new_scroll, QPointF ref_pos) 
{
  // Restrict the range of scroll
  QPointF top_left_bound = getTopLeftScrollBoundary();
  QPointF bottom_right_bound = getBottomRightScrollBoundary();
  if (ref_pos.x() > 0 && new_scroll.x() > top_left_bound.x()) {
    new_scroll.setX(top_left_bound.x());
  } else if (ref_pos.x() < 0 && new_scroll.x() < bottom_right_bound.x()) {
    new_scroll.setX(bottom_right_bound.x());
  }
  if (ref_pos.y() > 0 && new_scroll.y() > top_left_bound.y()) {
    new_scroll.setY(top_left_bound.y());
  } else if (ref_pos.y() < 0 && new_scroll.y() < bottom_right_bound.y()) {
    new_scroll.setY(bottom_right_bound.y());
  }
  // Update
  document().setScroll(new_scroll);
}


void Canvas::wheelEvent(QWheelEvent *e) {
  QPointF new_scroll;
  QPointF mouse_pos;

  if (is_holding_ctrl_) {
    mouse_pos = e->position() - widget_offset_;
    double orig_scale = document().scale();
    double new_scale = std::min(30.0, std::max(0.01, document().scale() + e->angleDelta().y() / 8 / document().height()));
    document().setScale(new_scale);
    new_scroll = mouse_pos - (mouse_pos - document().scroll()) * document().scale() / orig_scale;
  } else {
    new_scroll.setX(document().scroll().x() + e->angleDelta().x() / 8 / 2.5);
    new_scroll.setY(document().scroll().y() + e->angleDelta().y() / 8 / 2.5);
    mouse_pos = e->angleDelta();
  }

  updateScroll(new_scroll, mouse_pos);
  volatility_timer.restart();
}

void Canvas::setScaleWithCenter(qreal new_scale) {
  QPointF center_pos = QPointF(width()/2 + 20, height()/2 + 20);
  qreal orig_scale = document().scale();
  document().setScale(new_scale);

  QPointF new_scroll = center_pos - (center_pos - document().scroll()) * document().scale() / orig_scale;

  updateScroll(new_scroll, center_pos);
  volatility_timer.restart();
}

void Canvas::loop() {
  update();//update paint & event(HoverMove)
}

bool Canvas::event(QEvent *e) {
  // qInfo() << "QEvent" << e;
  QNativeGestureEvent *nge;
  Qt::CursorShape cursor;
  switch (e->type()) {
    case QEvent::HoverMove:
      if(!is_in_canvas_ || is_holding_middle_button_) return QQuickPaintedItem::event(e);
      for (auto &control : ctrls_) {
        if (control->isActive() &&
            control->hoverEvent(dynamic_cast<QHoverEvent *>(e), &cursor)) {
          // TODO (Hack this to mainwindow support global cursor)
          // Local cursor has a bug..
          Q_EMIT cursorChanged(cursor);
          return QQuickPaintedItem::event(e);
        }
      }
      if(mode_ == Mode::Selecting) {
        QPointF canvas_coord = document().getCanvasCoord(dynamic_cast<QHoverEvent *>(e)->pos());
        ShapePtr hit = document().hitTest(canvas_coord);
        if(hit != nullptr) {
          is_on_shape_ = true;
          Q_EMIT cursorChanged(Qt::OpenHandCursor);
        } else {
          is_on_shape_ = false;
          Q_EMIT cursorChanged(Qt::ArrowCursor);
        }
      } else if (mode_ == Mode::PathEditing) {
        Q_EMIT cursorChanged(Qt::ArrowCursor);
      }
      break;
    case QEvent::NativeGesture:
      nge = dynamic_cast<QNativeGestureEvent *>(e);

      //  (passed by main window)
      if (nge->gestureType() == Qt::ZoomNativeGesture) {
        QPoint mouse_pos = nge->localPos().toPoint() - widget_offset_;
        double orig_scale = document().scale();
        double new_scale = std::min(30.0, std::max(0.01, document().scale() + nge->value() / 8));
        document().setScale(new_scale);
        QPointF new_scroll = mouse_pos - (mouse_pos - document().scroll()) * document().scale() / orig_scale;

        updateScroll(new_scroll, mouse_pos);
        volatility_timer.restart();
      }
      break;
    default:
      break;
  }

  return QQuickPaintedItem::event(e);
}

void Canvas::editCut() {
  if (mode() != Mode::Selecting)
    return;
  clipboard().cutFrom(document());
}

void Canvas::editCopy() {
  if (mode() != Mode::Selecting)
    return;
  clipboard().set(document().selections());
}

void Canvas::editPaste() {
  clipboard().pasteTo(document());
}

void Canvas::editPasteInRightButton() {
  clipboard().pasteTo(document(), right_click_);
}

void Canvas::editPasteInPlace() {
  clipboard().pasteInPlace(document());
}

void Canvas::editDelete() {
  if (mode() != Mode::Selecting)
    return;

  document().execute(
       Commands::RemoveSelections(&document())
  );
}

void Canvas::editDuplicate() {
  if (mode() != Mode::Selecting)
    return;
  clipboard().set(document().selections());
  clipboard().pasteTo(document());
}

void Canvas::editUndo() {
  QElapsedTimer t;
  t.start();
  document().undo();
  Q_EMIT layerChanged(); // TODO (Check if layers are really changed)
  Q_EMIT selectionsChanged(document().selections()); // Force refresh all selection related components
  Q_EMIT undoCalled();
  qInfo() << "[Undo] Took" << t.elapsed() << "ms";
}

void Canvas::editRedo() {
  document().redo();
  Q_EMIT layerChanged(); // TODO (Check if layers are really changed)
  Q_EMIT selectionsChanged(document().selections()); // Force refresh all selection related components
  Q_EMIT redoCalled();
}

void Canvas::editDrawRect() {
  if(mode() != Mode::RectDrawing) exitCurrentMode();
  if (document().activeLayer()->isLocked()) {
    Q_EMIT modeChanged();
    return;
  }
  document().setSelection(nullptr);
  setMode(Mode::RectDrawing);
}

void Canvas::editDrawPolygon() {
  if(mode() != Mode::PolygonDrawing) exitCurrentMode();
  if (document().activeLayer()->isLocked()) {
    Q_EMIT modeChanged();
    return;
  }
  document().setSelection(nullptr);
  setMode(Mode::PolygonDrawing);
}

void Canvas::editDrawOval() {
  if(mode() != Mode::OvalDrawing) exitCurrentMode();
  if (document().activeLayer()->isLocked()) {
    Q_EMIT modeChanged();
    return;
  }
  document().setSelection(nullptr);
  setMode(Mode::OvalDrawing);
}

void Canvas::editDrawLine() {
  if(mode() != Mode::LineDrawing) exitCurrentMode();
  if (document().activeLayer()->isLocked()) {
    Q_EMIT modeChanged();
    return;
  }
  document().setSelection(nullptr);
  setMode(Mode::LineDrawing);
}

void Canvas::editDrawPath() {
  if(mode() != Mode::PathDrawing) exitCurrentMode();
  if (document().activeLayer()->isLocked()) {
    Q_EMIT modeChanged();
    return;
  }
  document().setSelection(nullptr);
  setMode(Mode::PathDrawing);
}

void Canvas::editDrawText() {
  if(mode() != Mode::TextDrawing) exitCurrentMode();
  if (document().activeLayer()->isLocked()) {
    Q_EMIT modeChanged();
    return;
  }
  document().setSelection(nullptr);
  setMode(Mode::TextDrawing);
}

void Canvas::editClear() {
  editSelectAll(true);
  document().execute(
    Commands::RemoveSelections(&document())
  );
}

void Canvas::editSelectAll(bool with_hiden) {
  if (mode() != Mode::Selecting)
    return;
  QList<ShapePtr> all_shapes;

  for (auto &layer : document().layers()) {
    if(layer->isVisible() || with_hiden) all_shapes.append(layer->children());
  }

  document().setSelections(all_shapes);
  if (all_shapes.size() == 1) {
    document().setActiveLayer(all_shapes.first()->layer()->name());
    Q_EMIT layerChanged();
  }
}

void Canvas::editGroup() {
  qInfo() << "Edit Group";
  document().groupSelections();
}

void Canvas::editUngroup() {
  qInfo() << "Edit Ungroup";
  document().ungroupSelections();
}

void Canvas::editUnion() {
  assert(document().selections().size() > 0);
  QPainterPath result;

  for (auto &shape : document().selections()) {
    auto polygons = shape->transform().map(dynamic_cast<PathShape *>(shape.get())->path()).toSubpathPolygons();
    for (QPolygonF &poly : polygons) {
      QPainterPath p;
      p.addPolygon(poly);
      result = result.united(p);
    }
  }

  ShapePtr new_shape = std::make_shared<PathShape>(result);
  document().execute(
       Commands::AddShape(document().activeLayer(), new_shape),
       Commands::RemoveSelections(&document()),
       Commands::Select(&document(), {new_shape})
  );
}

void Canvas::editSubtract() {
  assert(document().selections().size() == 2);

  auto *a = dynamic_cast<PathShape *>(document().selections().at(0).get());
  auto *b = dynamic_cast<PathShape *>(document().selections().at(1).get());
  QPainterPath new_path(a->transform().map(a->path()).subtracted(
       b->transform().map(b->path())));
  auto new_shape = std::make_shared<PathShape>(new_path);
  document().execute(
       Commands::AddShape(document().activeLayer(), new_shape),
       Commands::RemoveSelections(&document()),
       Commands::Select(&document(), {new_shape})
  );
}

void Canvas::editIntersect() {
  assert(document().selections().size() == 2);

  auto *a = dynamic_cast<PathShape *>(document().selections().at(0).get());
  auto *b = dynamic_cast<PathShape *>(document().selections().at(1).get());
  QPainterPath new_path(a->transform().map(a->path()).intersected(
       b->transform().map(b->path())));
  new_path.closeSubpath();
  ShapePtr new_shape = std::make_shared<PathShape>(new_path);
  document().execute(
       Commands::AddShape(document().activeLayer(), new_shape),
       Commands::RemoveSelections(&document()),
       Commands::Select(&document(), {new_shape})
  );
}

void Canvas::editDifference() {
  assert(document().selections().size() == 2);

  auto *a = dynamic_cast<PathShape *>(document().selections().at(0).get());
  auto *b = dynamic_cast<PathShape *>(document().selections().at(1).get());

  QPainterPath result;
  QPainterPath intersection(a->transform().map(a->path()).intersected(
       b->transform().map(b->path())));
  intersection.closeSubpath();

  for (auto &shape : document().selections()) {
    auto polygons = shape->transform().map(dynamic_cast<PathShape *>(shape.get())->path()).toSubpathPolygons();
    for (QPolygonF &poly : polygons) {
      QPainterPath p;
      p.addPolygon(poly);
      result = result.united(p);
    }
  }

  ShapePtr new_shape = std::make_shared<PathShape>(result.subtracted(intersection));

  document().execute(
       Commands::AddShape(document().activeLayer(), new_shape),
       Commands::RemoveSelections(&document()),
       Commands::Select(&document(), {new_shape})
  );
}

void Canvas::addEmptyLayer() {
  int i = 1;
  while (document().findLayerByName(tr("Layer ") + QString::number(i)) != nullptr) i++;
  LayerPtr new_layer = std::make_shared<Layer>(&document(), i);
  document().execute(
       Commands::AddLayer(new_layer)
  );
  Q_EMIT layerChanged();
}

void Canvas::duplicateLayer(LayerPtr layer) {
  // NOTE: Unselect current shapes first to prevent selection box from malfunctioning
  document().execute(
          Commands::Select(&document(), {})
  );
  int i = 1;
  while (document().findLayerByName(layer->name() + " copy " + QString::number(i)) != nullptr) i++;
  LayerPtr new_layer = layer->clone();
  new_layer->setName(layer->name() + " copy " + QString::number(i));
  document().execute(Commands::AddLayer(new_layer));

  setActiveLayer(new_layer);
  Q_EMIT layerChanged();
}

void Canvas::resize() {
  if (widget_) {
    document().setScreenSize(widget_->geometry().size());
    widget_offset_ = widget_->parentWidget()->mapToParent(widget_->geometry().topLeft());
  }

  qreal proper_scale = std::min((width() - 100) / document().width(),
                           (height() - 100) / document().height());
  QPointF proper_translate =
       QPointF((width() - document().width() * proper_scale) / 2,
               (height() - document().height() * proper_scale) / 2);
  document().setScale(proper_scale);
  document().setScroll(proper_translate);
}

void Canvas::importImage(QImage &image) {
#ifdef Q_OS_IOS
  if (image.width() > 500) {
    float scale = 500.0 / image.width();
    qInfo() << "Scale" << scale << "Size" << image.size();
    image = image.scaled(image.width() * scale, image.height() * scale);
  }
#endif
  // NOTE: Force a consistent format conversion first (to 32-bit = 4-byte format).
  // Otherwise, we should handle each kind of format later for each image processing
  if (image.format() != QImage::Format_ARGB32) {
    image = image.convertToFormat(QImage::Format_ARGB32);
  }

  // Process ARGB values into grayscale value and alpha value
  for (int yy = 0; yy < image.height(); yy++) {
    uchar *scan = image.scanLine(yy);
    int depth = 4; // 32-bit = 4-byte
    for (int xx = 0; xx < image.width(); xx++) {
      QRgb *rgb_pixel = reinterpret_cast<QRgb *>(scan + xx * depth);
      int luma = qRound(0.299*qRed(*rgb_pixel) + 0.587*qGreen(*rgb_pixel) + 0.114*qBlue(*rgb_pixel));
      int alpha = qAlpha(*rgb_pixel);
      int gray = qRound(255.0 - alpha/255.0 * (255 - luma));
      *rgb_pixel = qRgba(gray, gray, gray, alpha);
    }
  }

  ShapePtr new_shape = std::make_shared<BitmapShape>(image);
  qreal scale = std::min(1.0, std::min(document().height() / image.height(),
                             document().width() / image.width()));
  qInfo() << "Scale" << scale;
  document().execute(
    Commands::SetTransform(new_shape.get(), QTransform().scale(scale, scale)),
    Commands::AddShape(document().activeLayer(), new_shape),
    Commands::Select(&(document()), {new_shape})
  );
}

/**
 * @brief Popout a dialog for param setting, and then generate the path offset accordingly
 *        MUST assert only path shape are selected
 */
void Canvas::genPathOffset() {
  QList<ShapePtr> &shapes = document().selections();
  PathOffsetDialog *dialog = new PathOffsetDialog(QSizeF{document().width(), document().height()});
  // initialize dialog
  for (auto &shape : shapes) {
    auto path_shape_ptr = dynamic_cast<PathShape *>(shape.get());
    auto polygons = path_shape_ptr->path().toSubpathPolygons(shape->transform());
    for (QPolygonF &poly : polygons) {
      dialog->addPath(poly);
    }
  }
  dialog->updatePathOffset();

  // output result
  int dialogRet = dialog->exec();
  if(dialogRet == QDialog::Accepted) {
    QPainterPath p;
    for (auto polygon :dialog->getResult()) {
      p.addPolygon(polygon);
    }
    ShapePtr new_shape = std::make_shared<PathShape>(p);
    document().execute(
            Commands::AddShape(document().activeLayer(), new_shape),
            Commands::Select(&document(), {new_shape})
    );
  }
  delete dialog;
}

/**
 * @brief Popout a dialog for param setting, and then generate the image trace accordingly
 *        MUST assert only one image is selected currently
 */
void Canvas::genImageTrace() {
  QList<ShapePtr> &items = document().selections();
  Q_ASSERT_X(items.count() == 1, "actionTrace", "MUST only be enabled when single item is selected");
  Q_ASSERT_X(items.at(0)->type() == Shape::Type::Bitmap, "actionTrace", "MUST only be enabled when an image is selected");

  ShapePtr selected_img_shape = items.at(0);
  BitmapShape * bitmap = static_cast<BitmapShape *>(selected_img_shape.get());
  ImageTraceDialog *dialog = new ImageTraceDialog();
  dialog->loadImage(bitmap->sourceImage());
  int dialog_ret = dialog->exec();
  if(dialog_ret == QDialog::Accepted) {
    // Add trace contours to canvas
    ShapePtr new_shape = std::make_shared<PathShape>(dialog->getTrace());
    new_shape->applyTransform(bitmap->transform());
    if (dialog->shouldDeleteImg()) {
      document().execute(
              Commands::AddShape(document().activeLayer(), new_shape),
              Commands::Select(&(document()), {new_shape}),
              Commands::RemoveShape(selected_img_shape)
      );
    } else {
      document().execute(
              Commands::AddShape(document().activeLayer(), new_shape),
              Commands::Select(&(document()), {new_shape})
      );
    }
  }
  delete dialog;
}

void Canvas::invertImage() {
  Q_ASSERT_X(document().selections().length() == 1,
             "Canvas", "Only one image can be processed at a time");
  ShapePtr origin_bitmap_shape = document().selections().at(0);
  Layer* target_layer = origin_bitmap_shape->layer();
  Q_ASSERT_X(origin_bitmap_shape->type() == Shape::Type::Bitmap,
             "Canvas", "invert action can only be applied on bitmap shape");
  ShapePtr inverted_bitmap_shape = origin_bitmap_shape->clone();
  BitmapShape * bitmap = static_cast<BitmapShape *>(inverted_bitmap_shape.get());
  bitmap->invertPixels();
  document().execute(
      Commands::Select(&document(), {}),
      Commands::RemoveShape(origin_bitmap_shape->layer(), origin_bitmap_shape),
      Commands::AddShape(target_layer, inverted_bitmap_shape),
      Commands::Select(&document(), {inverted_bitmap_shape})
  );
}

/**
 * @brief Popout a dialog for param setting, and then sharpen the image accordingly
 *        MUST assert only one image is selected currently
 */
void Canvas::sharpenImage() {
  QList<ShapePtr> &items = document().selections();
  Q_ASSERT_X(items.count() == 1, "actionSharpen", "MUST only be enabled when single item is selected");
  Q_ASSERT_X(items.at(0)->type() == Shape::Type::Bitmap, "actionSharpen", "MUST only be enabled when an image is selected");

  ShapePtr selected_img_shape = items.at(0);
  BitmapShape * bitmap = static_cast<BitmapShape *>(selected_img_shape.get());
  ImageSharpenDialog *dialog = new ImageSharpenDialog();
  dialog->reset();
  dialog->loadImage(bitmap->sourceImage());
  int dialogRet = dialog->exec();
  if(dialogRet == QDialog::Accepted) {
    ShapePtr new_shape = std::make_shared<BitmapShape>(dialog->getSharpenedImage());
    new_shape->applyTransform(bitmap->transform());
    document().execute(
            Commands::AddShape(document().activeLayer(), new_shape),
            Commands::Select(&(document()), {new_shape}),
          Commands::RemoveShape(selected_img_shape)
    );
  }
  dialog->reset();
  delete dialog;
}

void Canvas::replaceImage(QImage new_image) {
  Q_ASSERT_X(document().selections().length() == 1,
             "Canvas", "Only one image can be processed at a time");
  ShapePtr origin_shape = document().selections().at(0);
  Layer* target_layer = origin_shape->layer();
  Q_ASSERT_X(origin_shape->type() == Shape::Type::Bitmap,
             "Canvas", "invert action can only be applied on bitmap shape");
  BitmapShape * origin_bitmap_shape = static_cast<BitmapShape *>(origin_shape.get());

  // Apply the height, width, x_pos, y_pos to new image
  new_image = new_image.scaled(origin_bitmap_shape->sourceImage().size());
  ShapePtr new_bitmap_shape = std::make_shared<BitmapShape>(new_image);
  new_bitmap_shape->setTransform(origin_bitmap_shape->transform());

  // Remove old image, Add new image
  document().execute(
          Commands::Select(&document(), {}),
          Commands::RemoveShape(origin_shape->layer(), origin_shape),
          Commands::AddShape(target_layer, new_bitmap_shape),
          Commands::Select(&document(), {new_bitmap_shape})
  );
}

void Canvas::cropImage() {
  Q_ASSERT_X(document().selections().length() == 1,
             "Canvas", "Only one image can be processed at a time");
  ShapePtr selected_bitmap_shape = document().selections().at(0);
  Layer* target_layer = selected_bitmap_shape->layer();
  Q_ASSERT_X(selected_bitmap_shape->type() == Shape::Type::Bitmap,
             "Canvas", "Crop action can only be applied on bitmap shape");

  BitmapShape * bitmap = static_cast<BitmapShape *>(selected_bitmap_shape.get());

  // Popout a dialog for crop
  ImageCropDialog *dialog = new ImageCropDialog();
  dialog->loadImage(bitmap->sourceImage());
  int dialog_ret = dialog->exec();
  // Apply crop result
  if(dialog_ret == QDialog::Accepted) {
    // Add trace contours to canvas
    ShapePtr new_shape = std::make_shared<BitmapShape>(dialog->getCropImage());
    new_shape->applyTransform(bitmap->transform());
    document().execute(
            Commands::AddShape(document().activeLayer(), new_shape),
            Commands::Select(&(document()), {new_shape}),
            Commands::RemoveShape(selected_bitmap_shape)
    );
  }
  delete dialog;
}

void Canvas::setActiveLayer(LayerPtr &layer) {
  document().setActiveLayer(layer);
  Q_EMIT layerChanged();
}

void Canvas::setLayerOrder(QList<LayerPtr> &new_order) {
  document().execute(
       Commands::SetRef<Document, QList<LayerPtr>, &Document::layers, &Document::setLayersOrder>(
            doc_.get(), new_order
       )
  );
}

void Canvas::setFontFamily(QString font_family) {
  font_.setFamily(font_family);
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        QFont target_font = t->font();
        target_font.setFamily(font_family);
        cmd << Commands::SetFont((TextShape *) shape.get(), target_font);
      }
    }
    document().execute(cmd);
    Q_EMIT selectionsChanged(document().selections());
  } else {
    if(!ctrl_text_.isEmpty()) {
      ctrl_text_.target().setFont(font_);
    }
  }
}

void Canvas::setPointSize(int point_size) {
  QFont target_font = font_;
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        target_font = t->font();
        target_font.setPointSize(point_size);
        cmd << Commands::SetFont((TextShape *) shape.get(), target_font);
      }
    }
    document().execute(cmd);
    Q_EMIT selectionsChanged(document().selections());
  } else {
    target_font.setPointSize(point_size);
    if(!ctrl_text_.isEmpty()) {
      ctrl_text_.target().setFont(target_font);
    }
  }
  font_ = target_font;
}

void Canvas::setLetterSpacing(double spacing) {
  QFont target_font = font_;
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        target_font = t->font();
        target_font.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, spacing);
        cmd << Commands::SetFont((TextShape *) shape.get(), target_font);
      }
    }
    document().execute(cmd);
    Q_EMIT selectionsChanged(document().selections());
  } else {
    target_font.setLetterSpacing(QFont::SpacingType::AbsoluteSpacing, spacing);
    if(!ctrl_text_.isEmpty()) {
      ctrl_text_.target().setFont(target_font);
    }
  }
  font_ = target_font;
}

void Canvas::setBold(bool bold) {
  QFont target_font = font_;
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        target_font = t->font();
        target_font.setBold(bold);
        cmd << Commands::SetFont((TextShape *) shape.get(), target_font);
      }
    }
    document().execute(cmd);
    Q_EMIT selectionsChanged(document().selections());
  } else {
    target_font.setBold(bold);
    if(!ctrl_text_.isEmpty()) {
      ctrl_text_.target().setFont(target_font);
    }
  }
  font_ = target_font;
}

void Canvas::setItalic(bool italic) {
  QFont target_font = font_;
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        target_font = t->font();
        target_font.setItalic(italic);
        cmd << Commands::SetFont((TextShape *) shape.get(), target_font);
      }
    }
    document().execute(cmd);
    Q_EMIT selectionsChanged(document().selections());
  } else {
    target_font.setItalic(italic);
    if(!ctrl_text_.isEmpty()) {
      ctrl_text_.target().setFont(target_font);
    }
  }
  font_ = target_font;
}

void Canvas::setUnderline(bool underline) {
  QFont target_font = font_;
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        auto *t = (TextShape *) shape.get();
        target_font = t->font();
        target_font.setUnderline(underline);
        cmd << Commands::SetFont((TextShape *) shape.get(), target_font);
      }
    }
    document().execute(cmd);
    Q_EMIT selectionsChanged(document().selections());
  } else {
    target_font.setUnderline(underline);
    if(!ctrl_text_.isEmpty()) {
      ctrl_text_.target().setFont(target_font);
    }
  }
  font_ = target_font;
}

void Canvas::setLineHeight(float line_height) {
  line_height_ = line_height;
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        cmd << Commands::SetLineHeight((TextShape *) shape.get(), line_height);
      }
    }
    document().execute(cmd);
    Q_EMIT selectionsChanged(document().selections());//to update the boundingRect maybe use different Q_SIGNALS?
  } else {
    if(!ctrl_text_.isEmpty()) {
      ctrl_text_.target().setLineHeight(line_height);
    }
  }
}

Clipboard &Canvas::clipboard() {
  return clipboard_;
}

void Canvas::exitCurrentMode() {
  // TODO (Add exit function to all controls)
  switch (mode()) {
    case Mode::TextDrawing:
      ctrl_text_.exit();
      break;
    case Mode::OvalDrawing:
      ctrl_oval_.exit();
      break;
    case Mode::RectDrawing:
      ctrl_rect_.exit();
      break;
    case Mode::PolygonDrawing:
      ctrl_polygon_.exit();
      break;
    case Mode::LineDrawing:
      ctrl_line_.exit();
      break;
    case Mode::PathDrawing:
      ctrl_path_draw_.exit();
      break;
    case Mode::PathEditing:
      ctrl_path_edit_.exit();
      break;
  }
}

Document &Canvas::document() { return *doc_.get(); }

void Canvas::setDocument(Document *document) {
  if(document == nullptr) {
    QMessageBox msgbox;
    msgbox.setText(tr("File Open Error"));
    msgbox.setInformativeText(tr("Please update the Swiftray version"));
    msgbox.exec();
    return;
  }
  doc_ = std::unique_ptr<Document>(document);
  doc_->setCanvas(this);
  resize();
  connect(doc_.get(), &Document::selectionsChanged, this, &Canvas::selectionsChanged);
  connect(doc_.get(), &Document::scaleChanged, this, &Canvas::scaleChanged);
  connect(doc_.get(), &Document::fileModifiedChange, this, &Canvas::fileModifiedChange);
}

Canvas::Mode Canvas::mode() const { return mode_; }

void Canvas::setMode(Mode mode) {
  mode_ = mode;
  updateCursor();
  Q_EMIT modeChanged();
}

void Canvas::updateCursor() {
  if(!is_in_canvas_) {
    Q_EMIT cursorChanged(Qt::ArrowCursor);
    return;
  }
  switch (mode_) {
    case Mode::TextDrawing:
      Q_EMIT cursorChanged(Qt::IBeamCursor);
      break;
    case Mode::Rotating:
    case Mode::RectDrawing:
    case Mode::LineDrawing:
    case Mode::PolygonDrawing:
    case Mode::OvalDrawing:
    case Mode::PathDrawing:
      Q_EMIT cursorChanged(Qt::CrossCursor);
      break;
    case Mode::Moving:
      Q_EMIT cursorChanged(Qt::ClosedHandCursor);
      break;
    default:
      break;
  }
}

QRect Canvas::calculateShapeBoundary() {
  double x_min = -1, x_max = -1, y_min = -1, y_max = -1;
  for (auto &layer : document().layers()) {
    double x, y;
    if (!layer->isVisible()) {
      continue;
    }
    for (auto &shape : layer->children()) {
      x = shape->boundingRect().left();
      y = shape->boundingRect().top();
      if (x_min == -1 && x_max == -1) {
        x_min = x;
        x_max = x;
      } else if (x < x_min) {
        x_min = x;
      } else if (x > x_max) {
        x_max = x;
      }
      if (y_min == -1 && y_max == -1) {
        y_min = y;
        y_max = y;
      } else if (y < y_min) {
        y_min = y;
      } else if (y > y_max) {
        y_max = y;
      }
      x = shape->boundingRect().right();
      y = shape->boundingRect().bottom();
      if (x < x_min) {
        x_min = x;
      } else if (x > x_max) {
        x_max = x;
      }
      if (y < y_min) {
        y_min = y;
      } else if (y > y_max) {
        y_max = y;
      }
    }
  }
  return QRect(QPoint(x_min, y_min), QPoint(x_max, y_max));
}

void Canvas::setJobOrigin(bool use_job_origin) {
  use_job_origin_ = use_job_origin;
}

void Canvas::setJobOrigin(QPointF job_origin) {
  job_origin_ = job_origin;
}

void Canvas::setUserOrigin(QPointF user_origin) {
  user_origin_ = user_origin;
}

void Canvas::emitAllChanges() {
  Q_EMIT scaleChanged();
  Q_EMIT layerChanged();
  Q_EMIT modeChanged();
  Q_EMIT docSettingsChanged();
}

bool Canvas::isVolatile() const {
  if (volatility_timer.elapsed() < 1000) { return true; }
  return mode_ == Mode::Moving || mode_ == Mode::Rotating || mode_ == Mode::Transforming;
}

const QFont &Canvas::font() const { return font_; }

double Canvas::lineHeight() const { return line_height_;}

void Canvas::editHFlip() {
  transformControl().applyScale(transformControl().boundingRect().center(), -1, 1, false);
  Q_EMIT selectionsChanged(document().selections());
}

void Canvas::editVFlip() {
  transformControl().applyScale(transformControl().boundingRect().center(), 1, -1, false);
  Q_EMIT selectionsChanged(document().selections());
}

void Canvas::editAlignHLeft() {
  auto cmd = Commands::Joined();
  double left = transformControl().boundingRect().left();
  for (auto &shape : document().selections()) {
    QTransform new_transform = QTransform().translate(left - shape->boundingRect().left(), 0);
    cmd << Commands::SetTransform(shape.get(), shape->transform() * new_transform);
  }
  document().execute(cmd);
  Q_EMIT selectionsChanged(document().selections());
}

void Canvas::editAlignHCenter() {
  auto cmd = Commands::Joined();
  double center_x = transformControl().boundingRect().center().x();
  for (auto &shape : document().selections()) {
    QTransform new_transform = QTransform().translate(center_x - shape->boundingRect().center().x(), 0);
    cmd << Commands::SetTransform(shape.get(), shape->transform() * new_transform);
  }
  document().execute(cmd);
  Q_EMIT selectionsChanged(document().selections());
}

void Canvas::editAlignHRight() {
  auto cmd = Commands::Joined();
  double right = transformControl().boundingRect().right();
  for (auto &shape : document().selections()) {
    QTransform new_transform = QTransform().translate(right - shape->boundingRect().right(), 0);
    cmd << Commands::SetTransform(shape.get(), shape->transform() * new_transform);
  }
  document().execute(cmd);
  Q_EMIT selectionsChanged(document().selections());
}

void Canvas::editAlignVTop() {
  auto cmd = Commands::Joined();
  double top = transformControl().boundingRect().top();
  for (auto &shape : document().selections()) {
    QTransform new_transform = QTransform().translate(0, top - shape->boundingRect().top());
    cmd << Commands::SetTransform(shape.get(), shape->transform() * new_transform);
  }
  document().execute(cmd);
  Q_EMIT selectionsChanged(document().selections());
}

void Canvas::editAlignVCenter() {
  auto cmd = Commands::Joined();
  double center_y = transformControl().boundingRect().center().y();
  for (auto &shape : document().selections()) {
    QTransform new_transform = QTransform().translate(0, center_y - shape->boundingRect().center().y());
    cmd << Commands::SetTransform(shape.get(), shape->transform() * new_transform);
  }
  document().execute(cmd);
  Q_EMIT selectionsChanged(document().selections());
}

void Canvas::editAlignVBottom() {
  auto cmd = Commands::Joined();
  double bottom = transformControl().boundingRect().bottom();
  for (auto &shape : document().selections()) {
    QTransform new_transform = QTransform().translate(0, bottom - shape->boundingRect().bottom());
    cmd << Commands::SetTransform(shape.get(), shape->transform() * new_transform);
  }
  document().execute(cmd);
  Q_EMIT selectionsChanged(document().selections());
}

void Canvas::setWidget(QQuickWidget *widget) {
  widget_ = widget;
}

CanvasTextEdit *Canvas::textInput() const {
  return text_input_;
}

void Canvas::save(QDataStream &out) {
  DocumentSerializer ds(out);
  ds.serializeDocument(document());
}

const QColor Canvas::backgroundColor() {
  if (isDarkMode()) return QColor("#454545");
  return QColor("#F0F0F0");
}

void Canvas::updateCurrentPosition(std::tuple<qreal, qreal, qreal> target_pos) {
  current_x_ = std::get<0>(target_pos);
  current_y_ = std::get<1>(target_pos);
}

void Canvas::canvasUpdated() {
  is_flushed_ = false;
}

void Canvas::shapeUpdated() {
  is_shape_flushed_ = false;
}

void Canvas::setHoverMove(bool in_canvas) {
  is_in_canvas_ = in_canvas;
  updateCursor();
}