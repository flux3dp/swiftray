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
        bool gradient;
        int threshold;
        bool pwm;
    };

    struct BeamLayerConfig {
        int order_index;
        bool visible;
        float speed;
        float power;
        QColor color;
        QString title;
        int repeat;
        float height;
        float z_step;
        int diode;
        float backlash;
        int uv;
        int module;
        float focus;
        float focus_step;
        float printing_strength;
        double printing_speed;
        int uv_;
        int halftone;
        int multipass;
        int ink;
        double min_power;
    };

    enum ReadType {
        InSingleLayer,
        ByLayers,
        ByColors,
        BVG
    };
}