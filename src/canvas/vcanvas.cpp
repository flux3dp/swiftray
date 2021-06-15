#include <canvas/vcanvas.h>
#include <QQuickWidget>
#include <QCursor>
#include <QDebug>
#include <QHoverEvent>
#include <QPainter>
#include <QWidget>
#include <cstring>
#include <iostream>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/group-shape.h>
#include <shape/path-shape.h>
#include <boost/range/adaptor/reversed.hpp>
#include <gcode/toolpath-exporter.h>
#include <gcode/generators/gcode-generator.h>

VCanvas::VCanvas(QQuickItem *parent)
     : QQuickPaintedItem(parent), svgpp_parser_(SVGPPParser(document())),
       ctrl_transform_(Controls::Transform(document())),
       ctrl_select_(Controls::Select(document())),
       ctrl_grid_(Controls::Grid(document())), ctrl_line_(Controls::Line(document())),
       ctrl_oval_(Controls::Oval(document())),
       ctrl_path_draw_(Controls::PathDraw(document())),
       ctrl_path_edit_(Controls::PathEdit(document())),
       ctrl_rect_(Controls::Rect(document())), ctrl_text_(Controls::Text(document())),
       paste_shift_(QPointF()) {
  setRenderTarget(RenderTarget::FramebufferObject);
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true);
  setAcceptTouchEvents(true);
  setAntialiasing(true);
  setOpaquePainting(true);
  // Set main loop
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &VCanvas::loop);
  timer->start(16);
  // Set memory monitor
  mem_thread_ = new QThread(this);
  mem_monitor_.moveToThread(mem_thread_);
  mem_thread_->start();
  QTimer::singleShot(0, &mem_monitor_, &MemoryMonitor::doWork);

  // Set mode
  document().setMode(Document::Mode::Selecting);
  // Register controls
  ctrls_ << &ctrl_transform_ << &ctrl_select_ << &ctrl_rect_ << &ctrl_oval_
         << &ctrl_line_ << &ctrl_path_draw_ << &ctrl_path_edit_
         << &ctrl_text_;
  // FPS
  fps_count = 0;
  fps_timer.start();

  qInfo() << "[VCanvas] Rendering target = " << this->renderTarget();
}

VCanvas::~VCanvas() {
  mem_thread_->requestInterruption();
  mem_thread_->wait(1300);
}

void VCanvas::loadSVG(QByteArray &svg_data) {
  // document().clearAll();
  // document().addLayer();
  bool success = svgpp_parser_.parse(svg_data);
  setAntialiasing(true);

  if (success) {
    editSelectAll();
    document().stackStep();
    forceActiveFocus();
    ready = true;
    update();
  }
}

void VCanvas::paint(QPainter *painter) {
  painter->save();
  painter->fillRect(0, 0, width(), height(), QColor("#F0F0F0"));
  // Move to scroll and scale
  painter->translate(document().scroll());
  painter->scale(document().scale(), document().scale());

  ctrl_grid_.paint(painter);

  bool do_flush_cache = false;
  if (screen_rect_ != document().screenRect(screen_size_)) {
    screen_rect_ = document().screenRect(screen_size_);
    do_flush_cache = true;
  }

  int object_count = 0;

  for (const LayerPtr &layer : document().layers()) {
    if (do_flush_cache) layer->flushCache();
    object_count += layer->paint(painter, screen_rect_, counter);
  }

  for (auto &control : ctrls_) {
    if (control->isActive()) {
      control->paint(painter);
    }
  }

  painter->restore();
  // Calculate FPS
  fps = (fps * 4 + float(++fps_count) * 1000 / fps_timer.elapsed()) / 5;
  painter->setRenderHint(QPainter::RenderHint::Antialiasing, fps > 30);
  painter->setPen(Qt::black);
  painter->drawText(QPointF(10, 20), "FPS " + QString::number(round(fps * 100) / 100.0));
  painter->drawText(QPointF(10, 40), "Objects " + QString::number(object_count));
  painter->drawText(QPointF(10, 60), "Mem " + mem_monitor_.system_info_);
  if (fps_timer.elapsed() > 3000) {
    fps_count = 0;
    fps_timer.restart();
  }
}

void VCanvas::keyPressEvent(QKeyEvent *e) {
  // qInfo() << "Key press" << e;

  for (auto &control : ctrls_) {
    if (control->isActive() && control->keyPressEvent(e))
      return;
  }

  if (e->key() == Qt::Key::Key_Delete || e->key() == Qt::Key::Key_Backspace ||
      e->key() == Qt::Key::Key_Back) {
    editDelete();
  }
}

void VCanvas::mousePressEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  document().setMousePressedScreenCoord(e->pos());
  qInfo() << "Mouse Press (screen)" << e->pos() << " -> (canvas)"
          << canvas_coord;

  for (auto &control : ctrls_) {
    if (control->isActive() && control->mousePressEvent(e))
      return;
  }

  if (document().mode() == Document::Mode::Selecting) {
    ShapePtr hit = document().hitTest(canvas_coord);

    if (hit != nullptr) {
      if (!hit->selected()) {
        document().setSelection(hit);
      }
    } else {
      document().clearSelections();
      document().setMode(Document::Mode::MultiSelecting);
    }
  }
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  // qInfo() << "Mouse Move (screen)" << e->pos() << " -> (canvas)" <<
  // canvas_coord;

  for (auto &control : ctrls_) {
    if (control->isActive() && control->mouseMoveEvent(e))
      return;
  }
}

void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  qInfo() << "Mouse Release (screen)" << e->pos() << " -> (canvas)"
          << canvas_coord;

  for (auto &control : ctrls_) {
    if (control->isActive() && control->mouseReleaseEvent(e))
      return;
  }

  document().setMode(Document::Mode::Selecting);
}

void VCanvas::mouseDoubleClickEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  qInfo() << "Mouse Double Click (screen)" << e->pos() << " -> (canvas)"
          << canvas_coord;
  qInfo() << "Mode" << (int) document().mode();
  ShapePtr hit = document().hitTest(canvas_coord);
  if (document().mode() == Document::Mode::Selecting) {
    if (hit != nullptr) {
      qInfo() << "Double clicked" << hit.get();
      switch (hit->type()) {
        case Shape::Type::Path:
          document().clearSelections();
          ctrl_path_edit_.setTarget(hit);
          document().setMode(Document::Mode::PathEditing);
          break;
        case Shape::Type::Text:
          document().clearSelections();
          ctrl_text_.setTarget(hit);
          document().setMode(Document::Mode::TextDrawing);
          break;
        default:
          break;
      }
    }
  } else if (document().mode() == Document::Mode::PathEditing) {
    ctrl_path_edit_.endEditing();
  }
}

void VCanvas::wheelEvent(QWheelEvent *e) {
  document().setScroll(document().scroll() + e->pixelDelta() / 2.5);
  qInfo() << "Wheel Event" << e->pixelDelta();
}

void VCanvas::loop() {
  counter++;
  update();
}

bool VCanvas::event(QEvent *e) {
  // qInfo() << "QEvent" << e;
  QNativeGestureEvent *nge;
  Qt::CursorShape cursor;

  switch (e->type()) {
    case QEvent::HoverMove:
      unsetCursor();
      for (auto &control : ctrls_) {
        if (control->isActive() &&
            control->hoverEvent(static_cast<QHoverEvent *>(e), &cursor)) {
          setCursor(cursor);
          break;
        }
      }

      break;

    case QEvent::NativeGesture:
      nge = static_cast<QNativeGestureEvent *>(e);

      //  (passed by main window)
      if (nge->gestureType() == Qt::ZoomNativeGesture) {
        QPoint mouse_pos = nge->localPos().toPoint() - screen_offset_;
        double orig_scale = document().scale();
        document().setScale(max(0.01, document().scale() + nge->value() / 2));
        document().setScroll(mouse_pos - (mouse_pos - document().scroll()) * document().scale() / orig_scale);
      }

      break;

    default:
      break;
  }

  return QQuickPaintedItem::event(e);
}

void VCanvas::editCut() {
  if (document().mode() != Document::Mode::Selecting)
    return;
  document().stackStep();
  qInfo() << "Edit Cut";
  editCopy();
  document().removeSelections();
}

void VCanvas::editCopy() {
  if (document().mode() != Document::Mode::Selecting)
    return;
  qInfo() << "Edit Copy";
  document().clearClipboard();
  document().setClipboard(document().selections());
  paste_shift_ = QPointF(0, 0);
}

void VCanvas::editPaste() {
  if (document().mode() != Document::Mode::Selecting)
    return;
  document().stackStep();
  qInfo() << "Edit Paste";
  int index_clip_begin = document().activeLayer()->children().length();
  paste_shift_ += QPointF(20, 20);
  QTransform shift =
       QTransform().translate(paste_shift_.x(), paste_shift_.y());

  for (int i = 0; i < document().clipboard().length(); i++) {
    ShapePtr shape = document().clipboard().at(i)->clone();
    shape->applyTransform(shift);
    document().activeLayer()->children().push_back(shape);
  }

  QList<ShapePtr> selected_shapes;

  for (int i = index_clip_begin;
       i < document().activeLayer()->children().length(); i++) {
    selected_shapes << document().activeLayer()->children().at(i);
  }

  document().setSelections(selected_shapes);
}

void VCanvas::editDelete() {
  document().stackStep();
  qInfo() << "Edit Delete";
  document().removeSelections();
}

void VCanvas::editUndo() { document().undo(); }

void VCanvas::editRedo() { document().redo(); }

void VCanvas::editDrawRect() {
  ctrl_rect_.reset();
  document().clearSelections();
  document().setMode(Document::Mode::RectDrawing);
}

void VCanvas::editDrawOval() {
  ctrl_oval_.reset();
  document().clearSelections();
  document().setMode(Document::Mode::OvalDrawing);
}

void VCanvas::editDrawLine() {
  ctrl_line_.reset();
  document().clearSelections();
  document().setMode(Document::Mode::LineDrawing);
}

void VCanvas::editDrawPath() {
  ctrl_path_draw_.reset();
  document().clearSelections();
  document().setMode(Document::Mode::PathDrawing);
}

void VCanvas::editDrawText() {
  ctrl_text_.reset();
  document().clearSelections();
  document().setMode(Document::Mode::TextDrawing);
}

void VCanvas::editSelectAll() {
  if (document().mode() != Document::Mode::Selecting)
    return;
  QList<ShapePtr> all_shapes;

  for (auto &layer : document().layers()) {
    all_shapes.append(layer->children());
  }

  document().setSelections(all_shapes);
}

void VCanvas::editGroup() {
  if (document().selections().empty())
    return;

  qInfo() << "Groupping";
  document().stackStep();
  ShapePtr group_ptr =
       make_shared<GroupShape>(ctrl_transform_.selections());
  document().removeSelections();
  document().activeLayer()->children().push_back(group_ptr);
  document().setSelection(group_ptr);
}

void VCanvas::editUngroup() {
  qInfo() << "Groupping";
  document().stackStep();
  ShapePtr group_ptr = document().selections().first();
  GroupShape *group = (GroupShape *) group_ptr.get();

  for (auto &shape : group->children()) {
    shape->applyTransform(group->transform());
    shape->setRotation(shape->rotation() + group->rotation());
    document().activeLayer()->children().push_back(shape);
  }

  document().setSelections(group->children());

  for (auto &layer : document().layers()) {
    layer->children().removeOne(group_ptr);
  }
}

Document &VCanvas::document() { return scene_; }

void VCanvas::editUnion() {
  if (document().selections().size() < 2)
    return;
  QPainterPath result;

  for (auto &shape : document().selections()) {
    if (shape->type() != Shape::Type::Path &&
        shape->type() != Shape::Type::Text)
      return;
    result = result.united(shape->transform().map(
         dynamic_cast<PathShape *>(shape.get())->path()));
  }

  document().stackStep();
  ShapePtr new_shape = make_shared<PathShape>(result);
  document().removeSelections();
  document().activeLayer()->addShape(new_shape);
  document().setSelection(new_shape);
}

void VCanvas::editSubtract() {
  if (document().selections().size() != 2)
    return;

  if (document().selections().at(0)->type() != Shape::Type::Path ||
      document().selections().at(1)->type() != Shape::Type::Path)
    return;

  document().stackStep();
  PathShape *a = dynamic_cast<PathShape *>(document().selections().at(0).get());
  PathShape *b = dynamic_cast<PathShape *>(document().selections().at(1).get());
  QPainterPath new_path(a->transform().map(a->path()).subtracted(
       b->transform().map(b->path())));
  ShapePtr new_shape = make_shared<PathShape>(new_path);
  document().removeSelections();
  document().activeLayer()->addShape(new_shape);
  document().setSelection(new_shape);
}

void VCanvas::editIntersect() {
  if (document().selections().size() != 2)
    return;

  if (document().selections().at(0)->type() != Shape::Type::Path ||
      document().selections().at(1)->type() != Shape::Type::Path)
    return;

  document().stackStep();
  PathShape *a = dynamic_cast<PathShape *>(document().selections().at(0).get());
  PathShape *b = dynamic_cast<PathShape *>(document().selections().at(1).get());
  QPainterPath new_path(a->transform().map(a->path()).intersected(
       b->transform().map(b->path())));
  new_path.closeSubpath();
  ShapePtr new_shape = make_shared<PathShape>(new_path);
  document().removeSelections();
  document().activeLayer()->addShape(new_shape);
  document().setSelection(new_shape);
}

void VCanvas::editDifference() {}

void VCanvas::fitWindow() {
  // Notes: we can even speed up by using half resolution:
  // setTextureSize(QSize(width()/2, height()/2));
  qreal proper_scale = min((width() - 100) / document().width(),
                           (height() - 100) / document().height());
  QPointF proper_translate =
       QPointF((width() - document().width() * proper_scale) / 2,
               (height() - document().height() * proper_scale) / 2);
  document().setScale(proper_scale);
  document().setScroll(proper_translate);
}

void VCanvas::importImage(QImage &image) {
  ShapePtr new_shape = make_shared<BitmapShape>(image);
  qreal scale = min(1.0, min(document().height() / image.height(),
                             document().width() / image.width()));
  qInfo() << "Scale" << scale;
  new_shape->setTransform(QTransform().scale(scale, scale));
  document().activeLayer()->addShape(new_shape);
  document().setSelection(new_shape);
}

void VCanvas::setActiveLayer(LayerPtr &layer) {
  document().setActiveLayer(layer);
}

void VCanvas::setLayerOrder(QList<LayerPtr> &new_order) {
  document().stackStep();
  document().reorderLayers(new_order);
}

void VCanvas::setFont(const QFont &font) {
  QFont new_font;
  if (document().selections().size() > 0 &&
      document().selections().at(0)->type() == Shape::Type::Text) {
    TextShape *t = dynamic_cast<TextShape *>(document().selections().at(0).get());
    new_font = t->font();
    new_font.setFamily(font.family());
    t->setFont(new_font);
    ShapePtr shape = document().selections().at(0);
    document().setSelection(shape);
  }
  if (document().mode() == Document::Mode::TextDrawing) {
    if (ctrl_text_.hasTarget()) {
      new_font = ctrl_text_.target().font();
      new_font.setFamily(font.family());
      ctrl_text_.target().setFont(new_font);
    } else {
      new_font = document().font();
      new_font.setFamily(font.family());
      ctrl_text_.target().setFont(new_font);
      document().setFont(new_font);
    }
  }
  document().setFont(new_font);
}

void VCanvas::exportGcode() {
  GCodeGenerator gen;
  ToolpathExporter exporter(&gen);
  exporter.convertStack(document().layers());
  std::cout << gen.toString();
}

void VCanvas::setScreenSize(QSize screen_size) {
  screen_size_ = screen_size;
  fitWindow();
}

void VCanvas::setScreenOffset(QPoint offset) {
  screen_offset_ = offset;
}