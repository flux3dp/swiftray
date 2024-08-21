#pragma once

#include <QList>
#include <QString>
#include <QTransform>
#include <QColor>
#include <QPainterPath>
#include <QImage>
#include <QFont>

namespace MySVG {
    struct Node {
        QList<QString> node_names;
        QString layer_name;
        int type;
        QTransform trans;
        QColor color;
        bool visible;
        QPainterPath qpath;
        QImage image;
        QFont font;
        QString text;
        bool fill;
        bool is_symbol;
    };

    struct BeamLayerConfig {
        float speed;
        float power;
        QColor color;
        QString title;
    };

    enum ReadType {
        InSingleLayer,
        ByLayers,
        ByColors,
        BVG
    };
}