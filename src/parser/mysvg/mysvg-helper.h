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
    while (current_node->type() != QSVG_DOC) {
        layer_name = current_node->nodeId();
        current_node = current_node->parent();
    }
    return layer_name;
}

QString getBVGLayerName(QSvgNode *current_node, QMap<QString, MySVG::BeamLayerConfig> &layer_config_map_) {
    while (current_node->parent() != NULL) {
        current_node = current_node->parent();
        QString node_addr = QString::number(reinterpret_cast<quintptr>(current_node), 16);
        if (layer_config_map_.contains(node_addr)) {
            auto title = layer_config_map_[node_addr].title;
            return title;
        }
    }
    return current_node->nodeId();
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