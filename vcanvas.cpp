#include "vcanvas.h"
#include <QDebug>
#include <QPainter>
#include <QWidget>
#include <cstring>
#include <iostream>

void VCanvas::loadSvg(QByteArray &data) {
    shapes.clear();
    bool success = svgppParser.parse(data);

    if (shapes.length() > 5000) {
        setAntialiasing(false);
    }

    if (success) {
        ready = true;
        update();
    }
}


VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent),
    svgppParser { SVGPPParser(&this->shapes) } {
    setAcceptedMouseButtons(Qt::AllButtons);
    setAcceptHoverEvents(true);
    setAcceptTouchEvents(true);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(true);
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VCanvas::loop);
    timer->start(30);
    scrollX = 0;
    scrollY = 0;
    scale = 1;
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::paint(QPainter *painter) {
    painter->translate(scrollX, scrollY);
    painter->scale(scale, scale);
    QPen pen = QPen(Qt::blue, 2, Qt::DashLine);
    pen.setDashPattern(QVector<qreal>(10, 3));
    pen.setDashOffset(counter);
    pen.setCosmetic(true);
    painter->setPen(pen);
    bool dashed = true;

    for (int i = 0; i < shapes.size(); i++) {
        if (shapes[i].selected && !dashed) {
            pen.setStyle(Qt::PenStyle::DashLine);
            pen.setWidth(2);
            pen.setCosmetic(true);
            pen.setDashPattern(QVector<qreal>(10, 3));
            pen.setDashOffset(counter);
            painter->setPen(pen);
            dashed = true;
        } else if (!shapes[i].selected && dashed) {
            pen.setStyle(Qt::PenStyle::SolidLine);
            pen.setWidth(0);
            painter->setPen(pen);
            dashed = false;
        }

        painter->save();
        pen.setColor(Qt::blue);
        painter->setPen(pen);
        painter->translate(shapes[i].x, shapes[i].y);
        painter->drawPath(shapes[i].path);
        painter->restore();
        /*// Draw cache poly
        pen.setColor(Qt::red);
        painter->setPen(pen);
        painter->drawPolyline(shapes[i].polyCache.toVector().data(), shapes[i].polyCache.size());
        // Draw bounding box
        QRectF bbox = shapes[i].path.boundingRect();
        QRectF newBBox(bbox.x() + shapes[i].x - 10, bbox.y() + shapes[i].y - 10, bbox.width() + 20, bbox.height() + 20);
        pen.setColor(Qt::green);
        painter->setPen(pen);
        painter->drawRect(newBBox);*/
    }

    //qInfo() << "Offset" << scrollX << scrollY << "Scale" << scale;
}

void VCanvas::mousePressEvent(QMouseEvent *e) {
    qInfo() << "Mouse Press" << e->pos();
    m_mouseDrag = true;
    m_mousePress = e->pos();
    QList<Shape *> boundingShapes;
    QPointF clickPoint = (e->pos() - QPointF(scrollX, scrollY)) / scale;
    qInfo() << "Click point" << clickPoint;

    for (int i = 0; i < shapes.size(); i++) {
        QRectF bbox = shapes[i].path.boundingRect();
        QRectF newBBox(bbox.x() + shapes[i].x - 10, bbox.y() + shapes[i].y - 10, bbox.width() + 20, bbox.height() + 20);

        if (newBBox.contains(clickPoint)) {
            boundingShapes.push_back(&shapes[i]);
        }
    }

    qInfo() << "Bounding shapes" << boundingShapes.size();

    for (int i = 0; i < boundingShapes.size(); i++) {
        if (boundingShapes[i]->testHit(clickPoint)) {
            boundingShapes[i]->selected = !boundingShapes[i]->selected;
        }
    }
}

void VCanvas::mouseMoveEvent(QMouseEvent *e) {
    // If we've moved more then 25 pixels, assume user is dragging
    if (!m_mouseDrag && QPoint(m_mousePress - e->pos()).manhattanLength() > 25)
        m_mouseDrag = true;

    qInfo() << "Mouse Move" << e->pos();
}

void VCanvas::mouseReleaseEvent(QMouseEvent *e) {
    qInfo() << "Mouse Release" << e->pos();
}

void VCanvas::wheelEvent(QWheelEvent *e) {
    scrollX += e->pixelDelta().x() / 2.5;
    scrollY += e->pixelDelta().y() / 2.5;
    qInfo() << "Wheel Event" << e->pixelDelta();
}

void VCanvas::loop() {
    counter++;
    // scrollX -= 0.1;
    // scale += 0.01;
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
