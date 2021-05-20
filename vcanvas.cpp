#include "vcanvas.h"
#include <QDebug>
#include <QPainter>
#include <rapidxml_ns/rapidxml_ns.hpp>
#include <svgpp/policy/xml/rapidxml_ns.hpp>
#include <svgpp/svgpp.hpp>
#include <cstring>
#include <iostream>

using namespace svgpp;

typedef
boost::mpl::set <
// SVG Structural Elements
tag::element::svg,
    tag::element::g,
    // SVG Shape Elements
    tag::element::circle,
    tag::element::ellipse,
    tag::element::line,
    tag::element::path,
    tag::element::polygon,
    tag::element::polyline,
    tag::element::rect
    >::type processed_elements_t;

void VCanvas::loadSvg(QByteArray data) {
    try {
        if (m_data) free(m_data);

        m_data = (char *) malloc(data.length() + 1);
        strncpy(m_data, data.constData(), data.length() + 1);
        qInfo() << "File contents" << m_data;
        m_xml_doc.parse<0>(m_data);

        if (rapidxml_ns::xml_node<> *svg_element = m_xml_doc.first_node("svg")) {
            qInfo() << "SVG Element " << svg_element;
            m_xml_root_element = svg_element;
        }
    } catch (std::exception const &e) {
        qWarning() << "Error loading SVG: " << e.what();
    }

    if (m_xml_root_element) {
        ready = true;
        update();
    }
}


VCanvas::VCanvas(QQuickItem *parent): QQuickPaintedItem(parent), rightAligned(false) {
    this->setRenderTarget(QQuickPaintedItem::FramebufferObject);
    this->setAntialiasing(false);
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VCanvas::loop);
    timer->start(50);
}

void VCanvas::loop() {
    counter++;
    update();
}

void VCanvas::paint(QPainter *painter) {
    m_context.setPainter(painter);
    QPen pen = QPen(Qt::blue, 1, Qt::DashLine);
    pen.setDashOffset(counter / 2.0);
    painter->setPen(pen);
    qInfo() << "Paint Event";

    if (!ready) return;

    document_traversal <processed_elements<processed_elements_t>,
                       processed_attributes<traits::shapes_attributes_by_element>
                       >::load_document(m_xml_root_element, m_context);
}

bool VCanvas::isRightAligned() {
    return this->rightAligned;
}

void VCanvas::setRightAligned(bool rightAligned) {
    this->rightAligned = rightAligned;
}
