#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <cstdint>

#define FONT_SIZE 200
#define FONT_TYPE "Tahoma"
#define LINE_HEIGHT 1.2

enum JobOrigin{
  NW = 0,
  N,
  NE,
  E,
  SE,
  S,
  SW,
  W,
  CENTER,
  TotalJobOrigin
};
enum StartFrom{
  AbsoluteCoords = 0,
  UserOrigin,
  CurrentPosition,
  TotalStartFrom
};
enum CanvasQuality{
  AutoQuality = 0,
  NormalQuality,
  LowQuality
};
enum PathSort{
  MergeSort = 0,
  NestedSort,
  NoSort
};

namespace PromarkJobConfig {
// Expansion axes. The start speed / acceleration time are the arguments handed to
// lcs_set_axis_move(); they are named here so the motion command and the timing model that
// predicts its duration (PromarkTiming::axisMoveTimeMs) cannot drift apart.
inline constexpr double Z_PULSE_PER_MM = 1600;
inline constexpr double Z_PULSE_PER_SEC = 4800;
inline constexpr double Z_START_PULSE_PER_SEC = 0;
inline constexpr double A_PULSE_PER_MM = 63;
inline constexpr double A_PULSE_PER_SEC = 3200;
inline constexpr double A_START_PULSE_PER_SEC = 1600;
inline constexpr uint16_t AXIS_ACC_TIME_MS = 255; // lcs_set_axis_move() accTime, range 0~255

// Fixed cost of issuing one lcs_set_axis_move(), in ms -- everything the controller spends on an
// axis move besides travelling the pulses: accepting the list instruction, driving the stepper
// through its start/stop sequence and settling before the galvo is allowed to mark again.
//
// Measured from a Promark UV STL inner-carving job (2025-09-29 log, 8857 Z moves, nearly all
// 0.001mm = 1.6 pulses, 20 lists of 10000 list instructions each): the ramp model alone predicts
// 249s for a job whose lists actually took 695s, and the whole gap is proportional to the number
// of axis moves -- 50.4ms each, constant within +-3% per list while the Z moves per list vary
// from 296 to 1025. Adding it back predicts 691s vs 695s measured.
//
// The job only contains moves at the pulse quantisation floor, so this calibration cannot yet
// separate a fixed per-command cost from an error in the ramp model itself; a sweep over step
// sizes is needed before trusting it for long axis moves. It is applied to the A axis as well,
// where it has not been measured.
inline constexpr double AXIS_MOVE_OVERHEAD_MS = 50;

// Galvo. All delays are in us, as the lcs API takes them.
inline constexpr double JUMP_SPEED = 4000; // mm/s
inline constexpr int JUMP_DELAY_MIN = 200;
inline constexpr int JUMP_DELAY_MAX = 400;
inline constexpr double JUMP_LENGTH_LIMIT_MM = 10; // lcs_set_delay_mode() JumpLengthLimit
inline constexpr int LASER_ON_DELAY = -100;
inline constexpr int LASER_OFF_DELAY = 100;
inline constexpr int MARKING_DELAY = 100;
inline constexpr int CORNER_DELAY = 50;
}

#endif // CONSTANTS_H
