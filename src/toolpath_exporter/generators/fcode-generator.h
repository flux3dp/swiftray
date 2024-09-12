#pragma once

#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "config.h"
#include "crc16.c"
#include "crc32.c"
#include "estimate_time.cpp"

class FCodeGenerator {
 protected:
  static const int magic_number_size = 8;
  unsigned long script_crc32;
  float acc_x = 4000;
  float acc_y = 2000;
  QString time_format = "yyyy-MM-ddTHH:mm:ssZ";
  QString sw_version = QString("swiftray-%1.%2.%3")
                           .arg(VERSION_MAJOR)
                           .arg(VERSION_MINOR)
                           .arg(VERSION_BUILD);

  virtual void write(const char* buf,
                     size_t size,
                     unsigned long* crc32_ptr) = 0;
  // to_all: to full fcode or to task content, v2 only
  virtual void write(const char* buf,
                     size_t size,
                     unsigned long* crc32_ptr,
                     bool to_all) = 0;

  void write(float value, unsigned long* crc32) {
    write((const char*)&value, 4, crc32);
  }

  void write(uint32_t value, unsigned long* crc32, bool to_all = false) {
    write((const char*)&value, sizeof(uint32_t), crc32, to_all);
  }

  void write(uint16_t value, unsigned long* crc32) {
    write((const char*)&value, sizeof(uint16_t), crc32);
  }

  void write(uint8_t value, unsigned long* crc32) {
    write((const char*)&value, sizeof(uint8_t), crc32);
  }

  void write(char value, unsigned long* crc32) {
    write((const char*)&value, sizeof(char), crc32);
  }

  void write_command(unsigned char cmd, unsigned long* crc32) {
    write((const char*)&cmd, 1, crc32);
  }

  void write_magic_number(int magic_number) {
    write(QString("FCx%1\n")
              .arg(magic_number, magic_number_size - 4, 10, QChar('0'))
              .toStdString()
              .c_str(),
          magic_number_size, NULL, true);
  }

 public:
  static const int move_flag_S = 1;
  static const int move_flag_A = 4;
  static const int move_flag_Z = 8;
  static const int move_flag_Y = 16;
  static const int move_flag_X = 32;
  static const int move_flag_F = 64;
  float current_pwm = 0;

  virtual std::string to_string() = 0;
  virtual size_t total_length() = 0;
  virtual float get_time_cost() = 0;

  void set_time_est_acc(uint32_t x, uint32_t y = 2000) {
    acc_x = (float)x;
    acc_y = (float)y;
  }

  virtual void moveto(int flags,
                      float feedrate,
                      float x,
                      float y,
                      float z,
                      float a,
                      float s) {
    write_command(flags | 128, &script_crc32);
    if (flags & move_flag_F && feedrate > 0) {
      write(feedrate, &script_crc32);
    }
    if (flags & move_flag_X) {
      write(x, &script_crc32);
    }
    if (flags & move_flag_Y) {
      write(y, &script_crc32);
    }
    if (flags & move_flag_Z) {
      write(z, &script_crc32);
    }
    if (flags & move_flag_A) {
      write(a, &script_crc32);
    }
    if (flags & move_flag_S) {
      write(s, &script_crc32);
    }
  }
  virtual void sleep(float seconds) {
    write_command(4, &script_crc32);
    write(seconds * 1000, &script_crc32);
  }
  virtual void home(void) { write_command(1, &script_crc32); }

  void pause(bool to_standby_position) {
    write_command((to_standby_position ? 5 : 6), &script_crc32);
  }

  void set_toolhead_heater_temperature(float temperature, bool wait) {
    write_command(wait ? 24 : 16, &script_crc32);
    write(temperature, &script_crc32);
  }

  void set_toolhead_fan_speed(float strength) {
    write_command(48, &script_crc32);
    write(strength, &script_crc32);
  }

  void set_toolhead_pwm(float strength, bool update = false) {
    if (update) {
      current_pwm = strength;
    }
    write_command(32, &script_crc32);
    write(strength, &script_crc32);
  }

  void dwell_cmd(uint32_t milli_second) {
    write_command(4, &script_crc32);
    write(milli_second, &script_crc32);
  }

  void set_toolhead_laser_module(uint32_t laser_type) {
    write_command(7, &script_crc32);
    write(laser_type, &script_crc32);
  }

  void set_calibrate(void) {
    write_command(8, &script_crc32);
    write(uint32_t(1), &script_crc32);
  }

  void turn_on_gradient_print_mode(char resolution) {
    write_command(16, &script_crc32);
    write(uint8_t(1), &script_crc32);
    write(resolution, &script_crc32);
  }

  void turn_off_gradient_print_mode(void) {
    write_command(16, &script_crc32);
    write(uint8_t(6), &script_crc32);
  }

  void set_line_pixels(uint32_t pixel_number) {
    write_command(16, &script_crc32);
    write(uint8_t(2), &script_crc32);
    write(pixel_number, &script_crc32);
  }

  void fill_32_pixels(uint32_t pixels) {
    write_command(16, &script_crc32);
    write(uint8_t(3), &script_crc32);
    write(pixels, &script_crc32);
  }

  void set_fill_end(void) {
    write_command(16, &script_crc32);
    write(uint8_t(4), &script_crc32);
  }

  void set_print_line_status(void) {
    write_command(16, &script_crc32);
    write(uint8_t(5), &script_crc32);
  }

  void enter_printer_mode(void) {
    write_command(18, &script_crc32);
    write(uint8_t(0), &script_crc32);
    write(uint32_t(1), &script_crc32);
  }

  void wait_printer_mode_sync(void) {
    write_command(18, &script_crc32);
    write(uint8_t(0), &script_crc32);
    write(uint32_t(0), &script_crc32);
  }

  void exit_printer_mode(void) {
    write_command(18, &script_crc32);
    write(uint8_t(0), &script_crc32);
    write(uint32_t(2), &script_crc32);
  }

  void set_printer_packet_length(uint32_t length) {
    write_command(17, &script_crc32);
    write(uint8_t(0), &script_crc32);
    write(length, &script_crc32);
  }

  void start_printer_packet_payload(void) {
    write_command(17, &script_crc32);
    write(uint8_t(1), &script_crc32);
  }

  void add_printer_packet_payload(uint8_t byte) { write(byte, &script_crc32); }

  void set_printer_packet_crc(uint16_t val) {
    write_command(17, &script_crc32);
    write(uint8_t(2), &script_crc32);
    write(val, &script_crc32);
  }

  void start_printer_packet(uint8_t packet_type) {
    write_command(17, &script_crc32);
    write(uint8_t(3), &script_crc32);
    write(packet_type, &script_crc32);
  }

  void end_printer_packet(void) {
    write_command(17, &script_crc32);
    write(uint8_t(4), &script_crc32);
  }

  void set_printer_packet_px_count(uint32_t count) {
    write_command(17, &script_crc32);
    write(uint8_t(5), &script_crc32);
    write(count, &script_crc32);
  }

  void write_printer_packet(QByteArray payload) {
    set_printer_packet_length(payload.size());
    start_printer_packet_payload();
    for (auto b : payload) {
      add_printer_packet_payload(b);
    }
    set_printer_packet_crc(crc16(payload));
  }

  void sync_grbl_motion(uint32_t val) {
    write_command(18, &script_crc32);
    write(uint8_t(0), &script_crc32);
    write(val, &script_crc32);
  }

  void sync_motion_type2(uint32_t cmd, int field, float value) {
    write_command(18, &script_crc32);
    write(uint8_t(2), &script_crc32);
    write(cmd, &script_crc32);
    write_command(field, &script_crc32);
    if (field & 128)
      write(value, &script_crc32);  // Q
  }

  void set_path_acceleration(int flags, float x, float y, float z, float a) {
    write_command(18, &script_crc32);
    write(uint8_t(1), &script_crc32);
    write(uint32_t(150), &script_crc32);
    write_command(flags, &script_crc32);
    if (flags & move_flag_X)
      write(x, &script_crc32);
    if (flags & move_flag_Y)
      write(y, &script_crc32);
    if (flags & move_flag_Z)
      write(z, &script_crc32);
    if (flags & move_flag_A)
      write(a, &script_crc32);
    acc_x = x;
    acc_y = y;
  }

  void flux_custom_cmd(uint32_t val) {
    write_command(19, &script_crc32);
    write(uint8_t(0), &script_crc32);
    write(val, &script_crc32);
  }

  void one_seg_custom_cmd(int type, uint8_t cmd) {
    write_command(type, &script_crc32);
    write(cmd, &script_crc32);
  }
  void user_selection_cmd(uint8_t cmd) { one_seg_custom_cmd(20, cmd); }
  void miscellaneous_cmd(uint8_t cmd) { one_seg_custom_cmd(21, cmd); }
  void grbl_system_cmd(uint8_t cmd) { one_seg_custom_cmd(22, cmd); }

  // v1 only
  virtual void terminated(void) {};
  // v2 only
  virtual void end_content(void) {}
  virtual void write_post_config(const QJsonArray post_config) {}
  virtual void append_anchor(uint32_t value) {}
  virtual void write_string(const char* s,
                            size_t length,
                            bool write_length = false) {}
  virtual void start_task_script_block(const char* header,
                                       const char* proc_id) {}
  virtual void write_task_info(QJsonObject task_info) {}
  virtual void end_task_script_block(void) {}
  virtual void append_comment(const char* message, size_t length) {}
};

class FCodeGeneratorV1 : public FCodeGenerator {
 private:
  std::ostream* stream;
  int script_offset;
  // for estimating time_cost
  float last_feedrate;
  float last_direction;
  float current_feedrate, current_x, current_y, current_z;
  // metadata
  QDateTime created_at;
  double traveled;
  double time_cost;
  float max_x, max_y, max_z, max_r;
  const QString* thumbnail;
  bool start_with_home;

  void write(const char* buf, size_t size, unsigned long* crc32_ptr) override {
    stream->write(buf, size);
    if (crc32_ptr) {
      *crc32_ptr = crc32(*crc32_ptr, (const void*)buf, size);
    }
  }

  void write(const char* buf,
             size_t size,
             unsigned long* crc32_ptr,
             bool to_all) override {
    write(buf, size, crc32_ptr);
  }

  void moveto(int flags,
              float feedrate,
              float x,
              float y,
              float z,
              float a,
              float s) {
    if (flags & FCodeGenerator::move_flag_F && feedrate > 0) {
      current_feedrate = feedrate / 60;
    }

    bool has_move = false;
    float mv[3] = {0, 0, 0};

    if (flags & FCodeGenerator::move_flag_X) {
      mv[0] = x - current_x;
      current_x = x;
      max_x = fmax(max_x, x);
      has_move = true;
    }
    if (flags & FCodeGenerator::move_flag_Y) {
      mv[1] = y - current_y;
      current_y = y;
      max_y = fmax(max_y, y);
      has_move = true;
    } else if (flags & FCodeGenerator::move_flag_A) {
      mv[1] = a - current_y;
      current_y = a;
      max_y = fmax(max_y, a);
      has_move = true;
    }
    if (flags & FCodeGenerator::move_flag_X ||
        flags & FCodeGenerator::move_flag_Y) {
      float r = sqrtf(pow(current_x, 2) + pow(current_y, 2));
      max_r = fmax(max_r, r);
      has_move = true;
    }
    if (flags & FCodeGenerator::move_flag_Z) {
      if (z < 0) {
        current_z = 0;
      } else {
        mv[2] = z - current_z;
        current_z = z;
        max_z = fmax(max_z, z);
        has_move = true;
      }
    }

    if (has_move) {
      double dist = sqrt(pow(mv[0], 2) + pow(mv[1], 2));
      if (!isnan(dist) && dist > 0) {
        traveled += dist;
        if (current_feedrate > 0) {
          float direction = atan2(mv[1], mv[0]);
          float last_vel = (last_feedrate * cos(direction - last_direction)); // consider direction
          float acc = acc_x;
          if (abs(mv[1]) > 0)
            acc = acc_y;
          float vel = estimate_vel(last_vel, current_feedrate, acc, dist); // consider short distance
          float tc = estimate_time(last_vel, vel, acc, dist);
          if (!isnan(tc)) {
            time_cost += tc;
          }
          last_feedrate = vel;
          last_direction = direction;
        }
      } else if (abs(mv[2]) > 0) {
        float dist = abs(mv[2]);
        traveled += dist;
        // autofocus movespeed 7.5 mm/s
        float tc = (dist / 7.5);
        if (!isnan(tc)) {
          time_cost += tc;
        }
      }
    }
    FCodeGenerator::moveto(flags, feedrate, x, y, z, a, s);
  }

  void sleep(float seconds) {
    if (!isnan(seconds)) {
      time_cost += seconds;
    }
    FCodeGenerator::sleep(seconds);
  }

  void home(void) {
    current_x = current_y = current_z = 0;
    FCodeGenerator::home();
  }

  void write_metadata_(QString key, QString value, unsigned long* crc_ptr) {
    write(key.toStdString().c_str(), key.size(), crc_ptr);
    write("=", 1, crc_ptr);
    write(value.toStdString().c_str(), value.size(), crc_ptr);
    write("\x00", 1, crc_ptr);
  }

  unsigned long write_metadata(void) {
    unsigned long crc_val = 0;
    write_metadata_("VERSION", "1", &crc_val);
    write_metadata_("HEAD_TYPE", "LASER", &crc_val);
    write_metadata_("TIME_COST", QString::number(time_cost, 'f', 2), &crc_val);
    write_metadata_("TRAVEL_DIST", QString::number(traveled, 'f', 2), &crc_val);
    write_metadata_("MAX_X", QString::number(max_x + 0.2, 'f', 2), &crc_val);
    write_metadata_("MAX_Y", QString::number(max_y + 0.2, 'f', 2), &crc_val);
    write_metadata_("MAX_Z", QString::number(max_z + 0.2, 'f', 2), &crc_val);
    write_metadata_("MAX_R", QString::number(max_r + 0.2, 'f', 2), &crc_val);
    write_metadata_("CREATED_AT", created_at.toString(time_format), &crc_val);
    write_metadata_("SOFTWARE", sw_version, &crc_val);
    write_metadata_("START_WITH_HOME", start_with_home ? "1" : "0", &crc_val);
    return crc_val;
  }

 public:
  FCodeGeneratorV1(const QString* canvas_thumbnail, bool with_custom_origin) : thumbnail(canvas_thumbnail) {
    qInfo() << "FCodeGenerator V1 init";
    stream = new std::stringstream();
    created_at = QDateTime::currentDateTime();
    current_feedrate = last_feedrate = 0;
    last_direction = 0;
    current_x = current_y = current_z = 0;
    traveled = time_cost = 0;
    max_x = max_y = max_z = max_r = 0;
    script_crc32 = 0;
    start_with_home = !with_custom_origin;

    write_magic_number(1);
    script_offset = stream->tellp();
    if (script_offset < 0) {
      throw std::runtime_error("NOT_SUPPORT STREAM");
    }
    // Keep space for script size
    write("\x00\x00\x00\x00", 4, NULL);
  }

  ~FCodeGeneratorV1() { delete stream; }

  std::string to_string() override {
    return ((std::stringstream*)stream)->str();
  };

  size_t total_length() override {
    return int(stream->tellp()) - script_offset + magic_number_size;
  }

  float get_time_cost() override { return time_cost; }

  void terminated(void) override {
    uint32_t u32value;

    // Write script size and CRC32
    int script_end_offset = stream->tellp();
    stream->seekp(script_offset, stream->beg);
    u32value = script_end_offset - script_offset - 4;
    FCodeGenerator::write(u32value, NULL);
    stream->seekp(script_end_offset, stream->beg);
    FCodeGenerator::write((uint32_t)script_crc32, NULL);

    // Write metadata
    int metadata_offset = stream->tellp();
    int metadata_end_offset;
    write("\x00\x00\x00\x00", 4, NULL);
    unsigned long metadata_crc32 = write_metadata();
    metadata_end_offset = stream->tellp();
    stream->seekp(metadata_offset, stream->beg);
    u32value = metadata_end_offset - metadata_offset - 4;
    FCodeGenerator::write(u32value, NULL);
    stream->seekp(metadata_end_offset, stream->beg);
    FCodeGenerator::write((uint32_t)metadata_crc32, NULL);

    // Write image previews
    QByteArray thumbnail_data;
    QBuffer buffer(&thumbnail_data);
    QImage thumbnail_image;
    thumbnail_image.loadFromData(QByteArray::fromBase64(thumbnail->toLatin1().mid(thumbnail->indexOf(",") + 1)));
    thumbnail_image.save(&buffer, "PNG");
    u32value = thumbnail_data.size();
    FCodeGenerator::write(u32value, NULL);
    write(thumbnail_data.data(), u32value, NULL);
    write("\x00\x00\x00\x00", 4, NULL);
  }
};

class FCodeGeneratorV2 : public FCodeGenerator {
 private:
  std::ostream* content_stream;
  std::ostream* fc_stream;
  int start;
  /// task info
  int current_task_script_start;
  double current_task_traveled;
  double current_task_time_cost;
  // for estimating time_cost
  float last_feedrate;
  float last_direction;
  float current_feedrate, current_x, current_y, current_z;
  // metadata
  QDateTime created_at;
  double traveled;
  double time_cost;
  float max_x, max_y, max_z;
  const QString* thumbnail;
  bool start_with_home;

  // Ignore crc32_ptr; will be calculated when copying to fc_stream
  void write(const char* buf, size_t size, unsigned long* crc32_ptr) override {
    content_stream->write(buf, size);
  }

  void write_to_all(const char* buf, size_t size, unsigned long* crc32_ptr) {
    fc_stream->write(buf, size);
    if (crc32_ptr) {
      *crc32_ptr = crc32(*crc32_ptr, (const void*)buf, size);
    }
  }

  void write(const char* buf,
             size_t size,
             unsigned long* crc32_ptr,
             bool to_all) override {
    if (to_all) {
      write_to_all(buf, size, crc32_ptr);
    } else {
      write(buf, size, crc32_ptr);
    }
  }

 public:
  FCodeGeneratorV2(const QString* canvas_thumbnail, int magic_number, bool with_custom_origin)
      : thumbnail(canvas_thumbnail) {
    qInfo() << "FCodeGenerator V2 init";
    created_at = QDateTime::currentDateTime();
    fc_stream = new std::stringstream();
    content_stream = new std::stringstream();
    current_feedrate = last_feedrate = 0;
    last_direction = 0;
    current_x = current_y = current_z = 0;
    traveled = time_cost = 0;
    max_x = max_y = max_z = 0;
    script_crc32 = 0;
    start_with_home = !with_custom_origin;

    start = fc_stream->tellp();
    write_magic_number(magic_number);
  }

  ~FCodeGeneratorV2() {
    delete fc_stream;
    delete content_stream;
  }

  std::string to_string() override {
    return ((std::stringstream*)fc_stream)->str();
  };

  size_t total_length() override { return int(fc_stream->tellp()) - start; };

  float get_time_cost() override { return time_cost; }

  void write_string(const char* s,
                    size_t length,
                    bool write_length = false) override {
    if (write_length) {
      FCodeGenerator::write((uint32_t)length, &script_crc32);
    }
    write(s, length, &script_crc32);
  }

  void start_task_script_block(const char* header,
                               const char* proc_id) override {
    write(header, 4, NULL);
    if (proc_id) {
      write(proc_id, 4, NULL);
    }
    current_task_script_start = content_stream->tellp();
    current_task_traveled = traveled;
    current_task_time_cost = time_cost;
    write("\x00\x00\x00\x00", 4, NULL);
  }

  void end_task_script_block(void) override {
    int end = content_stream->tellp();
    content_stream->seekp(current_task_script_start, content_stream->beg);
    uint32_t task_length = end - current_task_script_start - 4;
    FCodeGenerator::write(task_length, NULL);
    content_stream->seekp(end, content_stream->beg);
  }

  void write_task_info(QJsonObject task_info) {
    current_task_traveled = traveled - current_task_traveled;
    task_info.insert("time_cost", round(current_task_time_cost * 100) / 100);
    current_task_time_cost = time_cost - current_task_time_cost;
    task_info.insert("travel_dist", round(current_task_traveled * 100) / 100);
    QJsonDocument doc(task_info);
    QString str(doc.toJson(QJsonDocument::Compact));
    str.replace("\\\\", "\\");
    write_string("INFO", 4);
    write_string(str.toStdString().c_str(), str.size(), true);
  }

  void moveto(int flags,
              float feedrate,
              float x,
              float y,
              float z,
              float a,
              float s) {
    if (flags & move_flag_F && feedrate > 0) {
      current_feedrate = feedrate / 60;
    }

    bool has_move = false;
    float mv[3] = {0, 0, 0};

    if (flags & move_flag_X) {
      mv[0] = x - current_x;
      current_x = x;
      max_x = fmax(max_x, x);
      has_move = true;
    }
    if (flags & move_flag_Y) {
      mv[1] = y - current_y;
      current_y = y;
      max_y = fmax(max_y, y);
      has_move = true;
    } else if (flags & move_flag_A) {
      mv[1] = a - current_y;
      current_y = a;
      max_y = fmax(max_y, a);
      has_move = true;
    }
    if (flags & move_flag_Z) {
      if (z < 0) {
        // Autofocus Homing
        current_z = 0;
      } else {
        mv[2] = z - current_z;
        current_z = z;
        max_z = fmax(max_z, z);
        has_move = true;
      }
    }

    if (has_move) {
      if (abs(mv[2]) > 0) {
        float dist = abs(mv[2]);
        traveled += dist;
        // autofocus movespeed 7.5 mm/s
        float tc = (dist / 7.5);
        if (!isnan(tc))
          time_cost += tc;
      } else {
        double dist = sqrt(pow(mv[0], 2) + pow(mv[1], 2));
        if (!isnan(dist) && dist > 0) {
          traveled += dist;
          if (current_feedrate > 0) {
            float direction = atan2(mv[1], mv[0]);
            float last_vel = last_feedrate * cos(direction - last_direction); // consider direction
            float acc = acc_x;
            if (abs(mv[1]) > 0)
              acc = acc_y;
            float vel = estimate_vel(last_vel, current_feedrate, acc, dist); // consider short distance
            float tc = estimate_time(last_vel, vel, acc, dist);
            if (!isnan(tc))
              time_cost += tc;
            last_feedrate = vel;
            last_direction = direction;
          }
        }
      }
    }
    FCodeGenerator::moveto(flags, feedrate, x, y, z, a, s);
  }

  void sleep(float seconds) {
    if (!isnan(seconds))
      time_cost += seconds;
    FCodeGenerator::sleep(seconds);
  }

  void home(void) {
    current_x = current_y = current_z = 0;
    FCodeGenerator::home();
  }

  void write_metadata_(QString key,
                       QString value,
                       unsigned long* crc32_ptr,
                       bool is_string = true,
                       bool has_next = true) {
    write_to_all("\"", 1, crc32_ptr);
    write_to_all(key.toStdString().c_str(), key.size(), crc32_ptr);
    write_to_all("\":", 2, crc32_ptr);
    if (is_string)
      write_to_all("\"", 1, crc32_ptr);
    write_to_all(value.toStdString().c_str(), value.size(), crc32_ptr);
    if (is_string)
      write_to_all("\"", 1, crc32_ptr);
    if (has_next)
      write_to_all(",", 1, crc32_ptr);
  }

  unsigned long write_metadata(void) {
    unsigned long crc_val = 0;
    write_to_all("{", 1, &crc_val);
    write_metadata_("version", "2", &crc_val);
    write_metadata_("CREATED_AT", created_at.toString(time_format), &crc_val);
    write_metadata_("SOFTWARE", sw_version, &crc_val);
    write_metadata_("START_WITH_HOME", start_with_home ? "1" : "0", &crc_val);
    write_metadata_("max_z", QString::number(max_z + 0.2, 'f', 2), &crc_val, false);
    write_metadata_("max_y", QString::number(max_y + 0.2, 'f', 2), &crc_val, false);
    write_metadata_("max_x", QString::number(max_x + 0.2, 'f', 2), &crc_val, false);
    write_metadata_("travel_dist", QString::number(traveled, 'f', 2), &crc_val, false);
    write_metadata_("time_cost", QString::number(time_cost, 'f', 2), &crc_val, false, false);
    write_to_all("}", 1, &crc_val);
    return crc_val;
  }

  void end_content(void) {
    // Write metadata
    uint32_t u32value;
    write_to_all("FILE", 4, NULL);
    int metadata_start_pos = fc_stream->tellp();
    write_to_all("\x00\x00\x00\x00", 4, NULL);
    unsigned long metadata_crc32 = write_metadata();
    int metadata_end_pos = fc_stream->tellp();
    fc_stream->seekp(metadata_start_pos, fc_stream->beg);
    u32value = metadata_end_pos - metadata_start_pos - 4;
    FCodeGenerator::write(u32value, NULL, true);
    fc_stream->seekp(metadata_end_pos, fc_stream->beg);
    FCodeGenerator::write((uint32_t)metadata_crc32, NULL, true);

    // Write preview images
    write_to_all("PREV", 4, NULL);
    QByteArray thumbnail_data;
    QBuffer buffer(&thumbnail_data);
    QImage thumbnail_image;
    thumbnail_image.loadFromData(QByteArray::fromBase64(thumbnail->toLatin1().mid(thumbnail->indexOf(",") + 1)));
    thumbnail_image.save(&buffer, "PNG");
    u32value = thumbnail_data.size();
    FCodeGenerator::write(u32value, NULL, true);
    write_to_all(thumbnail_data.data(), u32value, NULL);

    // Copy content to fc_stream
    int content_end_offset = content_stream->tellp();
    u32value = content_end_offset;
    write_to_all("CONT", 4, NULL);
    FCodeGenerator::write(u32value, NULL, true);
    std::string content = ((std::stringstream*)content_stream)->str();
    script_crc32 = 0;
    write_to_all(content.c_str(), u32value, &script_crc32);
    FCodeGenerator::write((uint32_t)script_crc32, NULL, true);
  }

  void write_post_config(const QJsonArray post_config) {
    unsigned long post_config_crc32 = 0;
    QJsonDocument doc(post_config);
    QString str(doc.toJson(QJsonDocument::Compact));
    write_to_all("POST", 4, NULL);
    FCodeGenerator::write((uint32_t)str.size(), NULL, true);
    write_to_all(str.toStdString().c_str(), str.size(), &post_config_crc32);
    FCodeGenerator::write((uint32_t)post_config_crc32, NULL, true);
  }
};
