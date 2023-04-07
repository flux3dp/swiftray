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
#endif // CONSTANTS_H
