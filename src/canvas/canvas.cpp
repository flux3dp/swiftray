#include <canvas/canvas.h>
#include <QQuickWidget>
#include <QCursor>
#include <QDebug>
#include <QHoverEvent>
#include <QPainter>
#include <layer.h>
#include <shape/bitmap-shape.h>
#include <shape/group-shape.h>
#include <shape/path-shape.h>
#include <gcode/toolpath-exporter.h>
#include <widgets/preview-window.h>

// Initialize static members
QList<QColor> Layer::DefaultColors = QList<QColor>(
     {
          "#333333", "#3F51B5", "#F44336", "#FFC107", "#8BC34A",
          "#2196F3", "#009688", "#FF9800", "#CDDC39", "#00BCD4",
          "#FFEB3B", "#E91E63", "#673AB7", "#03A9F4", "#9C27B0",
          "#607D8B", "#9E9E9E"
     }
);
Document *Canvas::current_doc_ = new Document();
QRectF Canvas::screen_rect_ = QRectF();

Canvas::Canvas(QQuickItem *parent)
     : QQuickPaintedItem(parent), svgpp_parser_(SVGPPParser(document())),
       ctrl_transform_(Controls::Transform(document())),
       ctrl_select_(Controls::Select(document())),
       ctrl_grid_(Controls::Grid(document())), ctrl_line_(Controls::Line(document())),
       ctrl_oval_(Controls::Oval(document())),
       ctrl_path_draw_(Controls::PathDraw(document())),
       ctrl_path_edit_(Controls::PathEdit(document())),
       ctrl_rect_(Controls::Rect(document())), ctrl_text_(Controls::Text(document())),
       dash_counter_(0),
       fps(0) {
  setRenderTarget(RenderTarget::FramebufferObject);
  setAcceptedMouseButtons(Qt::AllButtons);
  setAcceptHoverEvents(true);
  setAcceptTouchEvents(true);
  setAntialiasing(true);
  setOpaquePainting(true);
  // Set main loop
  timer = new QTimer(this);
  connect(timer, &QTimer::timeout, this, &Canvas::loop);
  timer->start(16);
  // Set memory monitor
  mem_thread_ = new QThread(this);
  mem_monitor_.moveToThread(mem_thread_);
  mem_thread_->start();
  QTimer::singleShot(0, &mem_monitor_, &MemoryMonitor::doWork);
  // Set document & mode
  document().setMode(Document::Mode::Selecting);
  // Register controls
  ctrls_ << &ctrl_transform_ << &ctrl_select_ << &ctrl_rect_ << &ctrl_oval_
         << &ctrl_line_ << &ctrl_path_draw_ << &ctrl_path_edit_
         << &ctrl_text_;
  // FPS
  fps_count = 0;
  fps_timer.start();

  qInfo() << "[Canvas] Rendering target = " << this->renderTarget();
}

Canvas::~Canvas() {
  mem_thread_->requestInterruption();
  mem_thread_->wait(1300);
}

void Canvas::loadSVG(QByteArray &svg_data) {
  // TODO(Add undo events for loading svg)
  document().setRecordingUndo(false);
  bool success = svgpp_parser_.parse(svg_data);
  document().setRecordingUndo(true);
  setAntialiasing(true);

  if (success) {
    editSelectAll();
    forceActiveFocus();
    ready = true;
    update();
  }
}

void Canvas::paint(QPainter *painter) {
  painter->setRenderHint(QPainter::RenderHint::Antialiasing, fps > 30);
  painter->save();
  painter->fillRect(0, 0, width(), height(), QColor("#F0F0F0"));
  // Move to scroll and scale
  painter->translate(document().scroll());
  painter->scale(document().scale(), document().scale());

  ctrl_grid_.paint(painter);

  bool screen_changed = false;
  if (screen_rect_ != document().screenRect(widget_size_)) {
    screen_rect_ = document().screenRect(widget_size_);
    screen_changed = true;
  }

  int object_count = 0;
  dash_counter_++;

  for (const LayerPtr &layer : document().layers()) {
    if (screen_changed) layer->flushCache();
    object_count += layer->paint(painter, dash_counter_);
  }

  for (auto &control : ctrls_) {
    if (control->isActive()) {
      control->paint(painter);
    }
  }

  painter->restore();
  // Calculate FPS
  fps = (fps * 4 + float(++fps_count) * 1000 / fps_timer.elapsed()) / 5;
  painter->setPen(Qt::black);
  painter->drawText(QPointF(10, 20), "FPS: " + QString::number(round(fps * 100) / 100.0));
  painter->drawText(QPointF(10, 40),
                    "Objects " + QString::number(object_count) + " Frames: #" + QString::number(dash_counter_));
  painter->drawText(QPointF(10, 60), "Mem: " + mem_monitor_.system_info_);
  if (fps_timer.elapsed() > 3000) {
    fps_count = 0;
    fps_timer.restart();
  }
}

void Canvas::keyPressEvent(QKeyEvent *e) {
  // qInfo() << "Key press" << e;

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

void Canvas::mousePressEvent(QMouseEvent *e) {
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
      document().setSelection(nullptr);
      document().setMode(Document::Mode::MultiSelecting);
    }
  }
}

void Canvas::mouseMoveEvent(QMouseEvent *e) {
  for (auto &control : ctrls_) {
    if (control->isActive() && control->mouseMoveEvent(e))
      return;
  }
}

void Canvas::mouseReleaseEvent(QMouseEvent *e) {
  for (auto &control : ctrls_) {
    if (control->isActive() && control->mouseReleaseEvent(e))
      return;
  }

  document().setMode(Document::Mode::Selecting);
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  qInfo() << "Mouse Double Click (screen)" << e->pos() << " -> (canvas)"
          << canvas_coord;
  ShapePtr hit = document().hitTest(canvas_coord);
  if (document().mode() == Document::Mode::Selecting) {
    if (hit != nullptr) {
      qInfo() << "Double clicked" << hit.get();
      switch (hit->type()) {
        case Shape::Type::Path:
          document().setSelection(nullptr);
          ctrl_path_edit_.setTarget(hit);
          document().setMode(Document::Mode::PathEditing);
          break;
        case Shape::Type::Text:
          document().setSelection(nullptr);
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

void Canvas::wheelEvent(QWheelEvent *e) {
  document().setScroll(document().scroll() + e->pixelDelta() / 2.5);
}

void Canvas::loop() {
  update();
}

bool Canvas::event(QEvent *e) {
  // qInfo() << "QEvent" << e;
  QNativeGestureEvent *nge;
  Qt::CursorShape cursor;

  switch (e->type()) {
    case QEvent::HoverMove:
      unsetCursor();
      for (auto &control : ctrls_) {
        if (control->isActive() &&
            control->hoverEvent(dynamic_cast<QHoverEvent *>(e), &cursor)) {
          setCursor(cursor);
          break;
        }
      }

      break;

    case QEvent::NativeGesture:
      nge = dynamic_cast<QNativeGestureEvent *>(e);

      //  (passed by main window)
      if (nge->gestureType() == Qt::ZoomNativeGesture) {
        QPoint mouse_pos = nge->localPos().toPoint() - widget_offset_;
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

void Canvas::editCut() {
  if (document().mode() != Document::Mode::Selecting)
    return;
  clipboard().cutFrom(document());
}

void Canvas::editCopy() {
  if (document().mode() != Document::Mode::Selecting)
    return;
  clipboard().set(document().selections());
}

void Canvas::editPaste() {
  if (document().mode() != Document::Mode::Selecting)
    return;
  clipboard().pasteTo(document());
}

void Canvas::editDelete() {
  qInfo() << "Edit Delete";
  if (document().mode() != Document::Mode::Selecting)
    return;

  // TODO (Check all selection events to accompany with document.removeSelections and setSelections)
  document().execute(
       JoinedCmd::removeSelections()
  );
}

void Canvas::editUndo() { document().undo(); }

void Canvas::editRedo() { document().redo(); }

void Canvas::editDrawRect() {
  document().setSelection(nullptr);
  document().setMode(Document::Mode::RectDrawing);
}

void Canvas::editDrawOval() {
  document().setSelection(nullptr);
  document().setMode(Document::Mode::OvalDrawing);
}

void Canvas::editDrawLine() {
  document().setSelection(nullptr);
  document().setMode(Document::Mode::LineDrawing);
}

void Canvas::editDrawPath() {
  document().setSelection(nullptr);
  document().setMode(Document::Mode::PathDrawing);
}

void Canvas::editDrawText() {
  document().setSelection(nullptr);
  document().setMode(Document::Mode::TextDrawing);
}

void Canvas::editSelectAll() {
  if (document().mode() != Document::Mode::Selecting)
    return;
  QList<ShapePtr> all_shapes;

  for (auto &layer : document().layers()) {
    all_shapes.append(layer->children());
  }

  document().setSelections(all_shapes);
}

void Canvas::editGroup() {
  qInfo() << "Edit Group";
  document().groupSelections();
}

void Canvas::editUngroup() {
  qInfo() << "Edit Ungroup";
  document().ungroupSelections();
}

Document &Canvas::document() { return *Canvas::current_doc_; }

const QRectF &Canvas::screenRect() { return screen_rect_; };

void Canvas::editUnion() {
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

  ShapePtr new_shape = make_shared<PathShape>(result);
  document().execute(
       Commands::AddShape::shared(document().activeLayer(), new_shape) +
       Commands::JoinedCmd::removeSelections() +
       Commands::Select::shared({new_shape})
  );
}

void Canvas::editSubtract() {
  if (document().selections().size() != 2)
    return;

  if (document().selections().at(0)->type() != Shape::Type::Path ||
      document().selections().at(1)->type() != Shape::Type::Path)
    return;

  PathShape *a = dynamic_cast<PathShape *>(document().selections().at(0).get());
  PathShape *b = dynamic_cast<PathShape *>(document().selections().at(1).get());
  QPainterPath new_path(a->transform().map(a->path()).subtracted(
       b->transform().map(b->path())));
  ShapePtr new_shape = make_shared<PathShape>(new_path);
  document().execute(
       Commands::AddShape::shared(document().activeLayer(), new_shape) +
       Commands::JoinedCmd::removeSelections() +
       Commands::Select::shared({new_shape})
  );
}

void Canvas::editIntersect() {
  if (document().selections().size() != 2)
    return;

  if (document().selections().at(0)->type() != Shape::Type::Path ||
      document().selections().at(1)->type() != Shape::Type::Path)
    return;

  PathShape *a = dynamic_cast<PathShape *>(document().selections().at(0).get());
  PathShape *b = dynamic_cast<PathShape *>(document().selections().at(1).get());
  QPainterPath new_path(a->transform().map(a->path()).intersected(
       b->transform().map(b->path())));
  new_path.closeSubpath();
  ShapePtr new_shape = make_shared<PathShape>(new_path);
  document().execute(
       Commands::AddShape::shared(document().activeLayer(), new_shape) +
       Commands::JoinedCmd::removeSelections() +
       Commands::Select::shared({new_shape})
  );
}

void Canvas::editDifference() {}

void Canvas::addEmptyLayer() {
  document().addLayer();
  document().execute(
       new Commands::AddLayer(document().activeLayer())
  );
}

void Canvas::fitToWindow() {
  // Notes: we can even speed up by using half resolution:
  //setTextureSize(QSize(width() / 2, height() / 2));
  qreal proper_scale = min((width() - 100) / document().width(),
                           (height() - 100) / document().height());
  QPointF proper_translate =
       QPointF((width() - document().width() * proper_scale) / 2,
               (height() - document().height() * proper_scale) / 2);
  document().setScale(proper_scale);
  document().setScroll(proper_translate);
}

void Canvas::importImage(QImage &image) {
  ShapePtr new_shape = make_shared<BitmapShape>(image);
  qreal scale = min(1.0, min(document().height() / image.height(),
                             document().width() / image.width()));
  qInfo() << "Scale" << scale;
  new_shape->setTransform(QTransform().scale(scale, scale));
  document().activeLayer()->addShape(new_shape);
  document().setSelection(new_shape);
}

void Canvas::setActiveLayer(LayerPtr &layer) {
  document().setActiveLayer(layer);
}

void Canvas::setLayerOrder(QList<LayerPtr> &new_order) {
  // TODO (Add undo for set layer order);
  document().reorderLayers(new_order);
}

void Canvas::setFont(const QFont &font) {
  // TODO (Add undo for set font event)
  QFont new_font;
  if (!document().selections().isEmpty() &&
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

shared_ptr<PreviewGenerator> Canvas::exportGcode() {
  auto gen = make_shared<PreviewGenerator>();
  ToolpathExporter exporter(gen.get());
  exporter.convertStack(document().layers());
  return gen;
}

void Canvas::setWidgetSize(QSize widget_size) {
  widget_size_ = widget_size;
  fitToWindow();
}

void Canvas::setWidgetOffset(QPoint offset) {
  widget_offset_ = offset;
}

Clipboard &Canvas::clipboard() {
  return clipboard_;
}

void Canvas::backToSelectMode() {
  // TODO (Add exit function to all controls)

}