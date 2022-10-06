#ifndef TIMESTAMP_H
#define TIMESTAMP_H
#include <QString>

class Timestamp
{
public:
  explicit Timestamp();
  explicit Timestamp(int h, int m, int s = 0, int ms = 0);
  void reset();
  int totalMSecs() const;
  
  int millisecond() const;
  int second() const;
  int minute() const;
  int hour() const;

  int msecsTo(const Timestamp &t) const;
  int secsTo(const Timestamp &t) const;

  Timestamp& addMSecs(int ms);
  Timestamp& addSecs(int s);

  QString toString() const;
  
private:
  int ms_ = 0;
};

#endif // TIMESTAMP_H
