#pragma once

#include "config.h"
#include "interpolation.cpp"
#include "toolpath_exporter/toolpath-exporter-types.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QRectF>
#include <initializer_list>
#include <vector>

#define FORWARD_TO_GENERATOR(FUNC)                           \
  template <typename... Args>                                \
  auto FUNC(Args&&... args)                                  \
      -> decltype(gen_->FUNC(std::forward<Args>(args)...)) { \
    return gen_->FUNC(std::forward<Args>(args)...);          \
  }

struct CurveEngravingData {
  QRectF bbox;
  QPointF gap;
  float safe_height;
  CloughTocher2DInterpolator interpolator;

  bool started = false;
  double z_speed_limit = 0;
  double target_feedrate = 0;
};

struct NamedArgs {
  float f = NAN;
  qreal x = NAN;
  qreal y = NAN;
  float z = NAN;
  float a = NAN;
  float s = NAN;
  bool force_y = false;
  bool is_travel = false;

  // For chaining initialization
  NamedArgs& rf(float v) { f = v; return *this; }
  NamedArgs& rx(qreal v) { x = v; return *this; }
  NamedArgs& ry(qreal v) { y = v; return *this; }
  NamedArgs& rz(float v) { z = v; return *this; }
  NamedArgs& ra(float v) { a = v; return *this; }
  NamedArgs& rs(float v) { s = v; return *this; }
  NamedArgs& set_force_y(bool v = true) { force_y = v; return *this; }
  NamedArgs& set_is_travel(bool v = true) { is_travel = v; return *this; }
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
  bool s_curve_enabled = false;
  float s_curve_a0 = 0;
  float s_curve_a_max = 0;
  float s_curve_jerk = 0;
  QJsonObject metadata{};

  virtual void write(const char* buf,
                     size_t size,
                     unsigned long* crc32_ptr) = 0;
  virtual void write(const char* buf,
                     size_t size,
                     unsigned long* crc32_ptr,
                     bool to_all) = 0;
  void write(float value, unsigned long* crc32);
  void write(double value, unsigned long* crc32);
  void write(uint32_t value, unsigned long* crc32, bool to_all = false);
  void write(uint16_t value, unsigned long* crc32);
  void write(uint8_t value, unsigned long* crc32);
  void write(char value, unsigned long* crc32);
  void write_command(unsigned char cmd, unsigned long* crc32);
  void write_magic_number(int magic_number);

 public:
  static const int FLAG_S = 1;
  static const int FLAG_A = 4;
  static const int FLAG_Z = 8;
  static const int FLAG_Y = 16;
  static const int FLAG_X = 32;
  static const int FLAG_F = 64;
  static const int FLAG_Q = 128;

  virtual ~FCodeGenerator() = default;

  virtual std::string to_string() = 0;
  virtual size_t total_length() = 0;
  virtual float get_time_cost() = 0;
  QJsonObject get_metadata();
  void add_metadata(const QString& key, const QJsonValue& value);

  void set_time_est_acc(uint32_t x, uint32_t y = 2000);
  void set_time_est_z_speed(float value);
  void set_s_curve_params(float a0, float a_max, float jerk);
  void set_s_curve_enabled(bool enabled);
  virtual void moveto(int flags,
                      float feedrate,
                      float x,
                      float y,
                      float z,
                      float a,
                      float s);
  virtual void home(void);
  virtual void sleep(float seconds);
  void pause(bool to_standby_position);
  virtual void set_toolhead_pwm(float strength);
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
  void set_printer_packet_length(uint32_t length, bool is_4c = false);
  void start_printer_packet_payload(bool is_4c = false);
  void add_printer_packet_payload(uint8_t byte);
  void set_printer_packet_crc(uint16_t val, bool is_4c = false);
  void start_printer_packet(uint8_t packet_type, bool is_4c = false);
  void end_printer_packet(bool is_4c = false);
  void set_printer_packet_px_count(uint32_t count);
  // End of printing mode
  void sync_grbl_motion(uint32_t val);
  void sync_motion_type2(uint32_t cmd, int flags, float q);
  void m137_cmd_type1(uint32_t cmd,
                      int flags,
                      float f,
                      float x,
                      float y,
                      float z,
                      float a,
                      float s);
  void flux_custom_cmd(uint32_t val);
  void one_seg_custom_cmd(int type, uint8_t cmd);
  // Promark mode, V2 only
  // BSL (galvo/Promark) command payload. Keeps the outer command byte 23 and
  // encodes the data like the reference galvo-command format: uint16 opcode
  // (LE), uint8 param count, then each param as a float64 (LE).
  void write_bsl_command(uint16_t opcode, std::initializer_list<double> params);
  void enter_promark_mode(void); // TODO: check rotary, red light
  void set_promark_block_center(float x, float y);
  void set_promark_pulse(float period, float pulse_length, uint16_t mopa_pulse);
  void set_promark_dotting_time(int dotting_time);
  void set_promark_wobble(float step, float diameter); // with wobble k
  void overwrite_promark_gradient_resolution(float resolution);
  void exit_promark_mode(void);
  void start_promark_task(void);
  // Grouped Promark setup commands (opcodes 8/9/10), matching the execution
  // end's Promark API call groups. 8/9 map to direct lcs_set_* calls; 10's axis
  // ratios are saved on the execution end for MoveAxis + time estimation.
  void set_promark_motion_ctrl(double jump_speed,
                               double mark_speed,
                               int jump_delay_min,
                               int jump_delay_max,
                               int jump_delay_limit);
  void set_promark_laser_scanner_delays(int laser_on_delay,
                                        int laser_off_delay,
                                        int scanner_mark_delay,
                                        int scanner_polygon_delay);
  void set_promark_axis_config(double z_pulse_per_mm,
                               double z_pulse_per_sec,
                               double a_pulse_per_mm,
                               double a_pulse_per_sec);
  // End of Promark mode

  // v1 only
  virtual void terminated() {};
  // v2 only
  virtual void end_content() {}
  virtual void write_post_config(const char* s, size_t length) {}
  virtual void write_string(const char* s,
                            size_t length,
                            bool write_length = false) {}
  virtual void start_task_script_block(const char* header,
                                       const char* proc_id) {}
  virtual void write_task_info(QJsonObject task_info) {}
  virtual void end_task_script_block(void) {}
};

class FCodeGeneratorV1 : public FCodeGenerator {
 private:
  std::ostream* stream;
  int script_offset;
  // for estimating time_cost
  float last_feedrate;
  float last_direction;
  float last_acc;
  float current_feedrate, current_x, current_y, current_z;
  // metadata
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
  void sleep(float seconds) override;
  void write_metadata_(QString key, QString value, unsigned long* crc_ptr);
  unsigned long write_metadata();

 public:
  FCodeGeneratorV1(const QString* canvas_thumbnail);
  ~FCodeGeneratorV1();

  std::string to_string() override;
  size_t total_length() override;
  float get_time_cost() override;
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
  float last_acc;
  float current_feedrate, current_x, current_y, current_z;
  // metadata
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
  void sleep(float seconds) override;
  void write_metadata_(QString key,
                       QString value,
                       unsigned long* crc32_ptr,
                       bool is_string = true,
                       bool has_next = true);
  unsigned long write_metadata();
  void end_content() override;
  void write_post_config(const char* s, size_t length) override;
};

class FCodeGeneratorG : public FCodeGenerator {
 private:
  const float move_precision = 10000;
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
  void set_toolhead_pwm(float strength) override;

 public:
  FCodeGeneratorG();
  ~FCodeGeneratorG();
  std::string to_string() override;
  size_t total_length() override;
  float get_time_cost() override;
};

using MoveCallback = std::function<void(NamedArgs args)>;

class ToolpathProcessor {
 private:
  std::shared_ptr<FCodeGenerator> gen;
  FCodeGenerator* gen_;

  bool is_a_mode_ = false;
  bool is_main_task_ = false;
  bool rotary_wait_move_ = false;
  bool rotary_enabled_ = false;
  float rotary_y_ = 0;
  float rotary_y_offset_ = 0;
  float rotary_y_ratio_ = 1;
  bool support_a_mode_ = false;
  float travel_speed_ = 12000;
  float a_travel_speed_ = 2000;

  std::vector<void (ToolpathProcessor::*)(NamedArgs args,
                                          MoveCallback callback)>
      moveto_pipeline_functions_;

  // For 3d curve
  float cur_x_ = 0;
  float cur_y_ = 0;
  float cur_z_ = NAN;
  float cur_f_ = 12000;

  // Boundary Metadata
  float min_x_ = NAN;
  float max_x_ = NAN;
  float min_y_ = NAN;
  float max_y_ = NAN;
  float min_z_ = NAN;
  float max_z_ = NAN;

 public:
  std::unique_ptr<CurveEngravingData> curve_engraving_data;
  ZPremoveData z_premove_;

  void init(int magic_number, const QString* thumbnail);
  void clear_curve_engraving_data();
  bool set_curve_engraving_data(const QJsonObject& curve_obj,
                                const QPointF& job_origin,
                                QSizeF work_area_size,
                                InwardRect& workarea_clip);
  void set_curve_engraving_data_by_key(QString& key, double value);
  void set_z_premove(ZPremoveData data);
  void set_a_mode(bool a_mode);
  void set_is_main_task(bool is_main_task);
  void set_rotary_axis(float rotary_y);
  void set_rotary_y_ratio(float ratio);
  void set_travel_speed(float feedrate = NAN, bool a_axis = false);
  float get_travel_speed(bool a_axis = false);
  void set_rotary_wait_move(bool wait, float y);
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
  void moveto(NamedArgs args);
  void rotary_motion_generator(NamedArgs args, MoveCallback callback);
  void curve_engraving_motion_generator(NamedArgs args, MoveCallback callback);
  void z_premove_motion_generator(NamedArgs args, MoveCallback callback);
  void _moveto(NamedArgs args);
  void pause(bool to_standby_position);
  void home();
  void m137_cmd_type1(unsigned cmd, NamedArgs args);
  void sync_motion_type2(unsigned cmd, float q = NAN);
  void set_acceleration_override(float x = NAN,
                                 float y = NAN,
                                 float z = NAN,
                                 float a = NAN);
  void set_s_curve_params(float a0, float a_max, float jerk);
  void user_selection_cmd(unsigned cmd);
  void miscellaneous_cmd(unsigned cmd);
  void grbl_system_cmd(unsigned cmd);
  void write_post_config(const QJsonArray& post_config);
  void write_boundary_to_metadata();
  void write_printer_packet(int printer_packet_type,
                            QByteArray payload,
                            bool should_wait = false,
                            bool is_4c = false);

  FORWARD_TO_GENERATOR(get_time_cost)
  FORWARD_TO_GENERATOR(sleep)
  FORWARD_TO_GENERATOR(set_toolhead_pwm)
  FORWARD_TO_GENERATOR(set_toolhead_laser_module)
  FORWARD_TO_GENERATOR(turn_on_gradient_print_mode)
  FORWARD_TO_GENERATOR(turn_off_gradient_print_mode)
  FORWARD_TO_GENERATOR(set_line_pixels)
  FORWARD_TO_GENERATOR(fill_32_pixels)
  FORWARD_TO_GENERATOR(set_time_est_acc)
  FORWARD_TO_GENERATOR(set_time_est_z_speed)
  FORWARD_TO_GENERATOR(set_s_curve_enabled)
  FORWARD_TO_GENERATOR(set_fill_end)
  FORWARD_TO_GENERATOR(set_print_line_status)
  FORWARD_TO_GENERATOR(enter_printer_mode)
  FORWARD_TO_GENERATOR(wait_printer_mode_sync)
  FORWARD_TO_GENERATOR(exit_printer_mode)
  FORWARD_TO_GENERATOR(start_printer_packet)
  FORWARD_TO_GENERATOR(end_printer_packet)
  FORWARD_TO_GENERATOR(set_printer_packet_px_count)
  FORWARD_TO_GENERATOR(set_printer_packet_length)
  FORWARD_TO_GENERATOR(start_printer_packet_payload)
  FORWARD_TO_GENERATOR(add_printer_packet_payload)
  FORWARD_TO_GENERATOR(set_printer_packet_crc)
  FORWARD_TO_GENERATOR(sync_grbl_motion)
  FORWARD_TO_GENERATOR(flux_custom_cmd)
  FORWARD_TO_GENERATOR(enter_promark_mode)
  FORWARD_TO_GENERATOR(set_promark_block_center)
  FORWARD_TO_GENERATOR(set_promark_pulse)
  FORWARD_TO_GENERATOR(set_promark_dotting_time)
  FORWARD_TO_GENERATOR(set_promark_wobble)
  FORWARD_TO_GENERATOR(overwrite_promark_gradient_resolution)
  FORWARD_TO_GENERATOR(exit_promark_mode)
  FORWARD_TO_GENERATOR(start_promark_task)
  FORWARD_TO_GENERATOR(set_promark_motion_ctrl)
  FORWARD_TO_GENERATOR(set_promark_laser_scanner_delays)
  FORWARD_TO_GENERATOR(set_promark_axis_config)
  FORWARD_TO_GENERATOR(end_content)
  FORWARD_TO_GENERATOR(add_metadata)
  FORWARD_TO_GENERATOR(write_string)
  FORWARD_TO_GENERATOR(start_task_script_block)
  FORWARD_TO_GENERATOR(end_task_script_block)
  FORWARD_TO_GENERATOR(terminated)
  FORWARD_TO_GENERATOR(write_task_info)
  FORWARD_TO_GENERATOR(to_string)
  FORWARD_TO_GENERATOR(total_length)
  FORWARD_TO_GENERATOR(get_metadata)
};
