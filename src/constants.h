#ifndef CONSTANTS_H
#define CONSTANTS_H

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
inline constexpr double Z_PULSE_PER_MM = 1600;
inline constexpr double Z_PULSE_PER_SEC = 4800;
inline constexpr double Z_MS_PER_MM = Z_PULSE_PER_MM / Z_PULSE_PER_SEC * 1000;
inline constexpr double A_PULSE_PER_MM = 63;
inline constexpr double A_PULSE_PER_SEC = 3200;
inline constexpr double A_MS_PER_MM = A_PULSE_PER_MM / A_PULSE_PER_SEC * 1000;
inline constexpr double JUMP_SPEED = 4000; // mm/s
inline constexpr int JUMP_DELAY_MIN = 200;
inline constexpr int JUMP_DELAY_MAX = 400;
inline constexpr double JUMP_DELAY_MS = (double)(JUMP_DELAY_MIN + JUMP_DELAY_MAX) / 2000;
inline constexpr int LASER_ON_DELAY = 0;
inline constexpr int LASER_OFF_DELAY = 100;
inline constexpr double LASER_DELAY_MS = (double)(LASER_OFF_DELAY - LASER_ON_DELAY) / 1000;
}

#endif // CONSTANTS_H
