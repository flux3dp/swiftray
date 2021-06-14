#include <canvas/vcanvas.h>
#include <QCursor>
#include <QDebug>
#include <QHoverEvent>
#include <QPainter>
#include <QWidget>
#include <cstring>
#include <iostream>
#include <canvas/layer.h>
#include <shape/bitmap_shape.h>
#include <shape/group_shape.h>
#include <shape/path_shape.h>
#include <boost/range/adaptor/reversed.hpp>
#include <gcode/toolpath_exporter.h>
#include <gcode/generators/gcode_generator.h>

VCanvas::VCanvas(QQuickItem *parent)
     : QQuickPaintedItem(parent), svgpp_parser_(SVGPPParser(scene())),
       ctrl_transform_(Controls::Transform(scene())),
       ctrl_select_(Controls::Select(scene())),
       ctrl_grid_(Controls::Grid(scene())), ctrl_line_(Controls::Line(scene())),
       ctrl_oval_(Controls::Oval(scene())),
       ctrl_path_draw_(Controls::PathDraw(scene())),
       ctrl_path_edit_(Controls::PathEdit(scene())),
       ctrl_rect_(Controls::Rect(scene())), ctrl_text_(Controls::Text(scene())),
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
  scene().setMode(Scene::Mode::Selecting);
  // Register controls
  ctrls_ << &ctrl_transform_ << &ctrl_select_ << &ctrl_rect_ << &ctrl_oval_
         << &ctrl_line_ << &ctrl_path_draw_ << &ctrl_path_edit_
         << &ctrl_text_;
  // FPS
  fps_count = 0;
  fps_timer.start();


  qInfo() << "Rendering target = " << this->renderTarget();

  connect(&ctrl_transform_, &Controls::Transform::transformChanged, [=]() {
    for (auto &shape : scene().selections()) {
      shape->parent()->flushCache();
    }
  });
}

VCanvas::~VCanvas() {
  mem_thread_->requestInterruption();
  mem_thread_->wait(1300);
}

void VCanvas::loadSVG(QByteArray &svg_data) {
  // scene().clearAll();
  // scene().addLayer();
  bool success = svgpp_parser_.parse(svg_data);
  setAntialiasing(true);

  if (success) {
    editSelectAll();
    scene().stackStep();
    forceActiveFocus();
    ready = true;
    update();
  }
}

void VCanvas::paint(QPainter *painter) {
  painter->save();
  painter->fillRect(0, 0, width(), height(), QColor("#F0F0F0"));
  // Calculate FPS
  fps_count++;
  fps = (fps * 4 + float(fps_count) * 1000 / fps_timer.elapsed()) / 5;
  if (fps < 30) {
    painter->setRenderHint(QPainter::RenderHint::Antialiasing, false);
  } else {
    painter->setRenderHint(QPainter::RenderHint::Antialiasing, true);
  }
  // Move to scroll and scale
  painter->translate(scene().scroll());
  painter->scale(scene().scale(), scene().scale());

  ctrl_grid_.paint(painter);

  bool do_flush_cache = false;
  if (screen_rect_ != scene().screenRect(screen_size_)) {
    screen_rect_ = scene().screenRect(screen_size_);
    do_flush_cache = true;
    qInfo() << "Flush cache" << screen_rect_;
  }
  for (const LayerPtr &layer : scene().layers()) {
    if (do_flush_cache) layer->flushCache();
    layer->paint(painter, screen_rect_, counter);
  }

  for (auto &control : ctrls_) {
    if (control->isActive()) {
      control->paint(painter);
    }
  }

  // Draw FPS
  painter->restore();
  painter->setPen(Qt::black);
  painter->drawText(QPointF(10, 20), "FPS " + QString::number(round(fps * 100) / 100.0));
  painter->drawText(QPointF(10, 40), "Objects " + QString::number(scene().activeLayer()->children().size()));
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
  QPointF canvas_coord = scene().getCanvasCoord(e->pos());
  scene().setMousePressedScreenCoord(e->pos());
  qInfo() << "Mouse Press (screen)" << e->pos() << " -> (canvas)"
          << canvas_coord;

  for (auto &control : ctrls_) {
    if (control->isActive() && control->mousePressEvent(e))
      return;
  }

  if (scene().mode() == Scene::Mode::Selecting) {
    ShapePtr hit = scene().hitTest(canvas_coord);

    if (hit != nullptr) {
      if (!hit->selected()) {
        scene().setSelection(hit);
      }
    } else {
      scene().clearSelections();
      scene().setMode(Scene::Mode::MultiSelecting);
    }
  }
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
  QPointF canvas_coord = scene().getCanvasCoord(e->pos());
  // qInfo() << "Mouse Move (screen)" << e->pos() << " -> (canvas)" <<
  // canvas_coord;

  for (auto &control : ctrls_) {
    if (control->isActive() && control->mouseMoveEvent(e))
      return;
  }
}

void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
  QPointF canvas_coord = scene().getCanvasCoord(e->pos());
  qInfo() << "Mouse Release (screen)" << e->pos() << " -> (canvas)"
          << canvas_coord;

  for (auto &control : ctrls_) {
    if (control->isActive() && control->mouseReleaseEvent(e))
      return;
  }

  scene().setMode(Scene::Mode::Selecting);
}

void VCanvas::mouseDoubleClickEvent(QMouseEvent *e) {
  QPointF canvas_coord = scene().getCanvasCoord(e->pos());
  qInfo() << "Mouse Double Click (screen)" << e->pos() << " -> (canvas)"
          << canvas_coord;
  qInfo() << "Mode" << (int) scene().mode();
  ShapePtr hit = scene().hitTest(canvas_coord);
  if (scene().mode() == Scene::Mode::Selecting) {
    if (hit != nullptr) {
      qInfo() << "Double clicked" << hit.get();
      switch (hit->type()) {
        case Shape::Type::Path:
          scene().clearSelections();
          ctrl_path_edit_.setTarget(hit);
          scene().setMode(Scene::Mode::PathEditing);
          break;
        case Shape::Type::Text:
          scene().clearSelections();
          ctrl_text_.setTarget(hit);
          scene().setMode(Scene::Mode::TextDrawing);
          break;
        default:
          break;
      }
    }
  } else if (scene().mode() == Scene::Mode::PathEditing) {
    ctrl_path_edit_.endEditing();
  }
}

void VCanvas::wheelEvent(QWheelEvent *e) {
  scene().setScroll(scene().scroll() + e->pixelDelta() / 2.5);
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

      if (nge->gestureType() == Qt::ZoomNativeGesture) {
        scene().setScale(max(0.01, scene().scale() + nge->value() / 2));
      }

      break;

    default:
      break;
  }

  return QQuickPaintedItem::event(e);
}

void VCanvas::editCut() {
  if (scene().mode() != Scene::Mode::Selecting)
    return;
  scene().stackStep();
  qInfo() << "Edit Cut";
  editCopy();
  scene().removeSelections();
}

void VCanvas::editCopy() {
  if (scene().mode() != Scene::Mode::Selecting)
    return;
  qInfo() << "Edit Copy";
  scene().clearClipboard();
  scene().setClipboard(scene().selections());
  paste_shift_ = QPointF(0, 0);
}

void VCanvas::editPaste() {
  if (scene().mode() != Scene::Mode::Selecting)
    return;
  scene().stackStep();
  qInfo() << "Edit Paste";
  int index_clip_begin = scene().activeLayer()->children().length();
  paste_shift_ += QPointF(20, 20);
  QTransform shift =
       QTransform().translate(paste_shift_.x(), paste_shift_.y());

  for (int i = 0; i < scene().clipboard().length(); i++) {
    ShapePtr shape = scene().clipboard().at(i)->clone();
    shape->applyTransform(shift);
    scene().activeLayer()->children().push_back(shape);
  }

  QList<ShapePtr> selected_shapes;

  for (int i = index_clip_begin;
       i < scene().activeLayer()->children().length(); i++) {
    selected_shapes << scene().activeLayer()->children().at(i);
  }

  scene().setSelections(selected_shapes);
}

void VCanvas::editDelete() {
  scene().stackStep();
  qInfo() << "Edit Delete";
  scene().removeSelections();
}

void VCanvas::editUndo() { scene().undo(); }

void VCanvas::editRedo() { scene().redo(); }

void VCanvas::editDrawRect() {
  ctrl_rect_.reset();
  scene().clearSelections();
  scene().setMode(Scene::Mode::RectDrawing);
}

void VCanvas::editDrawOval() {
  ctrl_oval_.reset();
  scene().clearSelections();
  scene().setMode(Scene::Mode::OvalDrawing);
}

void VCanvas::editDrawLine() {
  ctrl_line_.reset();
  scene().clearSelections();
  scene().setMode(Scene::Mode::LineDrawing);
}

void VCanvas::editDrawPath() {
  ctrl_path_draw_.reset();
  scene().clearSelections();
  scene().setMode(Scene::Mode::PathDrawing);
}

void VCanvas::editDrawText() {
  ctrl_text_.reset();
  scene().clearSelections();
  scene().setMode(Scene::Mode::TextDrawing);
}

void VCanvas::editSelectAll() {
  if (scene().mode() != Scene::Mode::Selecting)
    return;
  QList<ShapePtr> all_shapes;

  for (auto &layer : scene().layers()) {
    all_shapes.append(layer->children());
  }

  scene().setSelections(all_shapes);
}

void VCanvas::editGroup() {
  if (scene().selections().empty())
    return;

  qInfo() << "Groupping";
  scene().stackStep();
  ShapePtr group_ptr =
       make_shared<GroupShape>(ctrl_transform_.selections());
  scene().removeSelections();
  scene().activeLayer()->children().push_back(group_ptr);
  scene().setSelection(group_ptr);
}

void VCanvas::editUngroup() {
  qInfo() << "Groupping";
  scene().stackStep();
  ShapePtr group_ptr = scene().selections().first();
  GroupShape *group = (GroupShape *) group_ptr.get();

  for (auto &shape : group->children()) {
    shape->applyTransform(group->transform());
    shape->setRotation(shape->rotation() + group->rotation());
    scene().activeLayer()->children().push_back(shape);
  }

  scene().setSelections(group->children());

  for (auto &layer : scene().layers()) {
    layer->children().removeOne(group_ptr);
  }
}

Scene &VCanvas::scene() { return scene_; }

void VCanvas::editUnion() {
  if (scene().selections().size() < 2)
    return;
  QPainterPath result;

  for (auto &shape : scene().selections()) {
    if (shape->type() != Shape::Type::Path &&
        shape->type() != Shape::Type::Text)
      return;
    result = result.united(shape->transform().map(
         dynamic_cast<PathShape *>(shape.get())->path()));
  }

  scene().stackStep();
  ShapePtr new_shape = make_shared<PathShape>(result);
  scene().removeSelections();
  scene().activeLayer()->addShape(new_shape);
  scene().setSelection(new_shape);
}

void VCanvas::editSubtract() {
  if (scene().selections().size() != 2)
    return;

  if (scene().selections().at(0)->type() != Shape::Type::Path ||
      scene().selections().at(1)->type() != Shape::Type::Path)
    return;

  scene().stackStep();
  PathShape *a = dynamic_cast<PathShape *>(scene().selections().at(0).get());
  PathShape *b = dynamic_cast<PathShape *>(scene().selections().at(1).get());
  QPainterPath new_path(a->transform().map(a->path()).subtracted(
       b->transform().map(b->path())));
  ShapePtr new_shape = make_shared<PathShape>(new_path);
  scene().removeSelections();
  scene().activeLayer()->addShape(new_shape);
  scene().setSelection(new_shape);
}

void VCanvas::editIntersect() {
  if (scene().selections().size() != 2)
    return;

  if (scene().selections().at(0)->type() != Shape::Type::Path ||
      scene().selections().at(1)->type() != Shape::Type::Path)
    return;

  scene().stackStep();
  PathShape *a = dynamic_cast<PathShape *>(scene().selections().at(0).get());
  PathShape *b = dynamic_cast<PathShape *>(scene().selections().at(1).get());
  QPainterPath new_path(a->transform().map(a->path()).intersected(
       b->transform().map(b->path())));
  new_path.closeSubpath();
  ShapePtr new_shape = make_shared<PathShape>(new_path);
  scene().removeSelections();
  scene().activeLayer()->addShape(new_shape);
  scene().setSelection(new_shape);
}

void VCanvas::editDifference() {}

void VCanvas::fitWindow() {
  qInfo() << "Object size" << size();
  // Notes: we can even speed up by using half resolution:
  // setTextureSize(QSize(width()/2, height()/2));
  qreal proper_scale = min((width() - 100) / scene().width(),
                           (height() - 100) / scene().height());
  QPointF proper_translate =
       QPointF((width() - scene().width() * proper_scale) / 2,
               (height() - scene().height() * proper_scale) / 2);
  qInfo() << "Scale" << proper_scale << "Proper translate"
          << proper_translate;
  scene().setScale(proper_scale);
  scene().setScroll(proper_translate);
}

void VCanvas::importImage(QImage &image) {
  ShapePtr new_shape = make_shared<BitmapShape>(image);
  qreal scale = min(1.0, min(scene().height() / image.height(),
                             scene().width() / image.width()));
  qInfo() << "Scale" << scale;
  new_shape->setTransform(QTransform().scale(scale, scale));
  scene().activeLayer()->addShape(new_shape);
  scene().setSelection(new_shape);
}

void VCanvas::setActiveLayer(LayerPtr &layer) {
  scene().setActiveLayer(layer);
}

void VCanvas::setLayerOrder(QList<LayerPtr> &new_order) {
  scene().stackStep();
  scene().reorderLayers(new_order);
}

void VCanvas::setFont(const QFont &font) {
  QFont new_font;
  if (scene().selections().size() > 0 &&
      scene().selections().at(0)->type() == Shape::Type::Text) {
    TextShape *t = dynamic_cast<TextShape *>(scene().selections().at(0).get());
    new_font = t->font();
    new_font.setFamily(font.family());
    t->setFont(new_font);
    ShapePtr shape = scene().selections().at(0);
    scene().setSelection(shape);
  }
  if (scene().mode() == Scene::Mode::TextDrawing) {
    if (ctrl_text_.hasTarget()) {
      new_font = ctrl_text_.target().font();
      new_font.setFamily(font.family());
      ctrl_text_.target().setFont(new_font);
    } else {
      new_font = scene().font();
      new_font.setFamily(font.family());
      ctrl_text_.target().setFont(new_font);
      scene().setFont(new_font);
    }
  }
  scene().setFont(new_font);
}

void VCanvas::exportGcode() {
  GCodeGenerator gen;
  ToolpathExporter exporter(&gen);
  exporter.convertStack(scene().layers());
  std::cout << gen.toString();
}

void VCanvas::setScreenSize(QSize screen_size) {
  qInfo() << "Resize canvas" << screen_size;
  screen_size_ = screen_size;
  fitWindow();
}