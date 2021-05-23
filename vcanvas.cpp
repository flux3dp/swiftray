#include "vcanvas.h"
#include "parser/svgpp_impl.hpp"
#include <QDebug>
#include <QPainter>
#include <QWidget>
#include <cstring>
#include <iostream>

void VCanvas::loadSvg(QByteArray &data) {
    bool success = svgpp_parse(data, m_context);

    if (m_context.getPathCount() > 5000) {
        setAntialiasing(false);
    }

    if (success) {
        ready = true;
        update();
    }
}


VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent) {
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
    if (m_context.painter() == nullptr) {
        qInfo() << "Rendering engine = " << painter->paintEngine()->type();
    }

    m_context.setPainter(painter);
    painter->translate(scrollX, scrollY);
    painter->scale(scale, scale);
    QPen pen = QPen(Qt::blue, 0, Qt::DashLine);
    pen.setDashPattern(QVector<qreal>(10, 3));
    pen.setDashOffset(counter);
    painter->setPen(pen);
    m_context.render();
}

void VCanvas::mousePressEvent(QMouseEvent *e) {
    // If we're not running in small screen mode, always assume we're dragging
    m_mouseDrag = true;
    m_mousePress = e->pos();
    qInfo() << "Mouse Press" << e->pos();
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
    scrollX += e->pixelDelta().x() / 5;
    scrollY += e->pixelDelta().y() / 5;
    qInfo() << "Wheel Event" << e->pixelDelta();
}

void VCanvas::loop() {
    counter++;
    // scrollX -= 0.1;
    // scale += 0.01;
    update();
}


bool VCanvas::event(QEvent *e) {
    qInfo() << "QEvent" << e;
    QNativeGestureEvent *nge;

    switch (e->type()) {
    case QEvent::NativeGesture:
        qInfo() << "Native Gesture!";
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
