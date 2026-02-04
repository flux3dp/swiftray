#pragma once

#include <QJsonObject>
#include <QPointF>
#include <QPolygonF>
#include <array>
#include <bitset>
#include <opencv2/core.hpp>

using Bitset32 = std::bitset<32>;
using ByteArray32 = std::array<unsigned char, 32>;

enum class HardwareType {
  beamo,
  Beambox,
  BeamboxPro,
  HEXA,
  Ador,
  BB2,
  BM2,
  RF,
  UV
};

enum class NozzleMode {
  UNDEFINED = 0,
  LEFT = 1,
  RIGHT = 2,
  BOTH = 3,
};

// Module types of layer, defining what module each layer should be used.
enum class LayerModule {
  NONE = 0,
  LASER_10W = 1,
  LASER_20W = 2,
  LASER_1064 = 4,
  PRINTER = 5,
  PRINTER_4C = 7,
  WHITE_INK = 8,
  VARNISH = 9,
  UNIVERSAL_LASER = 15
};

// Machine module types, defining the module installed on the machine.
// Could be combination of multiple LayerModule.
enum class MachineModules {
  NONE = 0,
  LASER_10W_DIODE = 1,
  LASER_20W_DIODE = 2,
  LASER_1064 = 4,
  PRINTER_4C = 7,
  PRINTER_4C_WITH_1064 = 8,
  UNKNOWN = 9,
  PRINTER_4C_WITH_UV = 10,
  PRINTER_4C_WITH_UV_1064 = 11
};

enum class PrintingColor {
  CYAN = 'c',
  MAGENTA = 'm',
  YELLOW = 'y',
  BLACK = 'k',
  WHITE = 'w'
};

enum class CollisionRegions {
  FBM2_SLIDING_TABLE = 1,
};

enum class RegionType {
  ALL = 0,
  LASER = 1,
  PRINTER = 2,
};

struct NestedPolygonF {
  QPolygonF polygon;
  QVector<NestedPolygonF> children;
};

struct NozzleSettings {
  int saturation = 3;
  float voltage = 9.0;
  float pulse_width = 2.0;
  int DPI = 600;
  int ink_catridge_count = 1;
  int ink_type = 0;
  int nozzle_select = 0;
  int spray_time = 0;
  int ink_exchange = 0;
  int h_gap_ink1_ink2 = 0;
  int v_gap_ink1_ink2 = 0;
  int h_gap_ink2_ink3 = 0;
  int v_gap_ink2_ink3 = 0;
  int h_gap_ink3_ink4 = 0;
  int v_gap_ink3_ink4 = 0;
};

// enum key of HWFeature in fluxclient/hw_profile/support_info.py
struct SupportInfo {
  bool REL_Z_MOVE = false;
  bool MODULES = false;
  bool ROTARY_Z_MOTION = false;
  bool MODULE_TRANSITION = false;
  bool PRINTING_SCRIPTS = false;  // scripts xMIN 0005, xMIN 0006 for printing modules
  bool MODULE_CHECK_METADATA = false;
  bool LASER_DELAY = false;
};

struct HardwareProfile {
  int fcode_version = 1;
  double width = 0.0;
  double length = 0.0;
  // for time estimation
  double z_speed = 7.5;
  // max data width(px) for fast gradient in pwm mode
  int fg_pwm_limit = 0;
  // limit for high res engraving and fast gradient
  int max_pixel_per_mm_x = 20;
  // position when changing module without job origin
  QPointF tran_pos;  // Note: (0,0) = not set
  QPointF home_position;
  bool reverse_4c = false;
};

struct AccelerationData {
  bool is_valid = false;
  double x = NAN;
  double y = NAN;
  double z = NAN;
  double a = NAN;

  void updateFromJson(const QJsonObject& obj) {
    if (obj.isEmpty())
      return;
    is_valid = true;
    x = obj["x"].toDouble(NAN);
    y = obj["y"].toDouble(NAN);
    z = obj["z"].toDouble(NAN);
    a = obj["a"].toDouble(NAN);
  }
};

struct RegionData {
  RegionType type = RegionType::ALL;
  std::vector<cv::Point2f> points;
};

struct ZPremoveData {
  bool is_valid = false;
  double x = 0.0; // mm of 1 step
  double y = 0.0;
  double z = 0.0;
  double speed = 0.0; // mm/min
};

struct InwardRect {
  double top = 0.0;
  double right = 0.0;
  double bottom = 0.0;
  double left = 0.0;
};
