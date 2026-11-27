#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace beamgo {

using Bytes = std::vector<uint8_t>;

// USB uses fixed directional keys; the BLE name-derived key is not applicable.
uint16_t crc16(const Bytes& bytes);
Bytes encodeFrame(const Bytes& plaintext);

// Decoder owns one receive stream. Invalid frames are discarded one byte at a time,
// preserving valid replies/events behind noise or a damaged frame.
class FrameDecoder {
 public:
  std::vector<Bytes> push(const Bytes& bytes);
  void reset();
  size_t errors() const { return errors_; }

 private:
  Bytes buffer_;
  size_t errors_ = 0;
};

Bytes record(uint8_t opcode, uint16_t a = 0, uint16_t b = 0);
Bytes record32(uint8_t opcode, uint32_t value);
std::vector<Bytes> jobBlocks(const Bytes& records, bool preview);

}  // namespace beamgo
