#include "vcanvas.h"
#include <QDebug>
#include <QPainter>
#include <QWidget>
#include <QCursor>
#include <QHoverEvent>
#include <cstring>
#include <iostream>

void VCanvas::loadSvg(QByteArray &data) {
    transform_box.clear();
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
    svgpp_parser { SVGPPParser(&this->shapes) } {
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
    canvas_data.mode = CanvasData::Mode::SELECTING;
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::paint(QPainter *painter) {
    painter->translate(canvas_data.scroll());
    painter->scale(canvas_data.scale, canvas_data.scale);
    //Test selection box
    //transformBox.rotate(0.2);
    QPen dashPen = QPen(Qt::black, 2, Qt::DashLine);
    dashPen.setDashPattern(QVector<qreal>(10, 3));
    dashPen.setCosmetic(true);
    dashPen.setDashOffset(counter);
    QPen solidPen = QPen(Qt::black, 0, Qt::SolidLine);
    QPen greenPen = QPen(Qt::green, 0, Qt::DashLine);
    QColor sky_blue = QColor::fromRgb(0x00, 0x99, 0xCC, 255);
    QColor sky_blue_alpha = QColor::fromRgb(0x00, 0x99, 0xCC, 30);
    QPen bluePen = QPen(QBrush(sky_blue), 0, Qt::DashLine);
    QPen redPen = QPen(Qt::red, 0, Qt::DashLine);
    QPen multiSelectPen = QPen(sky_blue, 0, Qt::DashLine);
    QBrush multiSelectBrush = QBrush(sky_blue_alpha);
    QPen ptPen(sky_blue, 10 / canvas_data.scale, Qt::PenStyle::SolidLine, Qt::RoundCap);

    for (int i = 0; i < shapes.size(); i++) {
        painter->save();

        if (shapes[i].selected) {
            painter->setPen(dashPen);
        } else {
            painter->setPen(solidPen);
        }

        // shapes[i].rot += 0.01;
        // scaleX = shapes[i].scaleY = 1.0 + sin(0.01 * counter)
        painter->setTransform(shapes[i].globalTransform(), true);
        painter->drawPath(shapes[i].path);
        QTransform t = shapes[i].globalTransform();
        QString str;
        QTextStream(&str) << "T" << t.m11() << "," <<
                          t.m12() << "," <<
                          t.m13() << "," <<
                          t.m21() << "," <<
                          t.m22() << "," <<
                          t.m23() << "," <<
                          t.m31() << "," <<
                          t.m32() << "," <<
                          t.m33();
        // painter->drawText(-shapes[i].path.boundingRect().width() / 2, -shapes[i].path.boundingRect().height() / 2, str);
        painter->restore();
        //Draw bounding box
        /*if (shapes[i].selected) {
            QRectF bbox = shapes[i].boundingRect();
            painter->setPen(greenPen);
            painter->drawRect(bbox);
        }*/
    }

    //Draw transform box
    if (transform_box.hasTargets()) {
        painter->setPen(bluePen);
        painter->drawPolyline(transform_box.controlPoints(), 8);
        painter->drawLine(transform_box.controlPoints()[7], transform_box.controlPoints()[0]);
        painter->setPen(ptPen);
        painter->drawPoints(transform_box.controlPoints(), 8);
    }

    if (canvas_data.mode == CanvasData::Mode::MULTI_SELECTING) {
        painter->setBrush(multiSelectBrush);
        painter->setPen(multiSelectPen);
        painter->fillRect(selection_box, multiSelectBrush);
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
    if (transform_box.mousePressEvent(e, canvas_data)) return;

    // Test object selection
    bool selectedObject = false;

    for (int i = 0; i < shapes.size(); i++) {
        if (shapes[i].testHit(canvas_coord)) {
            selectedObject = true;

            // Do not reset transform box if it's already selected
            if (transform_box.containsTarget(&shapes[i])) {
                break;
            }

            // Reset transform box's target
            transform_box.setTarget(&shapes[i]);
            break;
        }
    }

    if (selectedObject) {
        mouse_drag = true;
        canvas_data.mode = CanvasData::Mode::SELECTING;
        return;
    }

    transform_box.clear();
    // Multi Select
    mouse_drag = true;
    canvas_data.mode = CanvasData::Mode::MULTI_SELECTING;
    selection_box = QRectF(0, 0, 0, 0);
    selection_start = canvas_coord;
    return;
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
    QPointF canvas_coord = canvas_data.getCanvasCoord(e->pos());

    if (transform_box.mouseMoveEvent(e, canvas_data)) return;

    if (canvas_data.mode == CanvasData::Mode::MULTI_SELECTING) {
        //QRectF a()
        //QSizeF rsize(abs(e->pos().x() - mouse_press.x()), abs(e->pos().y() - mouse_press.y()));
        selection_box = QRectF(selection_start, canvas_coord);
    }

    qInfo() << "Mouse Move" << e->pos();
}

void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
    if (transform_box.mouseReleaseEvent(e, canvas_data)) return;

    if (canvas_data.mode == CanvasData::Mode::MULTI_SELECTING) {
        QList<Shape *> selected;
        qInfo() << "Selection Box" << selection_box;

        for (int i = 0 ; i < shapes.size(); i++) {
            if (shapes[i].testHit(selection_box)) {
                selected << &shapes[i];
            }
        }

        qInfo() << "Mouse Release Selected" << selected.size();
        transform_box.setTargets(selected);
    }

    canvas_data.mode = CanvasData::Mode::SELECTING;
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
        if (transform_box.hoverEvent(static_cast<QHoverEvent *>(e), canvas_data, &cursor)) {
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
