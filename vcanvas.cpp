#include "vcanvas.h"
#include <QDebug>
#include <QPainter>
#include <QWidget>
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
    scroll_x = 0;
    scroll_y = 0;
    scale = 1;
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::paint(QPainter *painter) {
    painter->translate(scroll_x, scroll_y);
    painter->scale(scale, scale);
    //Test selection box
    //transformBox.rotate(0.2);
    QPen dashPen = QPen(Qt::blue, 2, Qt::DashLine);
    dashPen.setDashPattern(QVector<qreal>(10, 3));
    dashPen.setCosmetic(true);
    dashPen.setDashOffset(counter);
    QPen solidPen = QPen(Qt::black, 0, Qt::SolidLine);
    QPen greenPen = QPen(Qt::green, 0, Qt::DashLine);

    for (int i = 0; i < shapes.size(); i++) {
        painter->save();

        if (shapes[i].selected) {
            painter->setPen(dashPen);
        } else {
            painter->setPen(solidPen);
        }

        // shapes[i].rot += 0.01;
        // scaleX = shapes[i].scaleY = 1.0 + sin(0.01 * counter)
        painter->translate(shapes[i].x, shapes[i].y);
        painter->rotate(shapes[i].rot);
        painter->scale(shapes[i].scale_x, shapes[i].scale_y);
        painter->drawPath(shapes[i].path);
        painter->restore();

        //Draw bounding box
        if (shapes[i].selected) {
            QRectF bbox = shapes[i].boundingRect();
            painter->setPen(greenPen);
            painter->drawRect(bbox);
        }
    }

    //Draw transform box
    painter->drawPoints(transform_box.controlPoints(), 8);
    //qInfo() << "Offset" << scrollX << scrollY << "Scale" << scale;
}

void VCanvas::mousePressEvent(QMouseEvent *e) {
    qInfo() << "Mouse Press" << e->pos();
    mouse_press = e->pos();
    QPointF clickPoint = (e->pos() - QPointF(scroll_x, scroll_y)) / scale;
    qInfo() << "Click point" << clickPoint;
    bool selectedObject = false;

    for (int i = 0; i < shapes.size(); i++) {
        if (shapes[i].testHit(clickPoint)) {
            transform_box.setTarget(&shapes[i]);
            selectedObject = true;
            break;
        }
    }

    if (selectedObject) {
        mouse_drag = true;
    } else {
        transform_box.clear();
    }
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
    // If we've moved more then 25 pixels, assume user is dragging
    if (mouse_drag) {
        transform_box.move((e->pos() - mouse_press) / scale);
        mouse_press = e->pos();
    }

    qInfo() << "Mouse Move" << e->pos();
}

void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
    qInfo() << "Mouse Release" << e->pos();
}

void VCanvas::wheelEvent(QWheelEvent *e) {
    scroll_x += e->pixelDelta().x() / 2.5;
    scroll_y += e->pixelDelta().y() / 2.5;
    qInfo() << "Wheel Event" << e->pixelDelta();
}

void VCanvas::loop() {
    counter++;
    update();
}


bool VCanvas::event(QEvent *e) {
    //qInfo() << "QEvent" << e;
    QNativeGestureEvent *nge;

    switch (e->type()) {
    case QEvent::NativeGesture:
        //qInfo() << "Native Gesture!";
        nge = static_cast<QNativeGestureEvent *>(e);

        if (nge->gestureType() == Qt::ZoomNativeGesture) {
            double v = nge->value();
            scale += v;

            if (scale <= 0.01) {
                scale = 0.01;
            }
        }

        return false;

    default:
        break;
    }

    // Use parent handler
    return QQuickPaintedItem::event(e);
}
