#include "vcanvas.h"
#include <QDebug>
#include <QPainter>
#include <libxml/parser.h>
#include <svgpp/policy/xml/libxml2.hpp>
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
        m_xml_doc = xmlParseDoc((xmlChar *)m_data);

        if (xmlNode *svg_element = xmlDocGetRootElement(m_xml_doc)) {
            qInfo() << "SVG Element " << svg_element;
            m_xml_root_element = svg_element;
            m_context.clear();
            document_traversal < processed_elements<processed_elements_t>,
                               processed_attributes<traits::shapes_attributes_by_element>,
                               transform_events_policy<policy::transform_events::forward_to_method<VContext>>,
                               svgpp::error_policy<svgpp::policy::error::default_policy<VContext>>
                               >::load_document(m_xml_root_element, m_context);
            qInfo() << "Loaded SVG " << m_context.getPathCount();
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
