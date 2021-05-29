#include "vcanvas.h"
#include <QDebug>
#include <QPainter>
#include <QWidget>
#include <QCursor>
#include <QHoverEvent>
#include <cstring>
#include <iostream>
#include <shape/group_shape.h>
#include <canvas/layer.hpp>

VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent),
    svgpp_parser { SVGPPParser(data) }, transform_box { TransformBox(data) } {
    setRenderTarget(RenderTarget::FramebufferObject);
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setAntialiasing(true);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VCanvas::loop);
    timer->start(16);
    data.scroll_x = 0;
    data.scroll_y = 0;
    data.scale = 1;
    data.setMode(CanvasData::Mode::SELECTING);
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::loadSvg(QByteArray &svg_data) {
    data.clear();
    bool success = svgpp_parser.parse(svg_data);
    setAntialiasing(false);

    if (success) {
        data.stackStep();
        editSelectAll();
        forceActiveFocus();
        ready = true;
        update();
    }
}

void VCanvas::paint(QPainter *painter) {
    painter->translate(data.scroll());
    painter->scale(data.scale, data.scale);
    //qInfo() << "Rendering engine = " << painter->paintEngine()->type();
    QColor sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    QColor sky_blue_alpha = QColor::fromRgb(0x00, 0x99, 0xCC, 30);
    QPen multi_select_pen = QPen(sky_blue, 0, Qt::DashLine);
    QBrush multi_select_brush = QBrush(sky_blue_alpha);

    // Draw layers
    for (const Layer &layer : data.layers()) {
        layer.paint(painter, counter);
    }

    // Draw transform box
    transform_box.paint(painter);

    if (data.mode() == CanvasData::Mode::MULTI_SELECTING) {
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
    QPointF canvas_coord = data.getCanvasCoord(e->pos());
    qInfo() << "Mouse Press" << e->pos();
    mouse_press = e->pos();
    qInfo() << "Click point" << canvas_coord;

    // Test transform_box control point
    if (transform_box.mousePressEvent(e)) return;

    // Test object selection in reverse
    bool selectedObject = false;

    for (Layer &layer : data.layers()) {
        for (ShapePtr &shape : layer.children()) {
            if (shape->testHit(canvas_coord, 15 / data.scale)) {
                selectedObject = true;

                // Do not reset selection if it's already selected
                if (data.isSelected(shape)) {
                    break;
                }

                // Reset selection
                data.setSelection(shape);
                break;
            }
        }

        if (selectedObject) {
            break;
        }
    }

    if (selectedObject) {
        mouse_drag = true;
        data.setMode(CanvasData::Mode::SELECTING);
        return;
    }

    data.clearSelection();
    // Multi Select
    mouse_drag = true;
    data.setMode(CanvasData::Mode::MULTI_SELECTING);
    selection_box = QRectF(0, 0, 0, 0);
    selection_start = canvas_coord;
    return;
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
    QPointF canvas_coord = data.getCanvasCoord(e->pos());

    if (transform_box.mouseMoveEvent(e)) return;

    if (data.mode() == CanvasData::Mode::MULTI_SELECTING) {
        //QRectF a()
        //QSizeF rsize(abs(e->pos().x() - mouse_press.x()), abs(e->pos().y() - mouse_press.y()));
        selection_box = QRectF(selection_start, canvas_coord);
    }

    qInfo() << "Mouse Move" << e->pos();
}
void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (transform_box.mouseReleaseEvent(e)) return;

    if (data.mode() == CanvasData::Mode::MULTI_SELECTING &&
        (selection_box.width() != 0 || selection_box.height() != 0)) {
        QList<ShapePtr> selected;
        qInfo() << "Selection Box" << selection_box;

        for (Layer &layer : data.layers()) {
            for (ShapePtr &shape : layer.children()) {
                if (shape->testHit(selection_box)) {
                    selected << shape;
                }
            }
        }

        qInfo() << "Mouse Release Selected" << selected.size();
        data.setSelections(selected);
    }

    data.setMode(CanvasData::Mode::SELECTING);
    qInfo() << "Mouse Release" << e->pos();
}
void VCanvas::wheelEvent(QWheelEvent *e) {
    data.scroll_x += e->pixelDelta().x() / 2.5;
    data.scroll_y += e->pixelDelta().y() / 2.5;
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
            data.scale += v;

            if (data.scale <= 0.01) {
                data.scale = 0.01;
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
    data.stackStep();
    qInfo() << "Edit Cut";
    editCopy();
    removeSelection();
}
void VCanvas::editCopy() {
    qInfo() << "Edit Copy";
    data.clipboard().clear();
    data.setClipboard(data.selections());
    data.pasting_shift = QPointF(0, 0);
}
void VCanvas::editPaste() {
    data.stackStep();
    qInfo() << "Edit Paste";
    int index_clip_begin = data.activeLayer().children().length();
    data.pasting_shift += QPointF(10, 10);
    QTransform shift = QTransform().translate(data.pasting_shift.x(), data.pasting_shift.y());

    for (int i = 0; i < data.clipboard().length() ; i++) {
        ShapePtr shape = data.clipboard().at(i)->clone();
        shape->applyTransform(shift);
        data.activeLayer().children().push_back(shape);
    }

    QList<ShapePtr> selected;

    for (int i = index_clip_begin; i < data.activeLayer().children().length(); i++) {
        selected << data.activeLayer().children().at(i);
    }

    data.setSelections(selected);
}
void VCanvas::editDelete() {
    data.stackStep();
    qInfo() << "Edit Delete";
    removeSelection();
}
void VCanvas::removeSelection() {
    data.stackStep();
    // Need to clean up all selection pointer reference first
    data.clearSelectionNoFlag();

    for (Layer &layer : data.layers()) {
        layer.children().erase(std::remove_if(layer.children().begin(), layer.children().end(), [](ShapePtr & s) {
            return s->selected;
        }), layer.children().end());
    }
}
void VCanvas::editUndo() {
    data.undo();
}
void VCanvas::editRedo() {
    data.redo();
}
void VCanvas::editSelectAll() {
    QList<ShapePtr> all_shapes;

    for (Layer &layer : data.layers()) {
        all_shapes.append(layer.children());
    }

    data.setSelections(all_shapes);
}
void VCanvas::editGroup() {
    if (data.selections().size() == 0) return;

    qInfo() << "Groupping";
    GroupShape *group = new GroupShape(transform_box.selections());

    for (Layer &layer : data.layers()) {
        layer.children().erase(std::remove_if(layer.children().begin(), layer.children().end(), [](ShapePtr & s) {
            return s->selected;
        }), layer.children().end());
    }

    const ShapePtr group_ptr(group);
    data.activeLayer().children().push_back(group_ptr);
    data.setSelection(group_ptr);
    data.stackStep();
}
void VCanvas::editUngroup() {
    qInfo() << "Groupping";
    ShapePtr group_ptr = data.selections().first();
    GroupShape *group = (GroupShape *) group_ptr.get();

    for (const ShapePtr &shape : group->children()) {
        shape->applyTransform(group->transform());
        data.activeLayer().children().push_back(shape);
    }

    data.setSelections(group->children());

    for (Layer &layer : data.layers()) {
        layer.children().removeOne(group_ptr);
    }

    data.stackStep();
}
