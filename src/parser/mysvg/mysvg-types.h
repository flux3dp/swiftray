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
        int depthPass;
        double depthZStep;
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

        double min_power;
        int module;
        int uv;
        int ink;
        double printing_speed;
        int multipass;
        int halftone;
        float printing_strength;
        float focus;
        float focus_step;
        double ce_z_limit;
        int frequency;
        int pulse_width;
        double fill_interval;
        double fill_angle;
        bool fill_bidirectional;
        bool fill_hatch;
        int dotting_time;
        double wobble_step;
        double wobble_diameter;
    };

    enum ReadType {
        InSingleLayer,
        ByLayers,
        ByColors,
        BVG
    };
}