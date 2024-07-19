#pragma once

#include <private/qsvgnode_p.h>
#include <QString>
#include <QTransform>

#ifdef QT6
#define QSVG_DOC QSvgNode::Type::Doc
#else
#define QSVG_DOC QSvgNode::DOC
#endif


QString getNodeLayerName(QSvgNode *current_node) {
    QString layer_name;
    while (1) {
        if (current_node->type() == QSVG_DOC)
            break;
        layer_name = current_node->nodeId();
        current_node = current_node->parent();
    }
    return layer_name;
}

QTransform getNodeTransform(QSvgNode *current_node) {
    QTransform trans = QTransform();
    while (1) {
        if (current_node->type() == QSVG_DOC)
            break;
        trans *= ((QSvgTransformStyle *)current_node->styleProperty(QSvgStyleProperty::TRANSFORM))->qtransform();
        current_node = current_node->parent();
    }
    return trans;
}

bool getNodeVisible(QSvgNode *current_node) {
    while (current_node != nullptr) {
        if (current_node->displayMode() == QSvgNode::NoneMode)
            return false;
        current_node = current_node->parent();
    }
    return true;
}