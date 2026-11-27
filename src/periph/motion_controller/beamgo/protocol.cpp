#include "protocol.h"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#elif defined(__APPLE__)
#include <CommonCrypto/CommonCryptor.h>
#else
#include <openssl/evp.h>
#include <memory>
#endif

namespace beamgo {
namespace {
constexpr std::array<uint8_t, 16> kTxKey = {
    0x1b, 0x6e, 0x25, 0x18, 0x26, 0xaf, 0xd8, 0x46,
    0x2b, 0x17, 0x45, 0x88, 0x66, 0xc0, 0x48, 0x3e};
constexpr std::array<uint8_t, 16> kRxKey = {
    0x2b, 0x7e, 0x15, 0x26, 0x28, 0xae, 0xd2, 0xa8,
    0xab, 0xf7, 0x15, 0x88, 0x08, 0xcf, 0x4f, 0x3c};

Bytes crypt(const Bytes& input, bool encrypt) {
  const auto& key = encrypt ? kTxKey : kRxKey;
  std::array<uint8_t, 16> iv;
  iv.fill(1);
  Bytes output(input.size() + 16);
#ifdef _WIN32
  struct Handles {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_KEY_HANDLE key = nullptr;
    ~Handles() {
      if (key) BCryptDestroyKey(key);
      if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    }
  } handles;
  if (BCryptOpenAlgorithmProvider(&handles.algorithm, BCRYPT_AES_ALGORITHM, nullptr, 0) < 0 ||
      BCryptSetProperty(handles.algorithm, BCRYPT_CHAINING_MODE,
                        reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_CBC)),
                        sizeof(BCRYPT_CHAIN_MODE_CBC), 0) < 0 ||
      BCryptGenerateSymmetricKey(handles.algorithm, &handles.key, nullptr, 0,
                                 const_cast<PUCHAR>(key.data()), key.size(), 0) < 0) {
    throw std::runtime_error("AES initialization failed");
  }
  ULONG written = 0;
  const auto operation = encrypt ? BCryptEncrypt : BCryptDecrypt;
  if (operation(handles.key, const_cast<PUCHAR>(input.data()), static_cast<ULONG>(input.size()),
                nullptr, iv.data(), iv.size(), output.data(), static_cast<ULONG>(output.size()),
                &written, BCRYPT_BLOCK_PADDING) < 0) {
    throw std::runtime_error("AES operation failed");
  }
#elif defined(__APPLE__)
  size_t written = 0;
  if (CCCrypt(encrypt ? kCCEncrypt : kCCDecrypt, kCCAlgorithmAES, kCCOptionPKCS7Padding,
              key.data(), key.size(), iv.data(), input.data(), input.size(),
              output.data(), output.size(), &written) != kCCSuccess) {
    throw std::runtime_error("AES operation failed");
  }
#else
  std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)> context(
      EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
  int body = 0, tail = 0;
  if (!context ||
      EVP_CipherInit_ex(context.get(), EVP_aes_128_cbc(), nullptr, key.data(), iv.data(), encrypt) != 1 ||
      EVP_CipherUpdate(context.get(), output.data(), &body, input.data(), input.size()) != 1 ||
      EVP_CipherFinal_ex(context.get(), output.data() + body, &tail) != 1) {
    throw std::runtime_error("AES operation failed");
  }
  const size_t written = body + tail;
#endif
  output.resize(written);
  return output;
}

uint16_t read16(const Bytes& bytes, size_t offset) {
  return (uint16_t(bytes[offset]) << 8) | bytes[offset + 1];
}

void append16(Bytes& bytes, uint16_t value) {
  bytes.push_back(value >> 8);
  bytes.push_back(value & 0xff);
}
}  // namespace

uint16_t crc16(const Bytes& bytes) {
  uint16_t value = 0xffff;
  for (const auto byte : bytes) {
    value ^= uint16_t(byte) << 8;
    for (int bit = 0; bit < 8; ++bit) {
      value = (value & 0x8000) ? (value << 1) ^ 0x1021 : value << 1;
    }
  }
  return value;
}

Bytes encodeFrame(const Bytes& plaintext) {
  if (plaintext.size() > 65519) throw std::invalid_argument("USB payload too large");
  const auto cipher = crypt(plaintext, true);
  Bytes frame{0xa1};
  append16(frame, static_cast<uint16_t>(cipher.size() + 7));
  frame.insert(frame.end(), cipher.begin(), cipher.end());
  frame.insert(frame.end(), {0x66, 0x88});
  append16(frame, crc16(frame));
  return frame;
}

std::vector<Bytes> FrameDecoder::push(const Bytes& bytes) {
  buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
  std::vector<Bytes> messages;
  size_t offset = 0;
  while (offset < buffer_.size()) {
    if (buffer_[offset] != 0xb1) {
      ++offset;
      continue;
    }
    if (buffer_.size() - offset < 3) break;
    const size_t length = read16(buffer_, offset + 1);
    if (length < 21 || (length - 5) % 16 != 0) {
      ++offset;
      ++errors_;
      continue;
    }
    if (buffer_.size() - offset < length) {
      // A damaged frame can contain another B1 with a plausible, much larger length.
      // Do not let that false header hide a complete CRC-validated reply behind it.
      size_t candidate = offset + 1;
      for (; candidate + 21 <= buffer_.size(); ++candidate) {
        if (buffer_[candidate] != 0xb1) continue;
        const size_t next_length = read16(buffer_, candidate + 1);
        if (next_length < 21 || (next_length - 5) % 16 != 0 ||
            next_length > buffer_.size() - candidate) continue;
        const Bytes next_body(buffer_.begin() + candidate,
                              buffer_.begin() + candidate + next_length - 2);
        if (crc16(next_body) == read16(buffer_, candidate + next_length - 2)) break;
      }
      if (candidate + 21 > buffer_.size()) break;
      ++errors_;
      offset = candidate;
      continue;
    }
    const Bytes body(buffer_.begin() + offset, buffer_.begin() + offset + length - 2);
    if (crc16(body) != read16(buffer_, offset + length - 2)) {
      ++offset;
      ++errors_;
      continue;
    }
    try {
      messages.push_back(crypt(Bytes(body.begin() + 3, body.end()), false));
    } catch (const std::runtime_error&) {
      ++errors_;
    }
    offset += length;
  }
  buffer_.erase(buffer_.begin(), buffer_.begin() + offset);
  return messages;
}

void FrameDecoder::reset() {
  buffer_.clear();
  errors_ = 0;
}

Bytes record(uint8_t opcode, uint16_t a, uint16_t b) {
  Bytes result{opcode};
  append16(result, a);
  append16(result, b);
  return result;
}

Bytes record32(uint8_t opcode, uint32_t value) {
  return record(opcode, value >> 16, value & 0xffff);
}

std::vector<Bytes> jobBlocks(const Bytes& records, bool preview) {
  if (records.empty() || records.size() % 5 != 0) throw std::invalid_argument("Invalid records");
  if (preview && records.size() / 5 > 25000) throw std::invalid_argument("Preview too large");
  const size_t blocks = (records.size() + 4999) / 5000;
  if (blocks + 1 >= 0xffff) throw std::invalid_argument("Job too large");
  const uint8_t parameter = preview ? 0x07 : 0x01;
  std::vector<Bytes> result;
  if (!preview) {
    Bytes announce{0x10, parameter, 0xff, 0xff};
    append16(announce, blocks + 1);
    result.push_back(announce);
  }
  for (size_t index = 0; index < blocks; ++index) {
    Bytes block{0x10, parameter};
    append16(block, index);
    block.insert(block.end(), records.begin() + index * 5000,
                 records.begin() + std::min(records.size(), (index + 1) * 5000));
    result.push_back(std::move(block));
  }
  if (!preview) {
    Bytes end{0x10, parameter};
    append16(end, blocks);
    end.insert(end.end(), 5, 0xff);
    result.push_back(end);
  }
  return result;
}
}  // namespace beamgo
