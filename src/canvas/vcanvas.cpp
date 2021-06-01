#include <canvas/vcanvas.h>
#include <QDebug>
#include <QPainter>
#include <QWidget>
#include <QCursor>
#include <QHoverEvent>
#include <cstring>
#include <iostream>
#include <shape/group_shape.h>
#include <canvas/layer.h>

VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent),
    svgpp_parser { SVGPPParser(scene_) }, transform_box { TransformBox(scene_) } {
    setRenderTarget(RenderTarget::FramebufferObject);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setAntialiasing(true);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VCanvas::loop);
    timer->start(16);
    scene_.scroll_x = 0;
    scene_.scroll_y = 0;
    scene_.scale = 1;
    scene_.setMode(Scene::Mode::SELECTING);
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::loadSVG(QByteArray &svg_data) {
    //scene_.clearAll();
    //scene_.addLayer();
    bool success = svgpp_parser.parse(svg_data);
    setAntialiasing(false);

    if (success) {
        scene_.stackStep();
        editSelectAll();
        forceActiveFocus();
        ready = true;
        update();
    }
}

void VCanvas::paint(QPainter *painter) {
    painter->translate(scene_.scroll());
    painter->scale(scene_.scale, scene_.scale);
    //qInfo() << "Rendering engine = " << painter->paintEngine()->type();
    QColor sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    QColor sky_blue_alpha = QColor::fromRgb(0x00, 0x99, 0xCC, 30);
    QPen multi_select_pen = QPen(sky_blue, 0, Qt::DashLine);
    QBrush multi_select_brush = QBrush(sky_blue_alpha);

    // Draw layers
    for (const Layer &layer : scene_.layers()) {
        layer.paint(painter, counter);
    }

    // Draw transform box
    transform_box.paint(painter);

    if (scene_.mode() == Scene::Mode::MULTI_SELECTING) {
        painter->setBrush(multi_select_brush);
        painter->setPen(multi_select_pen);
        painter->fillRect(selection_box, multi_select_brush);
        painter->drawRect(selection_box);
    }

    //qInfo() << "Offset" << scrollX << scrollY << "Scale" << scale;
}

void VCanvas::keyPressEvent(QKeyEvent *e) {
    qInfo() << "Key press" << e;

    if (e->key() == Qt::Key::Key_Delete || e->key() == Qt::Key::Key_Backspace || e->key() == Qt::Key::Key_Back) {
        editDelete();
    }
}

void VCanvas::mousePressEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene_.getCanvasCoord(e->pos());
    qInfo() << "Mouse Press" << e->pos();
    mouse_press = e->pos();
    qInfo() << "Click point" << canvas_coord;

    // Test transform_box control point
    if (transform_box.mousePressEvent(e)) return;

    if (rect_drawer.mouseReleaseEvent(e)) return;

    // Test object selection in reverse
    bool selectedObject = false;

    for (Layer &layer : scene_.layers()) {
        for (ShapePtr &shape : layer.children()) {
            if (shape->testHit(canvas_coord, 10 / scene_.scale)) {
                selectedObject = true;

                // Do not reset selection if it's already selected
                if (scene_.isSelected(shape)) {
                    break;
                }

                // Reset selection
                scene_.setSelection(shape);
                break;
            }
        }

        if (selectedObject) {
            break;
        }
    }

    if (selectedObject) {
        mouse_drag = true;
        scene_.setMode(Scene::Mode::SELECTING);
        return;
    }

    scene_.clearSelection();
    // Multi Select
    mouse_drag = true;
    scene_.setMode(Scene::Mode::MULTI_SELECTING);
    selection_box = QRectF(0, 0, 0, 0);
    selection_start = canvas_coord;
    return;
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene_.getCanvasCoord(e->pos());

    if (transform_box.mouseMoveEvent(e)) return;

    if (rect_drawer.mouseReleaseEvent(e)) return;

    if (scene_.mode() == Scene::Mode::MULTI_SELECTING) {
        //QRectF a()
        //QSizeF rsize(abs(e->pos().x() - mouse_press.x()), abs(e->pos().y() - mouse_press.y()));
        selection_box = QRectF(selection_start, canvas_coord);
    }

    qInfo() << "Mouse Move" << e->pos();
}
void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (transform_box.mouseReleaseEvent(e)) return;

    if (rect_drawer.mouseReleaseEvent(e)) return;

    if (scene_.mode() == Scene::Mode::MULTI_SELECTING &&
        (selection_box.width() != 0 || selection_box.height() != 0)) {
        QList<ShapePtr> selected;
        qInfo() << "Selection Box" << selection_box;

        for (Layer &layer : scene_.layers()) {
            for (ShapePtr &shape : layer.children()) {
                if (shape->testHit(selection_box)) {
                    selected << shape;
                }
            }
        }

        qInfo() << "Mouse Release Selected" << selected.size();
        scene_.setSelections(selected);
    }

    scene_.setMode(Scene::Mode::SELECTING);
    qInfo() << "Mouse Release" << e->pos();
}
void VCanvas::wheelEvent(QWheelEvent *e) {
    scene_.scroll_x += e->pixelDelta().x() / 2.5;
    scene_.scroll_y += e->pixelDelta().y() / 2.5;
    qInfo() << "Wheel Event" << e->pixelDelta();
}
void VCanvas::loop() {
    counter++;
    update();
}
bool VCanvas::event(QEvent *e) {
    //qInfo() << "QEvent" << e;
    QNativeGestureEvent *nge;
    Qt::CursorShape cursor;

    switch (e->type()) {
    case QEvent::HoverMove:
        if (transform_box.hoverEvent(static_cast<QHoverEvent *>(e), &cursor)) {
            setCursor(cursor);
        } else {
            unsetCursor();
        }

        break;

    case QEvent::NativeGesture:
        //qInfo() << "Native Gesture!";
        nge = static_cast<QNativeGestureEvent *>(e);

        if (nge->gestureType() == Qt::ZoomNativeGesture) {
            double v = nge->value();
            scene_.scale += v;

            if (scene_.scale <= 0.01) {
                scene_.scale = 0.01;
            }
        }

        return false;

    default:
        break;
    }

    // Use parent handler
    return QQuickPaintedItem::event(e);
}
void VCanvas::editCut() {
    scene_.stackStep();
    qInfo() << "Edit Cut";
    editCopy();
    removeSelection();
}
void VCanvas::editCopy() {
    qInfo() << "Edit Copy";
    scene_.clipboard().clear();
    scene_.setClipboard(scene_.selections());
    scene_.pasting_shift = QPointF(0, 0);
}
void VCanvas::editPaste() {
    scene_.stackStep();
    qInfo() << "Edit Paste";
    int index_clip_begin = scene_.activeLayer().children().length();
    scene_.pasting_shift += QPointF(10, 10);
    QTransform shift = QTransform().translate(scene_.pasting_shift.x(), scene_.pasting_shift.y());

    for (int i = 0; i < scene_.clipboard().length() ; i++) {
        ShapePtr shape = scene_.clipboard().at(i)->clone();
        shape->applyTransform(shift);
        scene_.activeLayer().children().push_back(shape);
    }

    QList<ShapePtr> selected;

    for (int i = index_clip_begin; i < scene_.activeLayer().children().length(); i++) {
        selected << scene_.activeLayer().children().at(i);
    }

    scene_.setSelections(selected);
}
void VCanvas::editDelete() {
    scene_.stackStep();
    qInfo() << "Edit Delete";
    removeSelection();
}
void VCanvas::removeSelection() {
    scene_.stackStep();
    // Need to clean up all selection pointer reference first
    scene_.clearSelectionNoFlag();

    for (Layer &layer : scene_.layers()) {
        layer.children().erase(std::remove_if(layer.children().begin(), layer.children().end(), [](ShapePtr & s) {
            return s->selected;
        }), layer.children().end());
    }
}
void VCanvas::editUndo() {
    scene_.undo();
}
void VCanvas::editRedo() {
    scene_.redo();
}
void VCanvas::editSelectAll() {
    QList<ShapePtr> all_shapes;

    for (Layer &layer : scene_.layers()) {
        all_shapes.append(layer.children());
    }

    scene_.setSelections(all_shapes);
}
void VCanvas::editGroup() {
    if (scene_.selections().size() == 0) return;

    qInfo() << "Groupping";
    GroupShape *group = new GroupShape(transform_box.selections());

    for (Layer &layer : scene_.layers()) {
        layer.children().erase(std::remove_if(layer.children().begin(), layer.children().end(), [](ShapePtr & s) {
            return s->selected;
        }), layer.children().end());
    }

    const ShapePtr group_ptr(group);
    scene_.activeLayer().children().push_back(group_ptr);
    scene_.setSelection(group_ptr);
    scene_.stackStep();
}
void VCanvas::editUngroup() {
    qInfo() << "Groupping";
    ShapePtr group_ptr = scene_.selections().first();
    GroupShape *group = (GroupShape *) group_ptr.get();

    for (const ShapePtr &shape : group->children()) {
        shape->applyTransform(group->transform());
        scene_.activeLayer().children().push_back(shape);
    }

    scene_.setSelections(group->children());

    for (Layer &layer : scene_.layers()) {
        layer.children().removeOne(group_ptr);
    }

    scene_.stackStep();
}

Scene &VCanvas::scene() {
    return scene_;
}

Scene *VCanvas::scenePtr() {
    return &scene_;
}
