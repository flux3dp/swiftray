#include "vcanvas.h"
#include "parser/svgpp_impl.hpp"
#include <QDebug>
#include <QPainter>
#include <cstring>
#include <iostream>

void VCanvas::loadSvg(QByteArray &data) {
    bool success = svgpp_parse(data, m_context);

    if (success) {
        ready = true;
        update();
    }
}


VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent), rightAligned(false) {
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(true);
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VCanvas::loop);
    timer->start(30);
    qInfo() << "Rendering target = " << this->renderTarget();
}

void VCanvas::loop() {
    counter++;
    update();
}

void VCanvas::paint(QPainter *painter) {
    if (m_context.painter() == nullptr) {
        qInfo() << "Rendering engine = " << painter->paintEngine()->type();
    }

    m_context.setPainter(painter);
    painter->setRenderHint(QPainter::Antialiasing, true);
    QPen pen = QPen(Qt::blue, 1, Qt::DashLine);
    pen.setDashPattern(QVector<qreal>(10, 3));
    pen.setDashOffset(counter);
    painter->setPen(pen);
    m_context.render();
}

bool VCanvas::isRightAligned() {
    return this->rightAligned;
}

void VCanvas::setRightAligned(bool rightAligned) {
    this->rightAligned = rightAligned;
}
