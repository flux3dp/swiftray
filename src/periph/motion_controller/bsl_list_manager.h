#pragma once

#include <QString>
#include <functional>
#include <tuple>
#include <variant>
#include <vector>
#include "constants.h"
#include "liblcs/lcsApi.h"
#include "liblcs/lcsExpr.h"

enum class ListApiType {
  DisableLaser,
  SetPulses,
  SetSpeed,
  SetPower,
  SetWobble,
  SetIo,
  Jump,
  Mark,
  LaserPulse,
  MoveAxis,
  EndOfList
};

using Params = std::variant<
    std::monostate,                                  // end of list
    std::tuple<uint32_t>,                            // delay & laser pulse time
    std::tuple<double, double, uint16_t>,            // pulses
    std::tuple<double>,                              // speed
    std::tuple<unsigned char>,                       // power
    std::tuple<double, double, double, WobbleType>,  // wobble
    std::tuple<uint32_t, uint32_t>,                  // io
    std::tuple<double, double>,                      // jump & mark
    std::tuple<int, double, bool, double, double, uint16_t>  // axis
    >;

struct ListApiCall {
  ListApiType type;
  Params args;

  ListApiCall(ListApiType t, Params a) : type(t), args(a) {}
};

class BSLMotionController;

class BSLListManager {
 public:
  BSLListManager(BSLMotionController* controller): controller_(controller) {}
  void resetBackup(int list_no = -1, BSLMotionController* controller = nullptr);
  void redoBackup();
  int bufferSize() { return api_calls_.size(); }
  void call(ListApiType type, Params args);

  void call(ListApiType type);
  void call(ListApiType type, uint32_t time);
  void call(ListApiType type, double period, double pulseLength, uint16_t pulseWidth);
  void call(ListApiType type, double speed);
  void call(ListApiType type, unsigned char power);
  void call(ListApiType type, double diameter_x, double diameter_y, double step, WobbleType wobbleType);
  void call(ListApiType type, uint32_t value, uint32_t mask);
  void call(ListApiType type, double x, double y);
  void call(ListApiType type, int axis, double steps, bool direction, double speed, double accel, uint16_t current);

 private:
  int doApiCall(ListApiCall call);

  int backup_list_no_ = 0;
  double x_ = 0;
  double y_ = 0;
  double start_x_ = 0;
  double start_y_ = 0;
  std::vector<ListApiCall> api_calls_;
  BSLMotionController* controller_ = nullptr;
};