#ifndef VCANVAS_H
#define VCANVAS_H
#include <svgpp/detail/attribute_id.hpp>
#include <QtQuick>
#include <rapidxml_ns/rapidxml_ns.hpp>
#include <svgpp/policy/xml/rapidxml_ns.hpp>
#include "vcontext.h"

class VCanvas : public QQuickPaintedItem {
        Q_OBJECT
        Q_PROPERTY(bool rightAligned READ isRightAligned WRITE setRightAligned NOTIFY rightAlignedChanged)
        QML_ELEMENT

    public:
        VCanvas(QQuickItem *parent = 0);
        void paint(QPainter *painter);
        void loop();

        bool isRightAligned();
        void setRightAligned(bool rightAligned);
        void loadSvg(QByteArray data);

    private:
        bool rightAligned;
        bool ready;
        int counter;
        char *m_data;
        rapidxml_ns::xml_document<> m_xml_doc;
        VContext m_context;
        rapidxml_ns::xml_node<> const *m_xml_root_element;

    signals:
        void rightAlignedChanged();
};

#endif // VCANVAS_H
