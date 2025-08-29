#include "bsl_list_manager.h"
#include <QDebug>
#include "bsl_motion_controller.h"

QString getApiName(ListApiType type) {
  switch (type) {
    case ListApiType::DisableLaser: return "lcs_disable_laser";
    case ListApiType::SetPulses: return "lcs_set_laser_pulses";
    case ListApiType::SetSpeed: return "lcs_set_mark_speed";
    case ListApiType::SetPower: return "lcs_set_laser_power";
    case ListApiType::SetWobble: return "lcs_set_wobble_mode";
    case ListApiType::SetIo: return "lcs_write_io_port_mask_list";
    case ListApiType::Jump: return "lcs_jump_abs";
    case ListApiType::Mark: return "lcs_mark_abs";
    case ListApiType::LaserPulse: return "lcs_laser_on_list";
    case ListApiType::MoveAxis: return "lcs_set_axis_move";
    case ListApiType::EndOfList: return "lcs_set_end_of_list";
    default: return "unknown_api";
  }
}

QString formatArgs(const Params& args, ListApiType type) {
  QString result;
  switch (type) {
    case ListApiType::DisableLaser:
    case ListApiType::LaserPulse: {
      auto& [time] = std::get<std::tuple<uint32_t>>(args);
      result = QString("time=%1").arg(time);
      break;
    }
    case ListApiType::SetPulses: {
      auto& [period, pulseLength, pulseWidth] = std::get<std::tuple<double, double, uint16_t>>(args);
      result = QString("period=%1, pulseLength=%2, pulseWidth=%3").arg(period).arg(pulseLength).arg(pulseWidth);
      break;
    }
    case ListApiType::SetSpeed: {
      auto& [speed] = std::get<std::tuple<double>>(args);
      result = QString("speed=%1").arg(speed);
      break;
    }
    case ListApiType::SetPower: {
      auto& [power] = std::get<std::tuple<unsigned char>>(args);
      result = QString("power=%1").arg(power);
      break;
    }
    case ListApiType::SetWobble: {
      auto& [diameter_x, diameter_y, step, type] = std::get<std::tuple<double, double, double, WobbleType>>(args);
      result = QString("diameter_x=%1, diameter_y=%2, step=%3, type=%4").arg(diameter_x).arg(diameter_y).arg(step).arg(static_cast<int>(type));
      break;
    }
    case ListApiType::SetIo: {
      auto& [value, mask] = std::get<std::tuple<uint32_t, uint32_t>>(args);
      result = QString("value=0x%1, mask=0x%2").arg(value, 0, 16).arg(mask, 0, 16);
      break;
    }
    case ListApiType::Jump:
    case ListApiType::Mark: {
      auto& [x, y] = std::get<std::tuple<double, double>>(args);
      result = QString("x=%1, y=%2").arg(x).arg(y);
      break;
    }
    case ListApiType::MoveAxis: {
      auto& [axis, steps, direction, speed, accel, current] = std::get<std::tuple<int, double, bool, double, double, uint16_t>>(args);
      result = QString("axis=%1, steps=%2, direction=%3, speed=%4, accel=%5, current=%6").arg(axis).arg(steps).arg(direction ? "forward" : "backward").arg(speed).arg(accel).arg(current);
      break;
    }
    case ListApiType::EndOfList:
      result = "none";
      break;
    default:
      result = "unknown";
  }
  return result;
}

void BSLListManager::resetBackup(int list_no, BSLMotionController* controller) {
  qInfo() << "Resetting backup API calls" << list_no;
  if (list_no > 0 && backup_list_no_ == 0) {
    api_calls_.reserve(10100); // MAX_BUFFER_LIST_SIZE + additional 100
  }
  if (controller) controller_ = controller;
  backup_list_no_ = list_no;
  start_x_ = x_;
  start_y_ = y_;
  api_calls_.clear();
}

void BSLListManager::redoBackup() {
  if (!controller_->is_running_laser_ || backup_list_no_ <= 0 || api_calls_.empty()) return;

  controller_->setUpTaskCtrl();
  lcs_set_start_list(backup_list_no_);
  controller_->setUpTaskList();
  lcs_jump_abs(start_x_, start_y_);
  qInfo() << "Redoing backup API calls" << api_calls_.size() << "calls to redo";
  for (const auto& call : api_calls_) {
    controller_->current_error_ = doApiCall(call);
  }
  qInfo() << "Finished redoing backup API calls";
}

void BSLListManager::call(ListApiType type, Params args) {
  if (!controller_->is_running_laser_) return;
  ListApiCall call(type, args);
  api_calls_.push_back(call);
  controller_->current_error_ = doApiCall(call);
}

void BSLListManager::call(ListApiType type) {
  call(type, std::monostate{});
}
void BSLListManager::call(ListApiType type, uint32_t time) {
  call(type, std::make_tuple(time));
}
void BSLListManager::call(ListApiType type, double period, double pulseLength, uint16_t pulseWidth) {
  call(type, std::make_tuple(period, pulseLength, pulseWidth));
}
void BSLListManager::call(ListApiType type, double speed) {
  call(type, std::make_tuple(speed));
}
void BSLListManager::call(ListApiType type, unsigned char power) {
  call(type, std::make_tuple(power));
}
void BSLListManager::call(ListApiType type, double diameter_x, double diameter_y, double step, WobbleType wobbleType) {
  call(type, std::make_tuple(diameter_x, diameter_y, step, wobbleType));
}
void BSLListManager::call(ListApiType type, uint32_t value, uint32_t mask) {
  call(type, std::make_tuple(value, mask));
}
void BSLListManager::call(ListApiType type, double x, double y) {
  call(type, std::make_tuple(x, y));
}
void BSLListManager::call(ListApiType type, int axis, double steps, bool direction, double speed, double accel, uint16_t current) {
  call(type, std::make_tuple(axis, steps, direction, speed, accel, current));
}

int BSLListManager::doApiCall(ListApiCall call) {
  int error = LCS_RES_NO_ERROR;
  switch (call.type) {
    case ListApiType::DisableLaser: {
      auto& [time] = std::get<std::tuple<uint32_t>>(call.args);
      error = lcs_disable_laser(time);
      break;
    }
    case ListApiType::SetPulses: {
      auto& [period, pulseLength, pulseWidth] = std::get<std::tuple<double, double, uint16_t>>(call.args);
      error = lcs_set_laser_pulses(period, pulseLength, pulseWidth);
      break;
    }
    case ListApiType::SetSpeed: {
      auto& [speed] = std::get<std::tuple<double>>(call.args);
      error = lcs_set_mark_speed(speed);
      break;
    }
    case ListApiType::SetPower: {
      auto& [power] = std::get<std::tuple<unsigned char>>(call.args);
      error = lcs_set_laser_power(power);
      break;
    }
    case ListApiType::SetWobble: {
      auto& [diameter_x, diameter_y, step, type] = std::get<std::tuple<double, double, double, WobbleType>>(call.args);
      error = lcs_set_wobble_mode(diameter_x, diameter_y, step, type);
      break;
    }
    case ListApiType::SetIo: {
      auto& [value, mask] = std::get<std::tuple<uint32_t, uint32_t>>(call.args);
      error = lcs_write_io_port_mask_list(value, mask);
      break;
    }
    case ListApiType::Jump: {
      std::tie(x_, y_) = std::get<std::tuple<double, double>>(call.args);
      error = lcs_jump_abs(x_, y_);
      break;
    }
    case ListApiType::Mark: {
      std::tie(x_, y_) = std::get<std::tuple<double, double>>(call.args);
      error = lcs_mark_abs(x_, y_);
      break;
    }
    case ListApiType::LaserPulse: {
      auto& [duration] = std::get<std::tuple<uint32_t>>(call.args);
      error = lcs_laser_on_list(duration);
      break;
    }
    case ListApiType::MoveAxis: {
      auto& [axis, steps, direction, speed, accel, current] = std::get<std::tuple<int, double, bool, double, double, uint16_t>>(call.args);
      error = lcs_set_axis_move(axis, steps, direction, speed, accel, current);
      break;
    }
    case ListApiType::EndOfList:
      error = lcs_set_end_of_list();
      break;
  }

  if (error != LCS_RES_NO_ERROR) {
    if (controller_->is_running_laser_) {
      QString apiName = getApiName(call.type);
      QString argStr = formatArgs(call.args, call.type);
      qInfo() << "[LCS API Result]" << apiName << "(" << argStr << ") failed with error:" << controller_->getErrorString(error);
    } else {
      error = LCS_RES_NO_ERROR;
    }
  }
  return error;
}
