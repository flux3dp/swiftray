#pragma once

#include "toolpath-exporter-types.h"
#include <QMap>
#include <QPointF>
#include <QString>

constexpr float CANVAS_MM_RATIO = 10.0;

constexpr int WHITE_PIXEL = 255;  // should ignored
constexpr int BLACK_PIXEL = 0;    // should emit

constexpr int CLIP_FLAG_START = 0b01;
constexpr int CLIP_FLAG_END = 0b10;

const QMap<HardwareType, HardwareProfile> HW_PROFILE = {
  {HardwareType::beamo, {
    300, 210, 1500
  }},
  {HardwareType::Beambox, {
    400, 375, 1500
  }},
  {HardwareType::BeamboxPro, {
    600, 375, 1500
  }},
  {HardwareType::HEXA, {
    740, 410, 1875
  }},
  {HardwareType::Ador, {
    430, 320, 2150, 2, 7.5, 20, QPointF(215, 150)
  }},
  {HardwareType::BB2, {
    600, 375, 2150, 2, 5.16
  }},
  {HardwareType::BM2, {
    360, 240, 0, 2, 7.5, 20, {}, QPointF(0, 30)
  }},
  {HardwareType::RF, {
    740, 410, 7300, 2, 7.5, 39
  }},
  {HardwareType::UV, {
    300, 215, 0, 2, 7.5, 20, {}, {}, true
  }}
};

const QMap<HardwareType, AccelerationData> PATH_ACCELERATION_DATA = {
  {HardwareType::Ador, {
    true, 500, 500
  }},
  {HardwareType::BB2, {
    true, 1000, 1000
  }}
};

const QMap<CollisionRegions, RegionData> REGIONS = {
  {CollisionRegions::FBM2_SLIDING_TABLE, {
    RegionType::PRINTER,
    {{0, 180}, {130, 180}, {130, 240}, {0, 240}}
  }}
};

const QMap<HardwareType, SupportInfo> SUPPORT_INFO = {
  {HardwareType::HEXA, {
    true
  }},
  {HardwareType::Ador, {
    true, true, true, true
  }},
  {HardwareType::BB2, {
    true
  }},
  {HardwareType::BM2, {
    true, true, true, false, true, true
  }},
  {HardwareType::RF, {
    false, false, false, false, false, false, true
  }},
  {HardwareType::UV, {
    false, true, false, false, true, true
  }}
};

// Pre-move z axis in curve engraving to avoid motor out of step
const QMap<HardwareType, ZPremoveData> Z_PREMOVE_DATA = {
  {HardwareType::BB2, {
    true, 0.0127, 0.0064, 0.0005, 140
  }}
};

const QMap<PrintingColor, QString> COLOR_NAME_MAP = {
  {PrintingColor::CYAN, "cyan"},
  {PrintingColor::MAGENTA, "magenta"},
  {PrintingColor::YELLOW, "yellow"},
  {PrintingColor::BLACK, "black"},
  {PrintingColor::WHITE, "white"}
};

const QMap<PrintingColor, double> AM_ANGLE_MAP = {
  {PrintingColor::CYAN, 22.5},
  {PrintingColor::MAGENTA, 22.5},
  {PrintingColor::YELLOW, 22.5},
  {PrintingColor::BLACK, 52.5}
};

const QMap<PrintingColor, double> AM_ANGLE_MAP_4C = {
  {PrintingColor::CYAN, 75},
  {PrintingColor::MAGENTA, 45},
  {PrintingColor::YELLOW, 90},
  {PrintingColor::BLACK, 15}
};

const QVector<QMap<PrintingColor, QVector<int>>> COLOR_CURVES_MAP = {
  // FM
  {
    {PrintingColor::CYAN, {0, 15, 47, 95, 255}},
    {PrintingColor::MAGENTA, {0, 15, 47, 191, 255}},
    {PrintingColor::YELLOW, {0, 12, 37, 143, 255}},
    {PrintingColor::BLACK, {0, 15, 47, 79, 255}},
  },
  // AM
  {
    {PrintingColor::CYAN, {0, 47, 107, 159, 255}},
    {PrintingColor::MAGENTA, {0, 43, 115, 169, 255}},
    {PrintingColor::YELLOW, {0, 59, 115, 175, 255}},
    {PrintingColor::BLACK, {0, 47, 111, 143, 255}},
  },
};

const QVector<QMap<PrintingColor, QVector<int>>> COLOR_CURVES_MAP_4C = {
  // FM
  {
    {PrintingColor::CYAN, {0, 88, 170, 229, 255}},
    {PrintingColor::MAGENTA, {0, 90, 149, 204, 241}},
    {PrintingColor::YELLOW, {0, 96, 147, 186, 249}},
    {PrintingColor::BLACK, {0, 60, 109, 163, 207}},
  },
  // AM
  {
    {PrintingColor::CYAN, {0, 88, 170, 229, 255}},
    {PrintingColor::MAGENTA, {0, 90, 149, 204, 241}},
    {PrintingColor::YELLOW, {0, 96, 147, 186, 249}},
    {PrintingColor::BLACK, {0, 60, 109, 163, 207}},
  },
};

const QMap<HardwareType, double> HARDWARE_MIN_PADDING = {
  {HardwareType::BM2, 10},
};
const QMap<LayerModule, double> MIN_PADDING = {
  {LayerModule::UNIVERSAL_LASER, 0},
  {LayerModule::LASER_10W, 15},
  {LayerModule::LASER_20W, 25},
  {LayerModule::PRINTER, 10},
  {LayerModule::PRINTER_4C, 10},
  {LayerModule::WHITE_INK, 10},
  {LayerModule::VARNISH, 10},
  {LayerModule::LASER_1064, 25},
};
const QMap<MachineModules, int> MACHINE_MODULE_MIN_PADDING = {
  {MachineModules::PRINTER_4C, 20},
  {MachineModules::PRINTER_4C_WITH_1064, 25},
  {MachineModules::LASER_1064, 25},
};
