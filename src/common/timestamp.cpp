#include "timestamp.h"

#include <string>
#include <iomanip>
#include <sstream>

Timestamp::Timestamp() {
  ms_ = 0;
}

Timestamp::Timestamp(int h, int m, int s, int ms) {
  ms_ = h*3600000 + m*60000 + s*1000 + ms;
}

void Timestamp::reset() {
  ms_ = 0;
}

int Timestamp::totalMSecs() const {
  return ms_;
}

int Timestamp::millisecond() const {
  return ms_ % 1000;
}

int Timestamp::second() const {
  return (ms_ % 60000) / 1000;
}

int Timestamp::minute() const {
  return (ms_ % 3600000) / 60000;
}

int Timestamp::hour() const {
  return (ms_ / 3600000);
}

int Timestamp::msecsTo(const Timestamp &t) const{
  return t.totalMSecs() - ms_;
}

int Timestamp::secsTo(const Timestamp &t) const {
  return (t.totalMSecs() - ms_) / 1000;
}

Timestamp& Timestamp::addMSecs(int ms) {
  ms_ += ms;
  return *this;
}

Timestamp& Timestamp::addSecs(int s) {
  ms_ += 1000*s;
  return *this;
}

QString Timestamp::toString() const {
  // To format hh:mm:ss (NOTE: hour column can be variable number of digit)
  std::stringstream ss;
  ss << hour() 
     << ':' << std::setw(2) << std::setfill('0') << minute() 
     << ':' << std::setw(2) << std::setfill('0') << second();
  return QString::fromStdString(ss.str());
}
