#include "vcanvas.h"
#include <QDebug>
#include <QPainter>
#include <QWidget>
#include <QCursor>
#include <QHoverEvent>
#include <cstring>
#include <iostream>

void VCanvas::loadSvg(QByteArray &data) {
    canvas_data.clearSelection();
    shapes.clear();
    bool success = svgpp_parser.parse(data);

    if (shapes.length() > 5000) {
        setAntialiasing(false);
    }

    if (success) {
        ready = true;
        update();
    }
}


VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent),
    svgpp_parser { SVGPPParser(&this->shapes) }, transform_box { TransformBox(this->canvas_data) } {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(true);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VCanvas::loop);
    timer->start(16);
    canvas_data.scroll_x = 0;
    canvas_data.scroll_y = 0;
    canvas_data.scale = 1;
    canvas_data.setMode(CanvasData::Mode::SELECTING);
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::paint(QPainter *painter) {
    painter->translate(canvas_data.scroll());
    painter->scale(canvas_data.scale, canvas_data.scale);
    //Test selection box
    //transformBox.rotate(0.2);
    QPen dash_pen = QPen(Qt::black, 2, Qt::DashLine);
    dash_pen.setDashPattern(QVector<qreal>(10, 3));
    dash_pen.setCosmetic(true);
    dash_pen.setDashOffset(counter);
    QPen solid_pen = QPen(Qt::black, 0, Qt::SolidLine);
    QColor sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    QColor sky_blue_alpha = QColor::fromRgb(0x00, 0x99, 0xCC, 30);
    QPen multi_select_pen = QPen(sky_blue, 0, Qt::DashLine);
    QBrush multi_select_brush = QBrush(sky_blue_alpha);

    for (int i = 0; i < shapes.size(); i++) {
        painter->save();

        if (shapes[i].selected) {
            painter->setPen(dash_pen);
        } else {
            painter->setPen(solid_pen);
        }

        // shapes[i].rot += 0.01;
        // scaleX = shapes[i].scaleY = 1.0 + sin(0.01 * counter)
        painter->setTransform(shapes[i].globalTransform(), true);
        painter->drawPath(shapes[i].path);
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

    if (canvas_data.mode() == CanvasData::Mode::MULTI_SELECTING) {
        painter->setBrush(multi_select_brush);
        painter->setPen(multi_select_ben);
        painter->fillRect(selection_box, multi_select_brush);
        painter->drawRect(selection_box);
    }

    //qInfo() << "Offset" << scrollX << scrollY << "Scale" << scale;
}

void VCanvas::mousePressEvent(QMouseEvent *e) {
    QPointF canvas_coord = canvas_data.getCanvasCoord(e->pos());
    qInfo() << "Mouse Press" << e->pos();
    mouse_press = e->pos();
    qInfo() << "Click point" << canvas_coord;

    // Test transform_box control point
    if (transform_box.mousePressEvent(e)) return;

    // Test object selection
    bool selectedObject = false;

    for (int i = 0; i < shapes.size(); i++) {
        if (shapes[i].testHit(canvas_coord, 15 / canvas_data.scale)) {
            selectedObject = true;

            // Do not reset transform box if it's already selected
            if (canvas_data.isSelected(&shapes[i])) {
                break;
            }

            // Reset transform box's target
            canvas_data.setSelection(&shapes[i]);
            break;
        }
    }

    if (selectedObject) {
        mouse_drag = true;
        canvas_data.setMode(CanvasData::Mode::SELECTING);
        return;
    }

    canvas_data.clearSelection();
    // Multi Select
    mouse_drag = true;
    canvas_data.setMode(CanvasData::Mode::MULTI_SELECTING);
    selection_box = QRectF(0, 0, 0, 0);
    selection_start = canvas_coord;
    return;
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
    QPointF canvas_coord = canvas_data.getCanvasCoord(e->pos());

    if (transform_box.mouseMoveEvent(e)) return;

    if (canvas_data.mode() == CanvasData::Mode::MULTI_SELECTING) {
        //QRectF a()
        //QSizeF rsize(abs(e->pos().x() - mouse_press.x()), abs(e->pos().y() - mouse_press.y()));
        selection_box = QRectF(selection_start, canvas_coord);
    }

    qInfo() << "Mouse Move" << e->pos();
}

void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (transform_box.mouseReleaseEvent(e)) return;

    if (canvas_data.mode() == CanvasData::Mode::MULTI_SELECTING) {
        QList<Shape *> selected;
        qInfo() << "Selection Box" << selection_box;

        for (int i = 0 ; i < shapes.size(); i++) {
            if (shapes[i].testHit(selection_box)) {
                selected << &shapes[i];
            }
        }

        qInfo() << "Mouse Release Selected" << selected.size();
        canvas_data.setSelections(selected);
    }

    canvas_data.setMode(CanvasData::Mode::SELECTING);
    qInfo() << "Mouse Release" << e->pos();
}

void VCanvas::wheelEvent(QWheelEvent *e) {
    canvas_data.scroll_x += e->pixelDelta().x() / 2.5;
    canvas_data.scroll_y += e->pixelDelta().y() / 2.5;
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
            canvas_data.scale += v;

            if (canvas_data.scale <= 0.01) {
                canvas_data.scale = 0.01;
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
    qInfo() << "Edit Cut";
    editCopy();
    editDelete();
}
void VCanvas::editCopy() {
    qInfo() << "Edit Copy";
    shape_clipboard.clear();
    std::copy_if(shapes.begin(), shapes.end(), std::back_inserter(shape_clipboard), [](Shape & s) {
        return s.selected;
    });
    std::for_each(shape_clipboard.begin(), shape_clipboard.end(),  [](Shape & s) {
        s.selected = false;
    });
}
void VCanvas::editPaste() {
    qInfo() << "Edit Paste";
    int index_clip_begin = shapes.length();
    shapes.append(shape_clipboard);
    QList<Shape *> selected;

    for (int i = index_clip_begin; i < shapes.length(); i++) {
        selected << &shapes[i];
    }

    canvas_data.setSelections(selected);
}
void VCanvas::editDelete() {
    qInfo() << "Edit Delete";
    // Need to clean up all selection pointer reference first
    canvas_data.clearSelectionNoFlag();
    qInfo() << "Clean upped";
    shapes.erase(std::remove_if(shapes.begin(), shapes.end(), [](Shape & s) {
        return s.selected;
    }), shapes.end());
}
