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
        bool is_one_way_engraving;
        int module;
        int ink;
        double printing_speed;
        int multipass;
        int halftone;
        double am_density;
        float printing_strength;
        double c_ratio;
        double m_ratio;
        double y_ratio;
        double k_ratio;
        float smooth;
        QString raw_am_angle_map;
        QString raw_color_curves_map;
        int refresh_interval;
        int refresh_threshold;
        int nozzle_mode;
        double nozzle_offset_x;
        double nozzle_offset_y;
        float focus;
        float focus_step;
        double ce_z_limit;
        int interpolation;
        double right_padding;
        int uv_printing_repeat;
        int uv_curing_after;
        int uv_curing_repeat;
        int uv_strength;
        int uv_x_step;
        int frequency;
        int pulse_width;
        double fill_interval;
        double fill_angle;
        bool fill_bidirectional;
        bool fill_hatch;
        int dotting_time;
        double wobble_step;
        double wobble_diameter;
        int air_assist;
        QString raw_bbox;
        int laser_delay;
        int dpmm;
        bool is_high_quality;
    };

    enum ReadType {
        InSingleLayer,
        ByLayers,
        ByColors,
        BVG
    };
}