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
        m_xml_doc.parse<0>(m_data);

        if (rapidxml_ns::xml_node<> *svg_element = m_xml_doc.first_node("svg")) {
            qInfo() << "SVG Element " << svg_element;
            m_xml_root_element = svg_element;
            m_context.clear();
            document_traversal < processed_elements<processed_elements_t>,
                               processed_attributes<traits::shapes_attributes_by_element>,
                               transform_events_policy<policy::transform_events::forward_to_method<VContext>>
                               >::load_document(m_xml_root_element, m_context);
            qInfo() << "Loaded SVG " << m_context.getPathCount();
            m_xml_doc.clear();
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
    //((QQuickWindow *)parent)->setGraphicsApi(QSGRendererInterface::Metal);
    setRenderTarget(QQuickPaintedItem::FramebufferObject);
    setAntialiasing(false);
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
    painter->setBackgroundMode(Qt::BGMode::OpaqueMode);
    painter->scale(0.5, 0.5);
    QPen pen = QPen(Qt::cyan, 1, Qt::DashLine);
    pen.setCosmetic(true);
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
