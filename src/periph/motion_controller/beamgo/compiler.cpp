#include "compiler.h"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace beamgo {
namespace {
constexpr size_t kMaxRecords = 10000000;
constexpr double kEpsilon = 0.00001;

class Compiler {
 public:
  explicit Compiler(const CompileOptions& options) : options_(options) {
    if (!std::isfinite(options.field_mm) || options.field_mm != 90 ||
        options.framing_power < 1 || options.framing_power > 100) {
      throw std::invalid_argument("INVALID_PARAMS");
    }
  }

  Bytes run(const std::string& gcode) {
    std::istringstream input(gcode);
    std::string line;
    while (std::getline(input, line)) parse(line);
    if (raster_pending_ || !raster_hex_.empty()) throw std::invalid_argument("Incomplete raster row");
    if (!started_) return {};
    append(options_.preview ? Bytes(5, 0xff) : record(0x1e));
    if (options_.preview && output_.size() / 5 > 25000) throw std::invalid_argument("Preview too large");
    return output_;
  }

 private:
  CompileOptions options_;
  Bytes output_;
  bool started_ = false;
  bool absolute_ = true;
  bool spindle_ = false;
  bool first_move_ = true;
  bool ended_ = false;
  double x_ = 0, y_ = 0, feed_ = 100, power_ = 0;
  double emitted_feed_ = -1, emitted_power_ = -1;
  double pixel_size_ = 0.1;
  size_t raster_count_ = 0;
  bool raster_pending_ = false;
  std::string raster_hex_;

  void append(const Bytes& value) {
    if (output_.size() / 5 >= kMaxRecords) throw std::invalid_argument("Job too large");
    output_.insert(output_.end(), value.begin(), value.end());
  }

  static double number(const std::string& value) {
    char* end = nullptr;
    const double result = std::strtod(value.c_str(), &end);
    if (end == value.c_str() || *end != '\0' || !std::isfinite(result)) {
      throw std::invalid_argument("Invalid numeric parameter");
    }
    return result;
  }

  void start() {
    if (started_) return;
    started_ = true;
    const double ratio = 65535.0 / options_.field_mm;
    if (!options_.preview) append(record(0x3c, 1));
    append(record32(options_.preview ? 0x13 : 0x05,
                    static_cast<uint32_t>(ratio * (options_.preview ? 1500 : feed_))));
    append(record32(0x08, options_.preview ? 100 : 50));
    append(record32(0x09, options_.preview ? 100 : 200));
    append(record32(0x0f, 1));
    append(record32(0x07, static_cast<uint32_t>(ratio * 4000)));
    append(record32(0x0c, 100));
    append(record32(0x0e, 1));
    append(record32(options_.preview ? 0x11 : 0x03, 10000));
    append(record32(0x0a, 200));
    append(record32(options_.preview ? 0x12 : 0x04,
                    options_.preview ? options_.framing_power * 10 : static_cast<uint32_t>(power_)));
    append(record32(0x0b, 100));
    if (!options_.preview) append(record(0x1d));
    emitted_feed_ = feed_;
    emitted_power_ = power_;
  }

  void move(double x, double y, bool mark) {
    if (!std::isfinite(x) || !std::isfinite(y) || x < -kEpsilon || y < -kEpsilon ||
        x > options_.field_mm + kEpsilon || y > options_.field_mm + kEpsilon) {
      throw std::invalid_argument("OUT_OF_BOUNDS");
    }
    if (mark && !options_.preview && (feed_ < 1 || feed_ > 2000)) {
      throw std::invalid_argument("Invalid marking speed");
    }
    start();
    if (!options_.preview) {
      if (feed_ != emitted_feed_) {
        append(record32(0x05, static_cast<uint32_t>(65535.0 / options_.field_mm * feed_)));
        emitted_feed_ = feed_;
      }
      if (power_ != emitted_power_) {
        append(record32(0x04, static_cast<uint32_t>(power_)));
        emitted_power_ = power_;
      }
    }
    const double device_x = options_.mirror_x ? options_.field_mm - x : x;
    const auto coordinate = [&](double value) {
      return static_cast<uint16_t>(std::clamp(value * 65535 / options_.field_mm, 0.0, 65535.0));
    };
    append(record(mark && !first_move_ ? 0x01 : 0x02, coordinate(device_x), coordinate(y)));
    first_move_ = false;
    x_ = x;
    y_ = y;
  }

  void raster(double final_x, double final_y) {
    if (std::abs(final_y - y_) > kEpsilon || raster_count_ == 0 ||
        raster_hex_.size() * 4 != raster_count_) throw std::invalid_argument("Invalid raster row");
    const double start_x = x_, row_y = y_;
    const double direction = final_x >= start_x ? 1 : -1;
    const double length = std::abs(final_x - start_x);
    if (length > raster_count_ * pixel_size_ + 0.002) throw std::invalid_argument("Short raster data");
    bool previous = false;
    double boundary = start_x;
    for (size_t bit = 0; bit < raster_count_ && bit * pixel_size_ < length; ++bit) {
      const char hex = raster_hex_[bit / 4];
      const unsigned nibble = hex <= '9' ? hex - '0' : hex - 'A' + 10;
      const bool mark = (nibble & (1u << (3 - bit % 4))) != 0;
      const double begin = start_x + direction * std::min(length, bit * pixel_size_);
      if (bit != 0 && mark != previous) move(begin, row_y, previous && spindle_ && power_ > 0);
      previous = mark;
      boundary = start_x + direction * std::min(length, (bit + 1) * pixel_size_);
    }
    move(boundary, row_y, previous && spindle_ && power_ > 0);
    if (std::abs(final_x - x_) > kEpsilon) move(final_x, final_y, false);
    raster_pending_ = false;
    raster_hex_.clear();
    raster_count_ = 0;
  }

  void parse(std::string line) {
    line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
    const auto first = line.find_first_not_of(" \t");
    if (first == std::string::npos) return;
    line.erase(0, first);
    if (line.rfind(";PIXEL SIZE ", 0) == 0) {
      pixel_size_ = number(line.substr(12));
      if (pixel_size_ <= 0 || pixel_size_ > 1) throw std::invalid_argument("Invalid pixel size");
      return;
    }
    if (line.rfind(";CONFIG INNER_ENGRAVING=", 0) == 0 && number(line.substr(24)) != 0) {
      throw std::invalid_argument("UNSUPPORTED: inner engraving");
    }
    const auto comment = line.find(';');
    if (comment != std::string::npos) line.resize(comment);
    while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) line.pop_back();
    if (line.empty()) return;
    if (ended_) throw std::invalid_argument("Commands after M2");
    // M100 is deliberately accepted here; it never reaches the USB transport.
    if (line == "M100") return;
    if (line.rfind("D0R", 0) == 0) {
      if (line.size() != 4 || std::string("LMHU").find(line[3]) == std::string::npos) {
        throw std::invalid_argument("Invalid raster resolution");
      }
      pixel_size_ = line[3] == 'L' ? 0.2 : line[3] == 'M' ? 0.1 : line[3] == 'H' ? 0.05 : 0.02;
      return;
    }
    if (line.rfind("D1PC", 0) == 0) {
      if (raster_pending_ || !raster_hex_.empty()) throw std::invalid_argument("Unfinished raster row");
      const double count = number(line.substr(4));
      if (count < 1 || count > 1000000 || count != std::floor(count)) throw std::invalid_argument("Invalid pixel count");
      raster_count_ = static_cast<size_t>(count);
      return;
    }
    if (line.rfind("D2W", 0) == 0) {
      const auto data = line.substr(3);
      if (data.empty() || data.find_first_not_of("0123456789ABCDEF") != std::string::npos ||
          (raster_hex_.size() + data.size()) * 4 > raster_count_) throw std::invalid_argument("Invalid raster data");
      raster_hex_ += data;
      return;
    }
    if (line == "D3FE") {
      if (raster_hex_.size() * 4 != raster_count_ || raster_count_ == 0) throw std::invalid_argument("Short raster data");
      return;
    }
    if (line == "D4PL") {
      if (raster_pending_ || raster_count_ == 0 || raster_hex_.size() * 4 != raster_count_) throw std::invalid_argument("Invalid raster row");
      raster_pending_ = true;
      return;
    }
    if (line == "D5") {
      if (raster_pending_ || !raster_hex_.empty()) throw std::invalid_argument("Incomplete raster row");
      return;
    }

    std::vector<std::pair<std::string, double>> words;
    for (size_t offset = 0; offset < line.size();) {
      if (std::isspace(static_cast<unsigned char>(line[offset]))) { ++offset; continue; }
      std::string word(1, line[offset++]);
      if (word == "W" && offset < line.size() && (line[offset] == 'S' || line[offset] == 'D')) word += line[offset++];
      const size_t begin = offset;
      if (offset < line.size() && (line[offset] == '-' || line[offset] == '+')) ++offset;
      while (offset < line.size() && (std::isdigit(static_cast<unsigned char>(line[offset])) || line[offset] == '.')) ++offset;
      words.emplace_back(word, number(line.substr(begin, offset - begin)));
    }
    double next_x = x_, next_y = y_;
    bool has_motion = false, rapid = false;
    // Resolve modal changes before coordinates, independent of word ordering.
    for (const auto& [word, value] : words) {
      if (word == "G") {
        if (value == 90) absolute_ = true;
        else if (value == 91) absolute_ = false;
        else if (value == 0) rapid = true;
        else if (value != 1) throw std::invalid_argument("UNSUPPORTED: G command");
      } else if (word == "M") {
        if (value == 3 || value == 4) { spindle_ = true; power_ = 0; first_move_ = true; }
        else if (value == 5) { spindle_ = false; power_ = 0; }
        else if (value == 2) ended_ = true;
        else if (value != 100 && value != 103 && value != 104) throw std::invalid_argument("UNSUPPORTED: M command");
      }
    }
    for (const auto& [word, value] : words) {
      if (word == "X") { next_x = value + (absolute_ ? 0 : x_); has_motion = true; }
      else if (word == "Y") { next_y = value + (absolute_ ? 0 : y_); has_motion = true; }
      else if (word == "F") {
        feed_ = value / 60;
        if (feed_ <= 0 || feed_ > 10000) throw std::invalid_argument("Invalid feed rate");
      } else if (word == "S") {
        if (value < 0 || value > 1000) throw std::invalid_argument("Invalid power");
        power_ = value;
      } else if (word == "Q") {
        if (value != 10 && !options_.preview) throw std::invalid_argument("UNSUPPORTED: frequency");
      } else if (word == "P" || word == "T" || word == "W") {
        // P: no MOPA pulse width. T: phase-one raster is lines, not dots. W: device owns field.
      } else if (word != "M" && word != "G") {
        throw std::invalid_argument("UNSUPPORTED: " + word);
      }
    }
    if (has_motion) {
      if (raster_pending_) raster(next_x, next_y);
      else move(next_x, next_y, !rapid && spindle_ && (options_.preview || power_ > 0));
    }
  }
};
}  // namespace

Bytes compile(const std::string& gcode, const CompileOptions& options) {
  return Compiler(options).run(gcode);
}
}  // namespace beamgo
