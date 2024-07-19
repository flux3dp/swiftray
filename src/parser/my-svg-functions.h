#include "my_qsvg_handler.h"
/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt SVG module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <qplatformdefs.h>
#include <private/qsvgfont_p.h>
#include <private/qsvggraphics_p.h>
#include <private/qsvgnode_p.h>
#include <private/qsvgstructure_p.h>
#include <private/qsvgtinydocument_p.h>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPainterPath>
#include <QPen>
#include <QRegularExpression>
#include <QTextFormat>
#include <QTransform>
#include <QVarLengthArray>
#include <QVector>
#include <QtGlobal>
#include <QtMath>
#include <private/qmath_p.h>
#include "shape/bitmap-shape.h"
#include "my-svg-types.h"
#include "float.h"

#ifdef QT6
    #define QSVG_PATH QSvgNode::Type::Path
    #define QSVG_USE QSvgNode::Type::Use
    #define QSVG_IMAGE QSvgNode::Type::Image
    #define QSVG_TEXT QSvgNode::Type::Text
    #define QSVG_TSPAN QSvgNode::Type::Tspan
#else
    #define QSVG_PATH QSvgNode::PATH
    #define QSVG_USE QSvgNode::USE
    #define QSVG_IMAGE QSvgNode::IMAGE
    #define QSVG_TEXT QSvgNode::TEXT
    #define QSVG_TSPAN QSvgNode::TSPAN
#endif


namespace MySVG {
    void transformUse(QList<Node> &nodes, QString node_name, QTransform transform) {
        for (int i = 0; i < nodes.size(); ++i) {
            for (auto name : nodes[i].node_names) {
                if (name == node_name) {
                    nodes[i].trans *= transform;
                    break;
                }
            }
        }
    }

    LayerPtr findLayer(ReadType read_type_, QList<LayerPtr> &layers, QString layer_name, QColor color) {
        LayerPtr target_layer;
        switch (read_type_) {
        case InSingleLayer:
            if (layers.empty()) {
                layers.push_back(std::make_shared<Layer>());
            }
            target_layer = layers[0];
            break;
        case ByLayers:
            if (layers.empty()) {
                layers.push_back(std::make_shared<Layer>());
                layers[0]->setName(layer_name);
                target_layer = layers[0];
                target_layer->setColor(color);
            } else {
                for (int i = 0; i < layers.size(); ++i) {
                    target_layer = layers[i];
                    if (target_layer->name() == layer_name) {
                        break;
                    } else if (i == layers.size() - 1) {
                        target_layer = std::make_shared<Layer>();
                        target_layer->setName(layer_name);
                        target_layer->setColor(color);
                        layers.push_back(target_layer);
                    }
                }
            }
            break;
        case ByColors:
            layer_name = color.name();
            if (layers.empty()) {
                layers.push_back(std::make_shared<Layer>());
                layers[0]->setName(layer_name);
                target_layer = layers[0];
                target_layer->setColor(color);
            } else {
                for (int i = 0; i < layers.size(); ++i) {
                    target_layer = layers[i];
                    if (target_layer->name() == layer_name) {
                        break;
                    } else if (i == layers.size() - 1) {
                        target_layer = std::make_shared<Layer>();
                        target_layer->setName(layer_name);
                        target_layer->setColor(color);
                        layers.push_back(target_layer);
                    }
                }
            }
            break;
        }
        return target_layer;
    }

    void processMySVGNode(QSvgNode *node, QList<Node> &nodes, double g_scale, QColor &g_color, QImage &g_image) {
        QTransform trans = getNodeTransform(node);
        double scale = 1;
        if(node->type() == QSVG_PATH) {
            QSvgPath *path_node = (QSvgPath*) node;
            QTransform tmp_scale = QTransform().scale(g_scale * scale, g_scale * scale);

            Node n;
            n.type = QSVG_PATH;
            #ifdef QT6
            n.qpath = path_node->path();
            #else
            n.qpath = *path_node->qpath();
            #endif
            QSvgNode* tmp_node = node;
            while(tmp_node != nullptr) {
                n.node_names.push_back(tmp_node->nodeId());
                tmp_node = tmp_node->parent();
            }
            n.layer_name = getNodeLayerName(node);
            n.trans = trans * tmp_scale;
            n.color = g_color;
            n.visible = getNodeVisible(node);
            nodes.push_back(n);
        } else if(node->type() == QSVG_IMAGE) {
            QTransform tmp_scale = QTransform().scale(g_scale * scale, g_scale * scale);

            Node n;
            n.type = QSVG_IMAGE;
            n.image = g_image;
            QSvgNode* tmp_node = node;
            while(tmp_node != nullptr) {
                n.node_names.push_back(tmp_node->nodeId());
                tmp_node = tmp_node->parent();
            }
            n.layer_name = getNodeLayerName(node);
            n.trans = trans * tmp_scale;
            n.color = g_color;
            n.visible = getNodeVisible(node);
            nodes.push_back(n);
        } else if(node->type() == QSVG_USE) {
            QSvgUse* use_node = (QSvgUse*)node;
            Node n;
            n.type = QSVG_USE;
            n.node_names.push_back(use_node->linkId());
            n.trans = trans;
            nodes.push_back(n);
        } else if(node->type() == QSVG_TEXT) {
            QSvgText *text_node = (QSvgText*) node;
            QTransform tmp_scale = QTransform();
            #ifdef QT6
            tmp_scale = tmp_scale.translate(text_node->position().x(), text_node->position().y());
            #else
            tmp_scale = tmp_scale.translate(text_node->getCoord().x(), text_node->getCoord().y());
            #endif
            tmp_scale = tmp_scale.scale(g_scale * scale, g_scale * scale);
            QFont font;
            if(node->styleProperty(QSvgStyleProperty::FONT) != 0) {
                font = ((QSvgFontStyle*)node->styleProperty(QSvgStyleProperty::FONT))->qfont();
            } else {
                font = QFont();
                font.setPointSize(10);
            }
            Node n;
            n.type = QSVG_TEXT;
            n.trans = trans * tmp_scale;
            n.color = g_color;
            QSvgNode* tmp_node = node;
            while(tmp_node != nullptr) {
                n.node_names.push_back(tmp_node->nodeId());
                tmp_node = tmp_node->parent();
            }
            n.visible = getNodeVisible(node);
            n.font = font;
            n.text = QString();
            nodes.push_back(n);
        } else if(node->type() == QSVG_TSPAN) {
            // QSvgTspan *tmp_node = (QSvgTspan*) node;
        }
    }
}