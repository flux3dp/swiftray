#pragma once

#include "config.h"
#include <QJsonArray>
#include <QJsonObject>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

struct NamedArgs {
  float f = NAN;
  qreal x = NAN;
  qreal y = NAN;
  float z = NAN;
  float a = NAN;
  float s = NAN;
  bool force_y = false;
  bool is_travel = false;
};

class FCodeGenerator {
 protected:
  static const int magic_number_size = 8;
  const QString time_format = "yyyy-MM-ddTHH:mm:ssZ";
  const QString sw_version = QString("swiftray-%1.%2.%3")
                                 .arg(VERSION_MAJOR)
                                 .arg(VERSION_MINOR)
                                 .arg(VERSION_BUILD);
  unsigned long script_crc32;
  float acc_x = 4000;
  float acc_y = 2000;
  float z_speed = 7.5;

  virtual void write(const char* buf,
                     size_t size,
                     unsigned long* crc32_ptr) = 0;
  virtual void write(const char* buf,
                     size_t size,
                     unsigned long* crc32_ptr,
                     bool to_all) = 0;
  void write(float value, unsigned long* crc32);
  void write(uint32_t value, unsigned long* crc32, bool to_all = false);
  void write(uint16_t value, unsigned long* crc32);
  void write(uint8_t value, unsigned long* crc32);
  void write(char value, unsigned long* crc32);
  void write_command(unsigned char cmd, unsigned long* crc32);
  void write_magic_number(int magic_number);

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
  virtual QJsonObject get_metadata() = 0;
  virtual void add_metadata(QString key, QString value) = 0;

  void set_time_est_acc(uint32_t x, uint32_t y = 2000);
  void set_time_est_z_speed(float value);
  virtual void moveto(int flags,
                      float feedrate,
                      float x,
                      float y,
                      float z,
                      float a,
                      float s);
  virtual void home(void);
  void pause(bool to_standby_position);
  virtual void set_toolhead_pwm(float strength, bool update = false);
  void set_toolhead_laser_module(uint32_t laser_type);
  // Gradient mode, fcode only
  void turn_on_gradient_print_mode(char resolution);
  void turn_off_gradient_print_mode(void);
  void set_line_pixels(uint32_t pixel_number);
  void fill_32_pixels(uint32_t pixels);
  void set_fill_end(void);
  void set_print_line_status(void);
  // End of gradient mode
  // Printing mode, V2 only
  void enter_printer_mode(void);
  void wait_printer_mode_sync(void);
  void exit_printer_mode(void);
  void set_printer_packet_length(uint32_t length);
  void start_printer_packet_payload(void);
  void add_printer_packet_payload(uint8_t byte);
  void set_printer_packet_crc(uint16_t val);
  void start_printer_packet(uint8_t packet_type);
  void end_printer_packet(void);
  void set_printer_packet_px_count(uint32_t count);
  void write_printer_packet(QByteArray payload);
  // End of printing mode
  void sync_grbl_motion(uint32_t val);
  void sync_motion_type2(uint32_t cmd, int field, float value);
  void set_acceleration(int flags, float x, float y, float z, float a);
  void flux_custom_cmd(uint32_t val);
  void one_seg_custom_cmd(int type, uint8_t cmd);
  void user_selection_cmd(uint8_t cmd) { one_seg_custom_cmd(20, cmd); }
  void miscellaneous_cmd(uint8_t cmd) { one_seg_custom_cmd(21, cmd); }
  void grbl_system_cmd(uint8_t cmd) { one_seg_custom_cmd(22, cmd); }

  // v1 only
  virtual void terminated() {};
  // v2 only
  virtual void end_content() {}
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
  QJsonObject metadata{};
  QDateTime created_at;
  double traveled;
  double time_cost;
  const QString* thumbnail;

  void write(const char* buf, size_t size, unsigned long* crc32_ptr) override;
  void write(const char* buf,
             size_t size,
             unsigned long* crc32_ptr,
             bool to_all) override;
  void moveto(int flags,
              float feedrate,
              float x,
              float y,
              float z,
              float a,
              float s) override;
  void home(void) override;
  void write_metadata_(QString key, QString value, unsigned long* crc_ptr);
  unsigned long write_metadata();

 public:
  FCodeGeneratorV1(const QString* canvas_thumbnail);
  ~FCodeGeneratorV1();

  std::string to_string() override;
  size_t total_length() override;
  float get_time_cost() override;
  QJsonObject get_metadata() override;
  void add_metadata(QString key, QString value) override;
  void terminated() override;
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
  QJsonObject metadata{};
  QDateTime created_at;
  double traveled;
  double time_cost;
  const QString* thumbnail;

  void write(const char* buf, size_t size, unsigned long* crc32_ptr) override;
  void write_to_all(const char* buf, size_t size, unsigned long* crc32_ptr);
  void write(const char* buf,
             size_t size,
             unsigned long* crc32_ptr,
             bool to_all) override;

 public:
  FCodeGeneratorV2(const QString* canvas_thumbnail, int magic_number);
  ~FCodeGeneratorV2();
  std::string to_string() override;
  size_t total_length() override;
  float get_time_cost() override;
  QJsonObject get_metadata() override;
  void add_metadata(QString key, QString value) override;
  void write_string(const char* s,
                    size_t length,
                    bool write_length = false) override;
  void start_task_script_block(const char* header,
                               const char* proc_id) override;
  void end_task_script_block(void) override;
  void write_task_info(QJsonObject task_info) override;
  void moveto(int flags,
              float feedrate,
              float x,
              float y,
              float z,
              float a,
              float s) override;
  void home(void) override;
  void write_metadata_(QString key,
                       QString value,
                       unsigned long* crc32_ptr,
                       bool is_string = true,
                       bool has_next = true);
  unsigned long write_metadata();
  void end_content() override;
  void write_post_config(const QJsonArray post_config) override;
};

class FCodeGeneratorG : public FCodeGenerator {
 private:
  std::stringstream str_stream;
  int script_offset;
  // dummy metadata
  QJsonObject metadata{};

  void write(const char* buf, size_t size, unsigned long* crc32_ptr) override;
  void write(const char* buf,
             size_t size,
             unsigned long* crc32_ptr,
             bool to_all) override;
  void moveto(int flags,
              float feedrate,
              float x,
              float y,
              float z,
              float a,
              float s) override;
  void home(void) override;
  void set_toolhead_pwm(float strength, bool update = false) override;

 public:
  FCodeGeneratorG();
  ~FCodeGeneratorG();
  std::string to_string() override;
  size_t total_length() override;
  float get_time_cost() override;
  QJsonObject get_metadata() override;
  void add_metadata(QString key, QString value) override;
};

using MoveCallback = std::function<void(NamedArgs args)>;

class ToolpathProcessor {
 private:
  std::shared_ptr<FCodeGenerator> gen;
  FCodeGenerator* gen_;

 public:
  void update_moveto_pipeline();
  void pipeline_moveto(int idx, NamedArgs args);
  void moveto(float feedrate = NAN,
              float x = NAN,
              float y = NAN,
              float z = NAN,
              float a = NAN,
              float s = NAN,
              bool force_y = false,
              bool is_travel = false);
  void rotary_motion_generator(NamedArgs args, MoveCallback callback);
  void curve_engraving_motion_generator(NamedArgs args, MoveCallback callback);
  void z_premove_motion_generator(NamedArgs args, MoveCallback callback);
  void _moveto(NamedArgs args);
  void write_boundary_to_metadata();
};
