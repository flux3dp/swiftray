#include "svgpp_common.h"
#ifndef VCANVAS_H
#define VCANVAS_H
#include <QtQuick>
#include <libxml/parser.h>
#include <svgpp/policy/xml/libxml2.hpp>
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
        xmlDoc *m_xml_doc;
        VContext m_context;
        xmlNode *m_xml_root_element;

    signals:
        void rightAlignedChanged();
};

#endif // VCANVAS_H
