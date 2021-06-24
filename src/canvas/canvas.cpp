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

Canvas::Canvas(QQuickItem *parent)
     : QQuickPaintedItem(parent),
       ctrl_transform_(Controls::Transform(this)),
       ctrl_select_(Controls::Select(this)),
       ctrl_grid_(Controls::Grid(this)),
       ctrl_line_(Controls::Line(this)),
       ctrl_oval_(Controls::Oval(this)),
       ctrl_path_draw_(Controls::PathDraw(this)),
       ctrl_path_edit_(Controls::PathEdit(this)),
       ctrl_rect_(Controls::Rect(this)),
       ctrl_text_(Controls::Text(this)),
       svgpp_parser_(SVGPPParser()),
       fps(0),
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
  timer->start(16);
  volatility_timer.start();

  // Register controls
  ctrls_ << &ctrl_transform_ << &ctrl_select_ << &ctrl_rect_ << &ctrl_oval_
         << &ctrl_line_ << &ctrl_path_draw_ << &ctrl_path_edit_
         << &ctrl_text_;

  // FPS
  fps_count = 0;
  fps_timer.start();

  // Register events
  connect(this, &Canvas::selectionsChanged, [=]() {
    for (auto &layer : document().layers()) {
      layer->flushCache();
    }
  });
}

Canvas::~Canvas() {
  mem_thread_->requestInterruption();
  mem_thread_->wait(1300);
}

void Canvas::loadSVG(QByteArray &svg_data) {
  // TODO(Add undo events for loading svg)
  QElapsedTimer t;
  t.start();
  bool success = svgpp_parser_.parse(&document(), svg_data);
  setAntialiasing(true);

  if (success) {
    editSelectAll();
    forceActiveFocus();
    emitAllChanges();
    update();
  }
  qInfo() << "[Parser] Took" << t.elapsed();
}

void Canvas::paint(QPainter *painter) {
  painter->setRenderHint(QPainter::RenderHint::Antialiasing, fps > 30);
  painter->save();
  painter->fillRect(0, 0, width(), height(), QColor("#F0F0F0"));
  // Move to scroll and scale
  painter->translate(document().scroll());
  painter->scale(document().scale(), document().scale());

  ctrl_grid_.paint(painter);

  document().paint(painter);

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
  painter->drawText(QPointF(10, 40), "Frames: #" + QString::number(document().framesCount()));
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

  if (mode() == Mode::Selecting) {
    ShapePtr hit = document().hitTest(canvas_coord);

    if (hit != nullptr) {
      if (!hit->selected()) {
        document().setSelection(hit);
      }
    } else {
      document().setSelection(nullptr);
      setMode(Mode::MultiSelecting);
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

  setMode(Mode::Selecting);
}

void Canvas::mouseDoubleClickEvent(QMouseEvent *e) {
  QPointF canvas_coord = document().getCanvasCoord(e->pos());
  ShapePtr hit = document().hitTest(canvas_coord);
  if (mode() == Mode::Selecting) {
    if (hit != nullptr) {
      switch (hit->type()) {
        case Shape::Type::Text:
          qInfo() << "[Canvas] Double clicked text" << hit.get();
          document().setSelection(hit);
          ctrl_text_.setTarget(hit);
          setMode(Mode::TextDrawing);
          break;
        case Shape::Type::Path:
          qInfo() << "[Canvas] Double clicked path" << hit.get();
          document().setSelection(nullptr);
          ctrl_path_edit_.setTarget(hit);
          setMode(Mode::PathEditing);
          break;
        default:
          break;
      }
    }
  } else if (mode() == Mode::PathEditing) {
    ctrl_path_edit_.exit();
  }
}

void Canvas::wheelEvent(QWheelEvent *e) {
  document().setScroll(document().scroll() + e->pixelDelta() / 2.5);
  volatility_timer.restart();
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
  if (mode() != Mode::Selecting)
    return;
  clipboard().pasteTo(document());
}

void Canvas::editDelete() {
  qInfo() << "Edit Delete";
  if (mode() != Mode::Selecting)
    return;

  // TODO (Check all selection events to accompany with document.removeSelections and setSelections)
  document().execute(
       Commands::RemoveSelections(&document())
  );
}

void Canvas::editUndo() {
  QElapsedTimer t;
  t.start();
  document().undo();
  emit layerChanged(); // TODO (Check if layers are really changed)
  emit selectionsChanged(); // Force refresh all selection related components
  emit undoCalled();
  qInfo() << "[Undo] Took" << t.elapsed() << "ms";
}

void Canvas::editRedo() {
  document().redo();
  emit layerChanged(); // TODO (Check if layers are really changed)
  emit selectionsChanged(); // Force refresh all selection related components
  emit redoCalled();
}

void Canvas::editDrawRect() {
  document().setSelection(nullptr);
  setMode(Mode::RectDrawing);
}

void Canvas::editDrawOval() {
  document().setSelection(nullptr);
  setMode(Mode::OvalDrawing);
}

void Canvas::editDrawLine() {
  document().setSelection(nullptr);
  setMode(Mode::LineDrawing);
}

void Canvas::editDrawPath() {
  document().setSelection(nullptr);
  setMode(Mode::PathDrawing);
}

void Canvas::editDrawText() {
  document().setSelection(nullptr);
  setMode(Mode::TextDrawing);
}

void Canvas::editSelectAll() {
  if (mode() != Mode::Selecting)
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

  ShapePtr new_shape = make_shared<PathShape>(result);
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
  auto new_shape = make_shared<PathShape>(new_path);
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
  ShapePtr new_shape = make_shared<PathShape>(new_path);
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

  QPainterPath intersection(a->transform().map(a->path()).intersected(
       b->transform().map(b->path())));
  intersection.closeSubpath();
  ShapePtr new_shape_a = make_shared<PathShape>(a->transform().map(a->path()).subtracted(intersection));
  ShapePtr new_shape_b = make_shared<PathShape>(b->transform().map(b->path()).subtracted(intersection));
  document().execute(
       Commands::AddShape(document().activeLayer(), new_shape_a),
       Commands::AddShape(document().activeLayer(), new_shape_b),
       Commands::RemoveSelections(&document()),
       Commands::Select(&document(), {new_shape_a, new_shape_b})
  );

}

void Canvas::addEmptyLayer() {
  int i = 1;
  while (document().findLayerByName("Layer " + QString::number(i)) != nullptr) i++;
  LayerPtr new_layer = make_shared<Layer>(&document(), i);
  document().execute(
       Commands::AddLayer(new_layer)
  );
  emit layerChanged();
}

void Canvas::fitToWindow() {
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
  emit layerChanged();
}

void Canvas::setLayerOrder(QList<LayerPtr> &new_order) {
  document().execute(
       Commands::SetRef<Document, QList<LayerPtr>, &Document::layers, &Document::setLayersOrder>(
            doc_.get(), new_order
       )
  );
}

void Canvas::setFont(const QFont &font) {
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        cmd << Commands::SetFont((TextShape *) shape.get(), font);
      }
    }
    document().execute(cmd);
    emit selectionsChanged();
  } else if (mode() == Mode::TextDrawing) {
    ctrl_text_.target().setFont(font);
  }

  font_ = font;
}

void Canvas::setLineHeight(float line_height) {
  if (!document().selections().isEmpty()) {
    auto cmd = Commands::Joined();
    for (auto &shape: document().selections()) {
      if (shape->type() == Shape::Type::Text) {
        cmd << Commands::SetLineHeight((TextShape *) shape.get(), line_height);
      }
    }
    document().execute(cmd);
    emit selectionsChanged();
  } else if (mode() == Mode::TextDrawing) {
    ctrl_text_.target().setLineHeight(line_height);
  }
}

shared_ptr<PreviewGenerator> Canvas::exportGcode() {
  auto gen = make_shared<PreviewGenerator>();
  ToolpathExporter exporter(gen.get());
  exporter.convertStack(document().layers());
  return gen;
}

void Canvas::setWidgetSize(QSize widget_size) {
  document().setScreenSize(widget_size);
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
  switch (mode()) {
    case Mode::TextDrawing:
      ctrl_text_.exit();
      break;
  }
}

void Canvas::startMemoryMonitor() {
  mem_monitor_.moveToThread(mem_thread_);
  mem_thread_->start();
  QTimer::singleShot(0, &mem_monitor_, &MemoryMonitor::doWork);
}

Document &Canvas::document() { return *doc_.get(); }

void Canvas::setDocument(Document *document) {
  doc_ = unique_ptr<Document>(document);
  doc_->setCanvas(this);
  connect(doc_.get(), &Document::selectionsChanged, this, &Canvas::selectionsChanged);
}

Canvas::Mode Canvas::mode() const { return mode_; }

void Canvas::setMode(Mode mode) {
  mode_ = mode;
  emit modeChanged();
}

void Canvas::emitAllChanges() {
  emit layerChanged();
  emit modeChanged();
}

bool Canvas::isVolatile() const {
  if (volatility_timer.elapsed() < 1000) { return true; }
  return mode_ == Mode::Moving || mode_ == Mode::Rotating || mode_ == Mode::Transforming;
}

const QFont &Canvas::font() const { return font_; }
