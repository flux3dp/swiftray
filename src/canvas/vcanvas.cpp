#include <canvas/vcanvas.h>
#include <QDebug>
#include <QPainter>
#include <QWidget>
#include <QCursor>
#include <QHoverEvent>
#include <cstring>
#include <iostream>
#include <shape/path_shape.h>
#include <shape/group_shape.h>
#include <canvas/layer.h>

VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent),
    svgpp_parser { SVGPPParser(scene()) },
    transform_box_ { TransformBox(scene()) },
    multi_selection_box_ { MultiSelectionBox(scene()) },
    rect_drawer_ { RectDrawer(scene()) },
    oval_drawer_ { OvalDrawer(scene()) },
    line_drawer_ { LineDrawer(scene()) },
    path_drawer_ { PathDrawer(scene()) },
    path_editor_ { PathEditor(scene()) },
    grid_ { Grid(scene()) },
    text_drawer_ { TextDrawer(scene()) } {
    setRenderTarget(RenderTarget::FramebufferObject);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setAntialiasing(true);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VCanvas::loop);
    timer->start(16);
    scene().setMode(Scene::Mode::SELECTING);
    controls_ << &transform_box_
              << &multi_selection_box_
              << &rect_drawer_
              << &oval_drawer_
              << &line_drawer_
              << &path_drawer_
              << &path_editor_
              << &text_drawer_;
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::loadSVG(QByteArray &svg_data) {
    //scene().clearAll();
    //scene().addLayer();
    bool success = svgpp_parser.parse(svg_data);
    setAntialiasing(false);

    if (success) {
        editSelectAll();
        scene().stackStep();
        forceActiveFocus();
        ready = true;
        update();
    }
}

void VCanvas::paint(QPainter *painter) {
    painter->fillRect(0, 0, width(), height(), QColor("#F0F0F0"));
    painter->translate(scene().scroll());
    painter->scale(scene().scale(), scene().scale());

    grid_.paint(painter);

    for (const LayerPtr &layer : scene().layers()) {
        layer->paint(painter, counter);
    }

    for (CanvasControl *control : controls_) {
        control->paint(painter);
    }
}

void VCanvas::keyPressEvent(QKeyEvent *e) {
    // qInfo() << "Key press" << e;

    for (CanvasControl *control : controls_) {
        if (control->keyPressEvent(e)) return;
    }

    if (e->key() == Qt::Key::Key_Delete || e->key() == Qt::Key::Key_Backspace || e->key() == Qt::Key::Key_Back) {
        editDelete();
    }
}

void VCanvas::mousePressEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    qInfo() << "Mouse Press (screen)" << e->pos() << " -> (canvas)" << canvas_coord;

    for (CanvasControl *control : controls_) {
        if (control->mousePressEvent(e)) return;
    }

    if (scene().mode() == Scene::Mode::SELECTING) {
        ShapePtr hit = scene().testHit(canvas_coord);

        if (hit != nullptr) {
            if (!hit->selected) {
                scene().setSelection(hit);
            }
        } else {
            scene().clearSelections();
            scene().setMode(Scene::Mode::MULTI_SELECTING);
        }
    }
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    qInfo() << "Mouse Move (screen)" << e->pos() << " -> (canvas)" << canvas_coord;

    for (CanvasControl *control : controls_) {
        if (control->mouseMoveEvent(e)) return;
    }
}

void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    qInfo() << "Mouse Release (screen)" << e->pos() << " -> (canvas)" << canvas_coord;

    for (CanvasControl *control : controls_) {
        if (control->mouseReleaseEvent(e)) return;
    }

    scene().setMode(Scene::Mode::SELECTING);
}

void VCanvas::mouseDoubleClickEvent(QMouseEvent *e) {
    QPointF canvas_coord = scene().getCanvasCoord(e->pos());
    qInfo() << "Mouse Double Click (screen)" << e->pos() << " -> (canvas)" << canvas_coord;
    qInfo() << "Mode" << (int)scene().mode();
    ShapePtr hit = scene().testHit(canvas_coord);
    if (scene().mode() == Scene::Mode::SELECTING) {
        if (hit != nullptr) {
            qInfo() << "Double clicked" << hit.get();
            switch (hit->type()) {
                case Shape::Type::Path:
                    scene().clearSelections();
                    path_editor_.setTarget(hit);
                    scene().setMode(Scene::Mode::EDITING_PATH);
                    break;
                case Shape::Type::Text:
                    scene().clearSelections();
                    text_drawer_.setTarget(hit);
                    scene().setMode(Scene::Mode::DRAWING_TEXT);
                    break;
                default:
                    break;
            }
        } 
    } else if (scene().mode() == Scene::Mode::EDITING_PATH) {
        scene().setMode(Scene::Mode::SELECTING);
        scene().clearSelections();
        path_editor_.reset();
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
    //qInfo() << "QEvent" << e;
    QNativeGestureEvent *nge;
    Qt::CursorShape cursor;

    switch (e->type()) {
    case QEvent::HoverMove:
        unsetCursor();
        for(CanvasControl *control : controls_) {
            if(control->hoverEvent(static_cast<QHoverEvent *>(e), &cursor)){
                setCursor(cursor);
                break;
            }
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
    if (scene().mode() != Scene::Mode::SELECTING) return;
    scene().stackStep();
    qInfo() << "Edit Cut";
    editCopy();
    scene().removeSelections();
}
void VCanvas::editCopy() {
    if (scene().mode() != Scene::Mode::SELECTING) return;
    qInfo() << "Edit Copy";
    scene().clipboard().clear();
    scene().setClipboard(scene().selections());
    scene().pasting_shift = QPointF(0, 0);
}

void VCanvas::editPaste() {
    if (scene().mode() != Scene::Mode::SELECTING) return;
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

void VCanvas::editDrawLine() {
    line_drawer_.reset();
    scene().clearSelections();
    scene().setMode(Scene::Mode::DRAWING_LINE);
}

void VCanvas::editDrawPath() {
    path_drawer_.reset();
    scene().clearSelections();
    scene().setMode(Scene::Mode::DRAWING_PATH);
}

void VCanvas::editDrawText() {
    text_drawer_.reset();
    scene().clearSelections();
    scene().setMode(Scene::Mode::DRAWING_TEXT);
}

void VCanvas::editSelectAll() {
    if (scene().mode() != Scene::Mode::SELECTING) return;
    QList<ShapePtr> all_shapes;

    for (LayerPtr &layer : scene().layers()) {
        all_shapes.append(layer->children());
    }

    scene().setSelections(all_shapes);
}
void VCanvas::editGroup() {
    if (scene().selections().size() == 0) return;

    qInfo() << "Groupping";
    scene().stackStep();
    scene().removeSelections();
    const ShapePtr group_ptr = make_shared<GroupShape>(transform_box_.selections());
    scene().activeLayer().children().push_back(group_ptr);
    scene().setSelection(group_ptr);
}
void VCanvas::editUngroup() {
    qInfo() << "Groupping";
    scene().stackStep();
    ShapePtr group_ptr = scene().selections().first();
    GroupShape *group = (GroupShape *) group_ptr.get();

    for (const ShapePtr &shape : group->children()) {
        shape->applyTransform(group->transform());
        scene().activeLayer().children().push_back(shape);
    }

    scene().setSelections(group->children());

    for (LayerPtr &layer : scene().layers()) {
        layer->children().removeOne(group_ptr);
    }

}

Scene &VCanvas::scene() {
    return scene_;
}

void VCanvas::editUnion() {
    if (scene().selections().size() < 2) return;
    QPainterPath result;

    for (ShapePtr &shape : scene().selections()) {
        if (shape->type() != Shape::Type::Path && shape->type() != Shape::Type::Text) return; 
        result = result.united(shape->transform().map(dynamic_cast<PathShape*>(shape.get())->path()));
    }
    scene().stackStep();
    ShapePtr new_shape = make_shared<PathShape>(result);
    scene().removeSelections();
    scene().activeLayer().addShape(new_shape);
    scene().setSelection(new_shape);
}
void VCanvas::editSubtract() {
    if (scene().selections().size() != 2) return;

    if (scene().selections().at(0)->type() != Shape::Type::Path ||
        scene().selections().at(1)->type() !=  Shape::Type::Path) return;

    scene().stackStep();
    PathShape *a = dynamic_cast<PathShape*>(scene().selections().at(0).get());
    PathShape *b = dynamic_cast<PathShape*>(scene().selections().at(1).get());
    QPainterPath new_path(a->transform().map(a->path()).subtracted(b->transform().map(b->path())));
    ShapePtr new_shape = make_shared<PathShape>(new_path);
    scene().removeSelections();
    scene().activeLayer().addShape(new_shape);
    scene().setSelection(new_shape);
}
void VCanvas::editIntersect() {
    if (scene().selections().size() != 2) return;

    if (scene().selections().at(0)->type() != Shape::Type::Path ||
        scene().selections().at(1)->type() !=  Shape::Type::Path) return;

    scene().stackStep();
    PathShape *a = dynamic_cast<PathShape*>(scene().selections().at(0).get());
    PathShape *b = dynamic_cast<PathShape*>(scene().selections().at(1).get());
    QPainterPath new_path(a->transform().map(a->path()).intersected(b->transform().map(b->path())));
    new_path.closeSubpath();
    ShapePtr new_shape = make_shared<PathShape>(new_path);
    scene().removeSelections();
    scene().activeLayer().addShape(new_shape);
    scene().setSelection(new_shape);
}
void VCanvas::editDifference() {
}
