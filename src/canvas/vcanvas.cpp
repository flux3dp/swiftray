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
    svgpp_parser { SVGPPParser(scene()) },
    transform_box_ { TransformBox(scene()) },
    rect_drawer_ { RectDrawer(scene()) },
    oval_drawer_ { OvalDrawer(scene()) },
    multi_selection_box_ { MultiSelectionBox(scene()) } {
    setRenderTarget(RenderTarget::FramebufferObject);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setAntialiasing(true);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VCanvas::loop);
    timer->start(16);
    scene().setMode(Scene::Mode::SELECTING);
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::loadSVG(QByteArray &svg_data) {
    //scene().clearAll();
    //scene().addLayer();
    bool success = svgpp_parser.parse(svg_data);
    setAntialiasing(false);

    if (success) {
        scene().stackStep();
        editSelectAll();
        forceActiveFocus();
        ready = true;
        update();
    }
}

void VCanvas::paint(QPainter *painter) {
    painter->translate(scene().scroll());
    painter->scale(scene().scale(), scene().scale());
    
    transform_box_.paint(painter);
    multi_selection_box_.paint(painter);
    rect_drawer_.paint(painter);
    oval_drawer_.paint(painter);

    for (const Layer &layer : scene().layers()) {
        layer.paint(painter, counter);
    }
    
}

void VCanvas::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key::Key_Delete || e->key() == Qt::Key::Key_Backspace || e->key() == Qt::Key::Key_Back) {
        editDelete();
    }
}

void VCanvas::mousePressEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    qInfo() << "Mouse Press (screen)" << e->pos() << " -> (canvas)" << canvas_coord;

    // Test transform_box control point
    if (transform_box_.mousePressEvent(e)) return;

    if (multi_selection_box_.mousePressEvent(e)) return;

    if (rect_drawer_.mousePressEvent(e)) return;

    if (oval_drawer_.mousePressEvent(e)) return;

    if (scene().mode() == Scene::Mode::SELECTING) {
        ShapePtr hit = scene().hitTest(canvas_coord);
        if (hit != nullptr) {
            if (!hit->selected) scene().setSelection(hit);
            scene().setMode(Scene::Mode::MOVING);
        } else {
            scene().clearSelections();
            scene().setMode(Scene::Mode::MULTI_SELECTING);
        }
    }
    return;
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    qInfo() << "Mouse Move (screen)" << e->pos() << " -> (canvas)" << canvas_coord;

    if (transform_box_.mouseMoveEvent(e)) return;

    if (multi_selection_box_.mouseMoveEvent(e)) return;

    if (rect_drawer_.mouseMoveEvent(e)) return;

    if (oval_drawer_.mouseMoveEvent(e)) return;

}

void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    qInfo() << "Mouse Release (screen)" << e->pos() << " -> (canvas)" << canvas_coord;

    if (transform_box_.mouseReleaseEvent(e)) return;

    if (multi_selection_box_.mouseReleaseEvent(e)) return;

    if (rect_drawer_.mouseReleaseEvent(e)) return;

    if (oval_drawer_.mouseReleaseEvent(e)) return;

    scene().setMode(Scene::Mode::SELECTING);
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
    //qInfo() << "QEvent" << e;
    QNativeGestureEvent *nge;
    Qt::CursorShape cursor;

    switch (e->type()) {
    case QEvent::HoverMove:
        if (transform_box_.hoverEvent(static_cast<QHoverEvent *>(e), &cursor)) {
            setCursor(cursor);
        } else {
            unsetCursor();
        }

        break;

    case QEvent::NativeGesture:
        nge = static_cast<QNativeGestureEvent *>(e);

        if (nge->gestureType() == Qt::ZoomNativeGesture) {
            scene().setScale(
                max(0.01, scene().scale() + nge->value())
            );
        }

        break;
    default:
        break;
    }

    return QQuickPaintedItem::event(e);
}

void VCanvas::editCut() {
    scene().stackStep();
    qInfo() << "Edit Cut";
    editCopy();
    scene().removeSelections();
}
void VCanvas::editCopy() {
    qInfo() << "Edit Copy";
    scene().clipboard().clear();
    scene().setClipboard(scene().selections());
    scene().pasting_shift = QPointF(0, 0);
}

void VCanvas::editPaste() {
    scene().stackStep();
    qInfo() << "Edit Paste";
    int index_clip_begin = scene().activeLayer().children().length();
    scene().pasting_shift += QPointF(10, 10);
    QTransform shift = QTransform().translate(scene().pasting_shift.x(), scene().pasting_shift.y());

    for (int i = 0; i < scene().clipboard().length() ; i++) {
        ShapePtr shape = scene().clipboard().at(i)->clone();
        shape->applyTransform(shift);
        scene().activeLayer().children().push_back(shape);
    }

    QList<ShapePtr> selected;

    for (int i = index_clip_begin; i < scene().activeLayer().children().length(); i++) {
        selected << scene().activeLayer().children().at(i);
    }

    scene().setSelections(selected);
}
void VCanvas::editDelete() {
    scene().stackStep();
    qInfo() << "Edit Delete";
    scene().removeSelections();
}

void VCanvas::editUndo() {
    scene().undo();
}

void VCanvas::editRedo() {
    scene().redo();
}

void VCanvas::editDrawRect() {
    rect_drawer_.reset();
    scene().clearSelections();
    scene().setMode(Scene::Mode::DRAWING_RECT);
}

void VCanvas::editDrawOval() {
    oval_drawer_.reset();
    scene().clearSelections();
    scene().setMode(Scene::Mode::DRAWING_OVAL);
}

void VCanvas::editSelectAll() {
    QList<ShapePtr> all_shapes;

    for (Layer &layer : scene().layers()) {
        all_shapes.append(layer.children());
    }

    scene().setSelections(all_shapes);
}
void VCanvas::editGroup() {
    if (scene().selections().size() == 0) return;

    qInfo() << "Groupping";
    GroupShape *group = new GroupShape(transform_box_.selections());

    scene().removeSelections();

    const ShapePtr group_ptr(group);
    scene().activeLayer().children().push_back(group_ptr);
    scene().setSelection(group_ptr);
    scene().stackStep();
}
void VCanvas::editUngroup() {
    qInfo() << "Groupping";
    ShapePtr group_ptr = scene().selections().first();
    GroupShape *group = (GroupShape *) group_ptr.get();

    for (const ShapePtr &shape : group->children()) {
        shape->applyTransform(group->transform());
        scene().activeLayer().children().push_back(shape);
    }

    scene().setSelections(group->children());

    for (Layer &layer : scene().layers()) {
        layer.children().removeOne(group_ptr);
    }

    scene().stackStep();
}

Scene &VCanvas::scene() {
    return scene_;
}