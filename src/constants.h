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

#endif // CONSTANTS_H
