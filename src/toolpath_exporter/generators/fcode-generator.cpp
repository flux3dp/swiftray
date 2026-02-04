#include "fcode-generator.h"
#include "crc16.c"
#include "crc32.c"
#include "estimate_time.cpp"
#include <QBuffer>
#include <QDebug>
#include <QJsonDocument>

void FCodeGenerator::write(float value, unsigned long* crc32) {
  write((const char*)&value, 4, crc32);
}

// to_all: to full fcode or to task content, v2 only
void FCodeGenerator::write(uint32_t value, unsigned long* crc32, bool to_all) {
  write((const char*)&value, sizeof(uint32_t), crc32, to_all);
}

void FCodeGenerator::write(uint16_t value, unsigned long* crc32) {
  write((const char*)&value, sizeof(uint16_t), crc32);
}

void FCodeGenerator::write(uint8_t value, unsigned long* crc32) {
  write((const char*)&value, sizeof(uint8_t), crc32);
}

void FCodeGenerator::write(char value, unsigned long* crc32) {
  write((const char*)&value, sizeof(char), crc32);
}

void FCodeGenerator::write_command(unsigned char cmd, unsigned long* crc32) {
  write((const char*)&cmd, 1, crc32);
}

void FCodeGenerator::write_magic_number(int magic_number) {
  write(QString("FCx%1\n")
            .arg(magic_number, magic_number_size - 4, 10, QChar('0'))
            .toStdString()
            .c_str(),
        magic_number_size, NULL, true);
}

QJsonObject FCodeGenerator::get_metadata() {
  return metadata;
}

void FCodeGenerator::add_metadata(const QString& key, const QJsonValue& value) {
  metadata.insert(key, value);
}

void FCodeGenerator::set_time_est_acc(uint32_t x, uint32_t y) {
  acc_x = (float)x;
  acc_y = (float)y;
}

void FCodeGenerator::set_time_est_z_speed(float value) {
  z_speed = value;
}

void FCodeGenerator::moveto(int flags,
                            float feedrate,
                            float x,
                            float y,
                            float z,
                            float a,
                            float s) {
  write_command(flags | 128, &script_crc32);
  if (flags & FLAG_F && feedrate > 0) {
    write(feedrate, &script_crc32);
  }
  if (flags & FLAG_X) {
    write(x, &script_crc32);
  }
  if (flags & FLAG_Y) {
    write(y, &script_crc32);
  }
  if (flags & FLAG_Z) {
    write(z, &script_crc32);
  }
  if (flags & FLAG_A) {
    write(a, &script_crc32);
  }
  if (flags & FLAG_S) {
    write(s, &script_crc32);
  }
}

void FCodeGenerator::home(void) {
  write_command(1, &script_crc32);
}

void FCodeGenerator::sleep(float seconds) {
  write_command(4, &script_crc32);
  write(uint32_t(seconds * 1000), &script_crc32);
}

void FCodeGenerator::pause(bool to_standby_position) {
  write_command((to_standby_position ? 5 : 6), &script_crc32);
}

void FCodeGenerator::set_toolhead_pwm(float strength) {
  write_command(32, &script_crc32);
  write(strength, &script_crc32);
}

void FCodeGenerator::set_toolhead_laser_module(uint32_t laser_type) {
  write_command(7, &script_crc32);
  write(laser_type, &script_crc32);
}

void FCodeGenerator::turn_on_gradient_print_mode(char resolution) {
  write_command(16, &script_crc32);
  write(uint8_t(1), &script_crc32);
  write(resolution, &script_crc32);
}

void FCodeGenerator::turn_off_gradient_print_mode(void) {
  write_command(16, &script_crc32);
  write(uint8_t(6), &script_crc32);
}

void FCodeGenerator::set_line_pixels(uint32_t pixel_number) {
  write_command(16, &script_crc32);
  write(uint8_t(2), &script_crc32);
  write(pixel_number, &script_crc32);
}

void FCodeGenerator::fill_32_pixels(uint32_t pixels) {
  write_command(16, &script_crc32);
  write(uint8_t(3), &script_crc32);
  write(pixels, &script_crc32);
}

void FCodeGenerator::set_fill_end(void) {
  write_command(16, &script_crc32);
  write(uint8_t(4), &script_crc32);
}

void FCodeGenerator::set_print_line_status(void) {
  write_command(16, &script_crc32);
  write(uint8_t(5), &script_crc32);
}

void FCodeGenerator::enter_printer_mode(void) {
  write_command(18, &script_crc32);
  write(uint8_t(0), &script_crc32);
  write(uint32_t(1), &script_crc32);
}
// for white ink and varnish, use m137_cmd_type1 with f=0, s=0.2646 (96DPI pixel
// size)

void FCodeGenerator::wait_printer_mode_sync(void) {
  write_command(18, &script_crc32);
  write(uint8_t(0), &script_crc32);
  write(uint32_t(0), &script_crc32);
}

void FCodeGenerator::exit_printer_mode(void) {
  write_command(18, &script_crc32);
  write(uint8_t(0), &script_crc32);
  write(uint32_t(2), &script_crc32);
}

void FCodeGenerator::set_printer_packet_length(uint32_t length, bool is_4c) {
  write_command(17, &script_crc32);
  write(uint8_t(is_4c ? 11 : 0), &script_crc32);
  write(length, &script_crc32);
}

void FCodeGenerator::start_printer_packet_payload(bool is_4c) {
  write_command(17, &script_crc32);
  write(uint8_t(is_4c ? 12 : 1), &script_crc32);
}

void FCodeGenerator::add_printer_packet_payload(uint8_t byte) {
  write(byte, &script_crc32);
}

void FCodeGenerator::set_printer_packet_crc(uint16_t val, bool is_4c) {
  write_command(17, &script_crc32);
  write(uint8_t(is_4c ? 13 : 2), &script_crc32);
  write(val, &script_crc32);
}

// 5 for white ink, 6 for varnish
void FCodeGenerator::start_printer_packet(uint8_t packet_type, bool is_4c) {
  write_command(17, &script_crc32);
  write(uint8_t(is_4c ? 10 : 3), &script_crc32);
  write(packet_type, &script_crc32);
}

void FCodeGenerator::end_printer_packet(bool is_4c) {
  write_command(17, &script_crc32);
  write(uint8_t(is_4c ? 14 : 4), &script_crc32);
}

void FCodeGenerator::set_printer_packet_px_count(uint32_t count) {
  write_command(17, &script_crc32);
  write(uint8_t(5), &script_crc32);
  write(count, &script_crc32);
}

void FCodeGenerator::sync_grbl_motion(uint32_t val) {
  write_command(18, &script_crc32);
  write(uint8_t(0), &script_crc32);
  write(val, &script_crc32);
}

void FCodeGenerator::sync_motion_type2(uint32_t cmd, int flags, float q) {
  write_command(18, &script_crc32);
  write(uint8_t(2), &script_crc32);
  write(cmd, &script_crc32);
  if (flags > 0)
    write_command(flags, &script_crc32);
  if (flags & FLAG_Q)
    write(q, &script_crc32);
}

void FCodeGenerator::m137_cmd_type1(uint32_t cmd,
                                    int flags,
                                    float f,
                                    float x,
                                    float y,
                                    float z,
                                    float a,
                                    float s) {
  write_command(18, &script_crc32);
  write(uint8_t(1), &script_crc32);
  write(cmd, &script_crc32);
  if (flags > 0) {
    write_command(flags, &script_crc32);
  }
  if (flags & FLAG_F) {
    write(f, &script_crc32);
  }
  if (flags & FLAG_X) {
    write(x, &script_crc32);
  }
  if (flags & FLAG_Y) {
    write(y, &script_crc32);
  }
  if (flags & FLAG_Z) {
    write(z, &script_crc32);
  }
  if (flags & FLAG_A) {
    write(a, &script_crc32);
  }
  if (flags & FLAG_S) {
    write(s, &script_crc32);
  }
}

void FCodeGenerator::flux_custom_cmd(uint32_t val) {
  write_command(19, &script_crc32);
  write(uint8_t(0), &script_crc32);
  write(val, &script_crc32);
}

void FCodeGenerator::one_seg_custom_cmd(int type, uint8_t cmd) {
  write_command(type, &script_crc32);
  write(cmd, &script_crc32);
}

// ==================== FCodeGeneratorV1 ====================
FCodeGeneratorV1::FCodeGeneratorV1(const QString* canvas_thumbnail)
    : thumbnail(canvas_thumbnail) {
  qInfo() << "FCodeGenerator V1 init";
  stream = new std::stringstream();
  created_at = QDateTime::currentDateTime();
  current_feedrate = last_feedrate = 0;
  last_direction = 0;
  last_acc = 0;
  current_x = current_y = current_z = 0;
  traveled = time_cost = 0;
  script_crc32 = 0;

  write_magic_number(1);
  script_offset = stream->tellp();
  if (script_offset < 0) {
    throw std::runtime_error("NOT_SUPPORT STREAM");
  }
  // Keep space for script size
  write("\x00\x00\x00\x00", 4, NULL);
}

FCodeGeneratorV1::~FCodeGeneratorV1() {
  delete stream;
}

void FCodeGeneratorV1::write(const char* buf,
                             size_t size,
                             unsigned long* crc32_ptr) {
  stream->write(buf, size);
  if (crc32_ptr) {
    *crc32_ptr = crc32(*crc32_ptr, (const void*)buf, size);
  }
}

void FCodeGeneratorV1::write(const char* buf,
                             size_t size,
                             unsigned long* crc32_ptr,
                             bool to_all) {
  write(buf, size, crc32_ptr);
}

void FCodeGeneratorV1::moveto(int flags,
                              float feedrate,
                              float x,
                              float y,
                              float z,
                              float a,
                              float s) {
  if (flags & FCodeGenerator::FLAG_F && feedrate > 0) {
    current_feedrate = feedrate / 60;
  }

  bool has_move = false;
  float mv[3] = {0, 0, 0};

  if (flags & FCodeGenerator::FLAG_X) {
    mv[0] = x - current_x;
    current_x = x;
    has_move = true;
  }
  if (flags & FCodeGenerator::FLAG_Y) {
    mv[1] = y - current_y;
    current_y = y;
    has_move = true;
  } else if (flags & FCodeGenerator::FLAG_A) {
    mv[1] = a - current_y;
    current_y = a;
    has_move = true;
  }
  if (flags & FCodeGenerator::FLAG_Z) {
    if (z < 0) {
      current_z = 0;
    } else {
      mv[2] = z - current_z;
      current_z = z;
      has_move = true;
    }
  }

  if (has_move) {
    double dist = sqrt(pow(mv[0], 2) + pow(mv[1], 2));
    if (!isnan(dist) && dist > 0) {
      traveled += dist;
      if (current_feedrate > 0) {
        float direction = atan2(mv[1], mv[0]);
        float last_vel_t =
            (last_feedrate *
             cos(direction - last_direction));  // consider direction
        float last_vel_n = abs(last_feedrate * sin(direction - last_direction));
        float acc = (abs(mv[0]) / acc_x) > (abs(mv[1]) / acc_y) ? acc_x : acc_y;
        if (last_acc == 0)
          last_acc = acc;
        float vel = estimate_vel(last_vel_t, current_feedrate, acc,
                                 dist);  // consider short distance
        float tc = estimate_time(last_feedrate, last_vel_n, last_vel_t, vel,
                                 last_acc, acc, dist);
        if (!isnan(tc))
          time_cost += tc;
        last_feedrate = vel;
        last_direction = direction;
        last_acc = acc;
      }
    } else if (abs(mv[2]) > 0) {
      float dist = abs(mv[2]);
      traveled += dist;
      float tc = (dist / z_speed);
      if (!isnan(tc)) {
        time_cost += tc;
      }
    }
  }
  FCodeGenerator::moveto(flags, feedrate, x, y, z, a, s);
}

void FCodeGeneratorV1::home(void) {
  current_x = current_y = current_z = 0;
  FCodeGenerator::home();
}

void FCodeGeneratorV1::sleep(float seconds) {
  if (!isnan(seconds)) {
    time_cost += seconds;
  }
  FCodeGenerator::sleep(seconds);
}

void FCodeGeneratorV1::write_metadata_(QString key,
                                       QString value,
                                       unsigned long* crc_ptr) {
  metadata.insert(key, value);
  write(key.toStdString().c_str(), key.size(), crc_ptr);
  write("=", 1, crc_ptr);
  write(value.toStdString().c_str(), value.size(), crc_ptr);
  write("\x00", 1, crc_ptr);
}

unsigned long FCodeGeneratorV1::write_metadata() {
  unsigned long crc_val = 0;
  // Write metadata from exporter first
  for (auto it = metadata.begin(); it != metadata.end(); ++it) {
    write_metadata_(it.key(), it.value().toString(), &crc_val);
  }
  write_metadata_("VERSION", "1", &crc_val);
  write_metadata_("HEAD_TYPE", "LASER", &crc_val);
  write_metadata_("TIME_COST", QString::number(time_cost, 'f', 2), &crc_val);
  write_metadata_("TRAVEL_DIST", QString::number(traveled, 'f', 2), &crc_val);
  write_metadata_("CREATED_AT", created_at.toString(time_format), &crc_val);
  write_metadata_("SOFTWARE", sw_version, &crc_val);
  return crc_val;
}

std::string FCodeGeneratorV1::to_string() {
  return ((std::stringstream*)stream)->str();
};

size_t FCodeGeneratorV1::total_length() {
  return int(stream->tellp()) - script_offset + magic_number_size;
}

float FCodeGeneratorV1::get_time_cost() {
  return time_cost;
}

void FCodeGeneratorV1::terminated() {
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
  thumbnail_image.loadFromData(QByteArray::fromBase64(
      thumbnail->toLatin1().mid(thumbnail->indexOf(",") + 1)));
  thumbnail_image.save(&buffer, "PNG");
  u32value = thumbnail_data.size();
  FCodeGenerator::write(u32value, NULL);
  write(thumbnail_data.data(), u32value, NULL);
  write("\x00\x00\x00\x00", 4, NULL);
}

// ==================== FCodeGeneratorV2 ====================
FCodeGeneratorV2::FCodeGeneratorV2(const QString* canvas_thumbnail,
                                   int magic_number)
    : thumbnail(canvas_thumbnail) {
  qInfo() << "FCodeGenerator V2 init";
  created_at = QDateTime::currentDateTime();
  fc_stream = new std::stringstream();
  content_stream = new std::stringstream();
  current_feedrate = last_feedrate = 0;
  last_direction = 0;
  last_acc = 0;
  current_x = current_y = current_z = 0;
  traveled = time_cost = 0;
  script_crc32 = 0;

  start = fc_stream->tellp();
  write_magic_number(magic_number);
}

FCodeGeneratorV2::~FCodeGeneratorV2() {
  delete fc_stream;
  delete content_stream;
}

void FCodeGeneratorV2::write(const char* buf,
                             size_t size,
                             unsigned long* crc32_ptr) {
  // Ignore crc32_ptr; will be calculated when copying to fc_stream
  content_stream->write(buf, size);
}

void FCodeGeneratorV2::write_to_all(const char* buf,
                                    size_t size,
                                    unsigned long* crc32_ptr) {
  fc_stream->write(buf, size);
  if (crc32_ptr) {
    *crc32_ptr = crc32(*crc32_ptr, (const void*)buf, size);
  }
}

void FCodeGeneratorV2::write(const char* buf,
                             size_t size,
                             unsigned long* crc32_ptr,
                             bool to_all) {
  if (to_all) {
    write_to_all(buf, size, crc32_ptr);
  } else {
    write(buf, size, crc32_ptr);
  }
}

std::string FCodeGeneratorV2::to_string() {
  return ((std::stringstream*)fc_stream)->str();
};

size_t FCodeGeneratorV2::total_length() {
  return int(fc_stream->tellp()) - start;
};

float FCodeGeneratorV2::get_time_cost() {
  return time_cost;
}

void FCodeGeneratorV2::write_string(const char* s,
                                    size_t length,
                                    bool write_length) {
  if (write_length) {
    FCodeGenerator::write((uint32_t)length, &script_crc32);
  }
  write(s, length, &script_crc32);
}

void FCodeGeneratorV2::start_task_script_block(const char* header,
                                               const char* proc_id) {
  write(header, 4, NULL);
  if (proc_id) {
    write(proc_id, 4, NULL);
  }
  current_task_script_start = content_stream->tellp();
  current_task_traveled = traveled;
  current_task_time_cost = time_cost;
  write("\x00\x00\x00\x00", 4, NULL);
}

void FCodeGeneratorV2::end_task_script_block(void) {
  int end = content_stream->tellp();
  content_stream->seekp(current_task_script_start, content_stream->beg);
  uint32_t task_length = end - current_task_script_start - 4;
  FCodeGenerator::write(task_length, NULL);
  content_stream->seekp(end, content_stream->beg);
  current_task_traveled = traveled - current_task_traveled;
  current_task_time_cost = time_cost - current_task_time_cost;
}

void FCodeGeneratorV2::write_task_info(QJsonObject task_info) {
  task_info.insert("time_cost", round(current_task_time_cost * 100) / 100);
  task_info.insert("travel_dist", round(current_task_traveled * 100) / 100);
  QJsonDocument doc(task_info);
  QString str(doc.toJson(QJsonDocument::Compact));
  str.replace("\\\\", "\\");
  write_string("INFO", 4);
  write_string(str.toStdString().c_str(), str.size(), true);
}

void FCodeGeneratorV2::moveto(int flags,
                              float feedrate,
                              float x,
                              float y,
                              float z,
                              float a,
                              float s) {
  if (flags & FLAG_F && feedrate > 0) {
    current_feedrate = feedrate / 60;
  }

  bool has_move = false;
  float mv[3] = {0, 0, 0};

  if (flags & FLAG_X) {
    mv[0] = x - current_x;
    current_x = x;
    has_move = true;
  }
  if (flags & FLAG_Y) {
    mv[1] = y - current_y;
    current_y = y;
    has_move = true;
  } else if (flags & FLAG_A) {
    mv[1] = a - current_y;
    current_y = a;
    has_move = true;
  }
  if (flags & FLAG_Z) {
    if (z < 0) {
      // Autofocus Homing
      current_z = 0;
    } else {
      mv[2] = z - current_z;
      current_z = z;
      has_move = true;
    }
  }

  if (has_move) {
    float time_est = 0;
    if (abs(mv[2]) > 0) {
      float dist = abs(mv[2]);
      traveled += dist;
      float res = (dist / z_speed);
      if (!isnan(res) && res > 0) {
        time_est = res;
      }
    }
    double dist = sqrt(pow(mv[0], 2) + pow(mv[1], 2));
    if (!isnan(dist) && dist > 0) {
      traveled += dist;
      if (current_feedrate > 0) {
        float direction = atan2(mv[1], mv[0]);
        float last_vel_t = (last_feedrate * cos(direction - last_direction));
        float last_vel_n = abs(last_feedrate * sin(direction - last_direction));
        float acc = (abs(mv[0]) / acc_x) > (abs(mv[1]) / acc_y) ? acc_x : acc_y;
        if (last_acc == 0)
          last_acc = acc;
        float vel = estimate_vel(last_vel_t, current_feedrate, acc, dist);
        float res = estimate_time(last_feedrate, last_vel_n, last_vel_t, vel,
                                  last_acc, acc, dist);
        if (!isnan(res) && res > time_est)
          time_est = res;
        last_feedrate = vel;
        last_direction = direction;
        last_acc = acc;
      }
    }
    if (time_est > 0) {
      time_cost += time_est;
    }
  }
  FCodeGenerator::moveto(flags, feedrate, x, y, z, a, s);
}

void FCodeGeneratorV2::home(void) {
  current_x = current_y = current_z = 0;
  FCodeGenerator::home();
}

void FCodeGeneratorV2::sleep(float seconds) {
  if (!isnan(seconds)) {
    time_cost += seconds;
  }
  FCodeGenerator::sleep(seconds);
}

void FCodeGeneratorV2::write_metadata_(QString key,
                                       QString value,
                                       unsigned long* crc32_ptr,
                                       bool is_string,
                                       bool has_next) {
  metadata.insert(key, value);
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

unsigned long FCodeGeneratorV2::write_metadata() {
  unsigned long crc_val = 0;
  write_to_all("{", 1, &crc_val);
  // Write metadata from exporter first
  for (auto it = metadata.begin(); it != metadata.end(); ++it) {
    // Note: REQUIRED_HEADTYPE and FORBIDDEN_HEADTYPE must be number without
    // quotes
    write_metadata_(it.key(), it.value().toString(), &crc_val,
                    !it.value().isDouble());
  }
  write_metadata_("version", "2", &crc_val);
  write_metadata_("CREATED_AT", created_at.toString(time_format), &crc_val);
  write_metadata_("SOFTWARE", sw_version, &crc_val);
  write_metadata_("travel_dist", QString::number(traveled, 'f', 2), &crc_val,
                  false);
  write_metadata_("time_cost", QString::number(time_cost, 'f', 2), &crc_val,
                  false, false);
  write_to_all("}", 1, &crc_val);
  return crc_val;
}

void FCodeGeneratorV2::end_content() {
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
  thumbnail_image.loadFromData(QByteArray::fromBase64(
      thumbnail->toLatin1().mid(thumbnail->indexOf(",") + 1)));
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

void FCodeGeneratorV2::write_post_config(const char* s, size_t length) {
  unsigned long post_config_crc32 = 0;
  write_to_all("POST", 4, NULL);
  FCodeGenerator::write((uint32_t)length, NULL, true);
  write_to_all(s, length, &post_config_crc32);
  FCodeGenerator::write((uint32_t)post_config_crc32, NULL, true);
}

// ==================== FCodeGeneratorG ====================
FCodeGeneratorG::FCodeGeneratorG() {
  qInfo() << "FCodeGenerator for Gcode init";
  script_offset = str_stream.tellp();
  if (script_offset < 0) {
    throw std::runtime_error("NOT_SUPPORT STREAM");
  }
}

FCodeGeneratorG::~FCodeGeneratorG() {}

void FCodeGeneratorG::write(const char* buf,
                            size_t size,
                            unsigned long* crc32_ptr) {}

void FCodeGeneratorG::write(const char* buf,
                            size_t size,
                            unsigned long* crc32_ptr,
                            bool to_all) {}

void FCodeGeneratorG::moveto(int flags,
                             float feedrate,
                             float x,
                             float y,
                             float z,
                             float a,
                             float s) {
  str_stream << "G1";
  if (flags & FCodeGenerator::FLAG_F && feedrate > 0) {
    str_stream << " F"
               << std::round(feedrate * move_precision) / move_precision;
  }
  if (flags & FCodeGenerator::FLAG_X) {
    str_stream << " X" << std::round(x * move_precision) / move_precision;
  }
  if (flags & FCodeGenerator::FLAG_Y) {
    str_stream << " Y" << std::round(y * move_precision) / move_precision;
  }
  if (flags & FCodeGenerator::FLAG_Z) {
    str_stream << " Z" << std::round(z * move_precision) / move_precision;
  }
  // if (flags & FCodeGenerator::FLAG_A) {
  //   str_stream << " Y" << std::round(a * move_precision) / move_precision;
  // }
  if (flags & FCodeGenerator::FLAG_S) {
    str_stream << " S" << std::round(s * move_precision) / move_precision;
  }
  str_stream << "\n";
}

void FCodeGeneratorG::home(void) {
  str_stream << "$H\n";
}

void FCodeGeneratorG::set_toolhead_pwm(float strength) {
  if (strength < 0) {
    str_stream << "G1 U" << (int)(strength * -1000) << "\n";
  } else if (strength < 0.001) {
    str_stream << "G1S0\n";
  } else {
    str_stream << "G1V0\n";
  }
}

std::string FCodeGeneratorG::to_string() {
  return str_stream.str();
};

size_t FCodeGeneratorG::total_length() {
  return int(str_stream.tellp()) - script_offset;
}

float FCodeGeneratorG::get_time_cost() {
  return 0;
}

// ==================== ToolpathProcessor ====================
void ToolpathProcessor::init(int magic_number, const QString* thumbnail) {
  qInfo() << "ToolpathProcessor init" << magic_number;
  update_moveto_pipeline();
  if (magic_number == -1) {
    gen = std::make_shared<FCodeGeneratorG>();
  } else if (magic_number > 1) {
    support_a_mode_ = true;
    gen = std::make_shared<FCodeGeneratorV2>(thumbnail, magic_number);
  } else {
    gen = std::make_shared<FCodeGeneratorV1>(thumbnail);
  }
  gen_ = gen.get();
}

void ToolpathProcessor::clear_curve_engraving_data() {
  curve_engraving_data = nullptr;
  update_moveto_pipeline();
}

bool ToolpathProcessor::set_curve_engraving_data(const QJsonObject& curve_obj,
                                                 const QPointF& job_origin,
                                                 QSizeF work_area_size,
                                                 InwardRect& workarea_clip) {
  if (curve_obj.isEmpty()) {
    clear_curve_engraving_data();
    return false;
  }
  qInfo() << "Set curve engraving data";
  QJsonObject bbox = curve_obj["bbox"].toObject();
  float left = bbox["x"].toDouble();
  workarea_clip.left = qMax(workarea_clip.left, left);
  float top = bbox["y"].toDouble();
  workarea_clip.top = qMax(workarea_clip.top, top);
  float width = bbox["width"].toDouble();
  float right_bound = left + width;
  float right = work_area_size.width() - right_bound;
  workarea_clip.right = qMax(workarea_clip.right, right);
  float height = bbox["height"].toDouble();
  float bottom_bound = top + height;
  float bottom = work_area_size.height() - bottom_bound;
  workarea_clip.bottom = qMax(workarea_clip.bottom, bottom);

  left -= job_origin.x();
  right -= job_origin.x();
  top -= job_origin.y();
  bottom -= job_origin.y();
  QJsonArray points = curve_obj["points"].toArray();
  int point_size = points.size();
  if (point_size >= 3) {
    curve_engraving_data = std::make_unique<CurveEngravingData>();
    // add 0.01 to avoid clipping the boundary
    float space = 0.01;
    curve_engraving_data->bbox = QRectF(left - space, top - space, width + 2 * space, height + 2 * space);
    curve_engraving_data->interpolator.set_bounding_box(left, top, right, bottom);
    QJsonArray gap = curve_obj["gap"].toArray();
    curve_engraving_data->gap = QPointF(gap[0].toDouble(), gap[1].toDouble());

    for (int i = 0; i < point_size; i++) {
      QJsonArray point = points[i].toArray();
      float x = point[0].toDouble();
      float y = point[1].toDouble();
      float z = point[2].toDouble();
      x -= job_origin.x();
      y -= job_origin.y();
      curve_engraving_data->interpolator.add_point(x, y, z);
    }
    curve_engraving_data->interpolator.setup();
    curve_engraving_data->safe_height = curve_obj["safe_height"].toDouble(NAN);
    return true;
  }
  return false;
}

void ToolpathProcessor::set_curve_engraving_data_by_key(QString& key,
                                                        double value) {
  if (!curve_engraving_data) {
    return;
  }
  if (key == "z_speed_limit") {
    curve_engraving_data->z_speed_limit = value;
  }
  update_moveto_pipeline();
}

void ToolpathProcessor::set_z_premove(ZPremoveData data) {
  z_premove_ = data;
  update_moveto_pipeline();
}

void ToolpathProcessor::set_a_mode(bool a_mode) {
  is_a_mode_ = a_mode;
}

void ToolpathProcessor::set_is_main_task(bool is_main_task) {
  is_main_task_ = is_main_task;
}

void ToolpathProcessor::set_rotary_axis(float rotary_y) {
  rotary_y_ = rotary_y;
}

void ToolpathProcessor::set_rotary_y_ratio(float ratio) {
  rotary_y_ratio_ = ratio;
}

void ToolpathProcessor::set_travel_speed(float feedrate, bool a_axis) {
  if (isnan(feedrate)) {
    return;
  }
  if (a_axis) {
    a_travel_speed_ = feedrate;
  } else {
    travel_speed_ = feedrate;
  }
}

float ToolpathProcessor::get_travel_speed(bool a_axis) {
  if (a_axis) {
    return a_travel_speed_;
  } else {
    return travel_speed_;
  }
}

void ToolpathProcessor::set_rotary_wait_move(bool wait, float y) {
  rotary_wait_move_ = wait;
  rotary_y_offset_ = y;
}

void ToolpathProcessor::update_moveto_pipeline() {
  moveto_pipeline_functions_.clear();
  moveto_pipeline_functions_.push_back(&ToolpathProcessor::rotary_motion_generator);
  if (curve_engraving_data) {
    moveto_pipeline_functions_.push_back(&ToolpathProcessor::curve_engraving_motion_generator);
    if (z_premove_.is_valid) {
      float z_speed_limit = curve_engraving_data->z_speed_limit;
      if (z_speed_limit == 0 || z_speed_limit > z_premove_.speed) {
        moveto_pipeline_functions_.push_back(&ToolpathProcessor::z_premove_motion_generator);
      }
    }
  }
}

void ToolpathProcessor::pipeline_moveto(int idx, NamedArgs args) {
  if (idx >= moveto_pipeline_functions_.size()) {
    _moveto(args);
    return;
  }
  MoveCallback callback = [this, idx](NamedArgs args) {
    pipeline_moveto(idx + 1, args);
  };
  (this->*moveto_pipeline_functions_[idx])(args, callback);
}

void ToolpathProcessor::moveto(float feedrate,
                               float x,
                               float y,
                               float z,
                               float a,
                               float s,
                               bool force_y,
                               bool is_travel) {
  if (is_travel && isnan(feedrate)) {
    feedrate = travel_speed_;
  }
  pipeline_moveto(0, {feedrate, x, y, z, a, s, force_y, is_travel});
}

void ToolpathProcessor::moveto(NamedArgs args) {
  if (args.is_travel && isnan(args.f)) {
    args.f = travel_speed_;
  }
  pipeline_moveto(0, args);
}

void ToolpathProcessor::rotary_motion_generator(NamedArgs args,
                                                MoveCallback callback) {
  // fcode v2 rotary
  bool use_a = support_a_mode_ && is_a_mode_ && !args.force_y;
  // apply rotary y ratio
  if (!isnan(args.y) && rotary_y_ratio_ != 1) {
    // for fcode v1: check rotary_enabled_
    // for fcode v2: check use_a
    if (support_a_mode_ ? use_a : rotary_enabled_) {
      args.y = rotary_y_ + (args.y - rotary_y_) * rotary_y_ratio_;
    }
  }
  if (!use_a) {
    callback(args);
    return;
  }
  // a-axis rotary
  if (isnan(args.a)) {
    args.a = args.y;
  }
  args.y = NAN;
  args.force_y = false;
  if (rotary_wait_move_) {
    callback({args.f, args.x, NAN, args.z, NAN, args.s, args.force_y,
              args.is_travel});
    args.x = NAN;
    args.z = NAN;
    if (!rotary_enabled_) {
      callback({args.f, NAN, rotary_y_, NAN, NAN, NAN, args.force_y, args.is_travel});
      pause(false);
    }
    callback({args.f, NAN, rotary_y_offset_, NAN, NAN, NAN, args.force_y,
              args.is_travel});
    sync_motion_type2(185, 0.0);
    sync_motion_type2(179, 2.0);
    rotary_wait_move_ = false;
  }
  if (args.is_travel) {
    callback({a_travel_speed_, NAN, NAN, NAN, args.a, NAN, args.force_y, args.is_travel});
    args.a = NAN;
  }
  callback(args);
}

void ToolpathProcessor::curve_engraving_motion_generator(
    NamedArgs args,
    MoveCallback callback) {
  if (!isnan(args.f)) {
    curve_engraving_data->target_feedrate = args.f;
  }
  if ((isnan(args.x) && isnan(args.y)) || !isnan(args.z)) {
    callback(args);
    return;
  }
  if (!curve_engraving_data->started) {
    if (args.is_travel) {
      callback(args);
      return;
    }
    curve_engraving_data->started = true;
  }

  float box_left = curve_engraving_data->bbox.left();
  float box_right = curve_engraving_data->bbox.right();
  float box_top = curve_engraving_data->bbox.top();
  float box_bottom = curve_engraving_data->bbox.bottom();
  float start_x = cur_x_, start_y = cur_y_;
  float cur_x = start_x, cur_y = start_y, cur_z = cur_z_;
  float dist_x = fabs(isnan(args.x) ? 0 : (args.x - start_x));
  float dist_y = fabs(isnan(args.y) ? 0 : (args.y - start_y));
  float target_feedrate = curve_engraving_data->target_feedrate;
  float z_speed_limit = curve_engraving_data->z_speed_limit;
  int seg_counts = qMax(qMax(int(ceil(2 * dist_x / curve_engraving_data->gap.x())),
                             int(ceil(2 * dist_y / curve_engraving_data->gap.y()))),
                        1);
  float step_x, step_y, dx, dy, dz, dxy, feedrate_z, scale, ratio;
  for (int i = 0; i < seg_counts; ++i) {
    if (i < seg_counts - 1) {
      ratio = (i + 1.0) / seg_counts;
      step_x = isnan(args.x) ? NAN : (start_x + (args.x - start_x) * ratio);
      step_y = isnan(args.y) ? NAN : (start_y + (args.y - start_y) * ratio);
    } else {
      step_x = args.x;
      step_y = args.y;
    }
    dx = isnan(step_x) ? 0 : (step_x - cur_x);
    dy = isnan(step_y) ? 0 : (step_y - cur_y);
    cur_x = isnan(step_x) ? cur_x : step_x;
    cur_y = isnan(step_y) ? cur_y : step_y;
    if (cur_x < box_left || cur_x > box_right || cur_y < box_top ||
        cur_y > box_bottom) {
      callback({target_feedrate, step_x, step_y, args.z, args.a, args.s, args.force_y, args.is_travel});
      continue;
    }
    args.z = qMax(curve_engraving_data->interpolator.do_evaluate(cur_x, cur_y), 0.0);
    dxy = sqrt(dx * dx + dy * dy);
    if (target_feedrate && !isnan(cur_z) && dxy > 0) {
      dz = fabs(args.z - cur_z);
      feedrate_z = target_feedrate * dz / dxy;
      if (feedrate_z > 0) {
        if (z_speed_limit > 0) {
          scale = qMin(1.0, z_speed_limit / feedrate_z);
        } else {
          scale = 1;
        }
        args.f = sqrt(target_feedrate * target_feedrate + feedrate_z * feedrate_z) * scale;
      }
    }
    cur_z = args.z;
    callback({args.f, step_x, step_y, args.z, args.a, args.s, args.force_y,
              args.is_travel});
  }
  if (target_feedrate != args.f) {
    callback({target_feedrate, NAN, NAN, NAN, NAN, NAN, args.force_y, args.is_travel});
  }
}

void ToolpathProcessor::z_premove_motion_generator(NamedArgs args,
                                                   MoveCallback callback) {
  if (isnan(args.z)) {
    callback(args);
    return;
  }
  if (isnan(cur_z_)) {
    sync_grbl_motion(0);
    callback(args);
    return;
  }
  float orig_feedrate = isnan(args.f) ? cur_f_ : args.f;
  float dz = args.z - cur_z_;
  if (fabs(dz) > 0) {
    float min_premove_x = z_premove_.x;
    float min_premove_y = z_premove_.y;
    float min_premove_z = z_premove_.z;
    float premove_feedrate = z_premove_.speed;
    float dx = isnan(args.x) ? 0 : (args.x - cur_x_);
    float dy = isnan(args.y) ? 0 : (args.y - cur_y_);
    float dxy = sqrt(dx * dx + dy * dy);
    float orig_feedrate_z = orig_feedrate * fabs(dz) / sqrt(dxy * dxy + dz * dz);
    if (orig_feedrate_z > premove_feedrate) {
      float scale = min_premove_z / fabs(dz);
      if (fabs(dx) > 0) {
        scale = qMax(scale, min_premove_x / fabs(dx));
      }
      if (fabs(dy) > 0) {
        scale = qMax(scale, min_premove_y / fabs(dy));
      }
      scale = qMin(scale, 1.0);
      float premove_x = isnan(args.x) ? NAN : (cur_x_ + dx * scale);
      float premove_y = isnan(args.y) ? NAN : (cur_y_ + dy * scale);
      float premove_z = cur_z_ + dz * scale;
      callback({premove_feedrate, premove_x, premove_y, premove_z, NAN, NAN, args.force_y, args.is_travel});
      args.f = orig_feedrate;
    }
  }
  callback(args);
}

void ToolpathProcessor::_moveto(NamedArgs args) {
  int flags = 0;
  if (!isnan(args.f)) {
    flags |= FCodeGenerator::FLAG_F;
    cur_f_ = args.f;
  }
  if (!isnan(args.x)) {
    flags |= FCodeGenerator::FLAG_X;
    cur_x_ = args.x;
    if (is_main_task_) {
      min_x_ = std::isnan(min_x_) ? args.x : qMin(min_x_, args.x);
      max_x_ = std::isnan(max_x_) ? args.x : qMax(max_x_, args.x);
    }
  }
  if (!isnan(args.y)) {
    flags |= FCodeGenerator::FLAG_Y;
    cur_y_ = args.y;
    if (is_main_task_) {
      min_y_ = std::isnan(min_y_) ? args.y : qMin(min_y_, args.y);
      max_y_ = std::isnan(max_y_) ? args.y : qMax(max_y_, args.y);
    }
  }
  if (!isnan(args.z)) {
    flags |= FCodeGenerator::FLAG_Z;
    cur_z_ = args.z != -1 ? args.z : 1;  // z pos is 1 after homing
    if (is_main_task_) {
      min_z_ = std::isnan(min_z_) ? args.z : qMin(min_z_, args.z);
      max_z_ = std::isnan(max_z_) ? args.z : qMax(max_z_, args.z);
    }
  }
  if (!isnan(args.a)) {
    flags |= FCodeGenerator::FLAG_A;
    if (is_main_task_) {
      min_y_ = std::isnan(min_y_) ? args.a : qMin(min_y_, args.a);
      max_y_ = std::isnan(max_y_) ? args.a : qMax(max_y_, args.a);
    }
  }
  if (!isnan(args.s)) {
    flags |= FCodeGenerator::FLAG_S;
  }
  gen_->moveto(flags, args.f, args.x, args.y, args.z, args.a, args.s);
}

void ToolpathProcessor::pause(bool to_standby_position) {
  gen_->pause(to_standby_position);
  if (!to_standby_position) {
    rotary_enabled_ = true;
  }
}

void ToolpathProcessor::home() {
  gen_->home();
  cur_x_ = 0;
  cur_y_ = 0;
}

void ToolpathProcessor::m137_cmd_type1(unsigned cmd, NamedArgs args) {
  int flags = 0;
  if (!isnan(args.f)) {
    flags |= FCodeGenerator::FLAG_F;
  }
  if (!isnan(args.x)) {
    flags |= FCodeGenerator::FLAG_X;
  }
  if (!isnan(args.y)) {
    flags |= FCodeGenerator::FLAG_Y;
  }
  if (!isnan(args.z)) {
    flags |= FCodeGenerator::FLAG_Z;
  }
  if (!isnan(args.a)) {
    flags |= FCodeGenerator::FLAG_A;
  }
  if (!isnan(args.s)) {
    flags |= FCodeGenerator::FLAG_S;
  }
  if (flags > 0) {
    gen_->m137_cmd_type1(cmd, flags, args.f, args.x, args.y, args.z, args.a, args.s);
  }
}

void ToolpathProcessor::sync_motion_type2(unsigned cmd, float q) {
  int flags = 0;
  if (!std::isnan(q)) {
    flags |= FCodeGenerator::FLAG_Q;
  }
  gen_->sync_motion_type2(cmd, flags, q);
}

void ToolpathProcessor::set_acceleration_override(float x,
                                                  float y,
                                                  float z,
                                                  float a) {
  m137_cmd_type1(150, {NAN, x, y, z, a});
}

void ToolpathProcessor::user_selection_cmd(unsigned cmd) {
  gen_->one_seg_custom_cmd(20, cmd);
}

void ToolpathProcessor::miscellaneous_cmd(unsigned cmd) {
  gen_->one_seg_custom_cmd(21, cmd);
}

void ToolpathProcessor::grbl_system_cmd(unsigned cmd) {
  gen_->one_seg_custom_cmd(22, cmd);
}

void ToolpathProcessor::write_post_config(const QJsonArray& post_config) {
  QJsonDocument doc(post_config);
  QString str(doc.toJson(QJsonDocument::Compact));
  gen_->write_post_config(str.toStdString().c_str(), str.size());
}

void ToolpathProcessor::write_boundary_to_metadata() {
  if (!std::isnan(min_x_))
    gen_->add_metadata("min_x", QString::number(min_x_, 'f', 2));
  if (!std::isnan(min_y_))
    gen_->add_metadata("min_y", QString::number(min_y_, 'f', 2));
  if (!std::isnan(max_x_))
    gen_->add_metadata("max_x", QString::number(max_x_, 'f', 2));
  if (!std::isnan(max_y_))
    gen_->add_metadata("max_y", QString::number(max_y_, 'f', 2));
  if (!std::isnan(min_z_))
    gen_->add_metadata("min_z", QString::number(min_z_, 'f', 2));
  if (!std::isnan(max_z_))
    gen_->add_metadata("max_z", QString::number(max_z_, 'f', 2));
}

void ToolpathProcessor::write_printer_packet(int printer_packet_type,
                                             QByteArray payload,
                                             bool should_wait,
                                             bool is_4c) {
  gen_->start_printer_packet(printer_packet_type, is_4c);
  gen_->set_printer_packet_length(payload.size(), is_4c);
  gen_->start_printer_packet_payload(is_4c);
  for (auto b : payload) {
    gen_->add_printer_packet_payload(b);
  }
  gen_->set_printer_packet_crc(crc16(payload), is_4c);
  if (should_wait) {
    gen_->wait_printer_mode_sync();
  }
  gen_->end_printer_packet(is_4c);
}
