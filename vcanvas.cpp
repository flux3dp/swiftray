#include "vcanvas.h"
#include <QDebug>
#include <QPainter>
#include <QWidget>
#include <QCursor>
#include <QHoverEvent>
#include <cstring>
#include <iostream>

VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent),
    svgpp_parser { SVGPPParser(shapes()) }, transform_box { TransformBox(data) } {
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

    if (shapes().length() > 5000) {
        setAntialiasing(false);
    }

    if (success) {
        data.stackStep();
        editSelectAll();
        forceActiveFocus();
        ready = true;
        update();
    }
}

QList<ShapePtr> &VCanvas::shapes() {
    return data.shapes();
}

void VCanvas::paint(QPainter *painter) {
    //qInfo() << "Rendering engine = " << painter->paintEngine()->type();
    QPen dash_pen = QPen(Qt::black, 2, Qt::DashLine);
    dash_pen.setDashPattern(QVector<qreal>(10, 3));
    dash_pen.setCosmetic(true);
    dash_pen.setDashOffset(counter);
    painter->translate(data.scroll());
    painter->scale(data.scale, data.scale);
    //Test selection box
    //transformBox.rotate(0.2);
    QPen solid_pen = QPen(Qt::black, 0, Qt::SolidLine);
    QColor sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    QColor sky_blue_alpha = QColor::fromRgb(0x00, 0x99, 0xCC, 30);
    QPen multi_select_pen = QPen(sky_blue, 0, Qt::DashLine);
    QBrush multi_select_brush = QBrush(sky_blue_alpha);

    for (int i = 0; i < shapes().size(); i++) {
        painter->save();

        if (shapes().at(i)->selected) {
            painter->setPen(dash_pen);
        } else {
            painter->setPen(solid_pen);
        }

        // shapes[i].rot += 0.01;
        // scaleX = shapes[i].scaleY = 1.0 + sin(0.01 * counter)
        painter->setTransform(shapes().at(i)->transform(), true);
        shapes().at(i)->paint(painter);
        painter->restore();
        //Draw bounding box
        /*if (shapes[i].selected) {
            QRectF bbox = shapes[i].boundingRect();
            painter->setPen(greenPen);
            painter->drawRect(bbox);
        }*/
    }

    //Draw transform box
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

    for (int i = shapes().size() - 1 ; i >= 0; i--) {
        if (shapes().at(i)->testHit(canvas_coord, 15 / data.scale)) {
            selectedObject = true;

            // Do not reset transform box if it's already selected
            if (data.isSelected(shapes().at(i))) {
                break;
            }

            // Reset transform box's target
            data.setSelection(shapes().at(i));
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

        for (int i = 0 ; i < shapes().length(); i++) {
            if (shapes().at(i)->testHit(selection_box)) {
                selected << shapes().at(i);
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
    // qInfo() << "QEvent" << e;
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
    data.shape_clipboard.clear();

    for (int i = 0; i < shapes().length() ; i++) {
        if (shapes().at(i)->selected) {
            ShapePtr shape = shapes().at(i)->clone();
            shape->selected = false;
            data.shape_clipboard.push_back(shape);
        }
    }
}
void VCanvas::editPaste() {
    data.stackStep();
    qInfo() << "Edit Paste";
    int index_clip_begin = shapes().length();

    for (int i = 0; i < data.shape_clipboard.length() ; i++) {
        shapes().push_back(data.shape_clipboard.at(i)->clone());
    }

    QList<ShapePtr> selected;

    for (int i = index_clip_begin; i < shapes().length(); i++) {
        selected << shapes().at(i);
    }

    data.setSelections(selected);
}
void VCanvas::editDelete() {
    data.stackStep();
    qInfo() << "Edit Delete";
    removeSelection();
}

void VCanvas::removeSelection() {
    // Need to clean up all selection pointer reference first
    data.clearSelectionNoFlag();
    shapes().erase(std::remove_if(shapes().begin(), shapes().end(), [](ShapePtr & s) {
        return s->selected;
    }), shapes().end());
}

void VCanvas::editUndo() {
    data.undo();
}

void VCanvas::editRedo() {
    data.redo();
}

void VCanvas::editSelectAll() {
    data.setSelections(shapes());
}
